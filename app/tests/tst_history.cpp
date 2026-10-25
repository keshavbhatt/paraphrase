// The history store (ADR-017) with a fake keyring: sealing, the wrong-key guard, delete all.

#include "history/history_crypto.h"
#include "history/history_keeper.h"
#include "history/history_store.h"
#include "history/secret_source.h"

#include <QCoreApplication>
#include <QFile>
#include <QSettings>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>

using namespace Qt::StringLiterals;
using namespace paraphrase::app::history;

namespace {

class FakeSecrets : public SecretSource
{
public:
    std::optional<QByteArray> secret = QByteArray("a desktop secret");
    int forgotten = 0;

    std::optional<QByteArray> fetch() override { return secret; }
    void forget() override { ++forgotten; }
    [[nodiscard]] QString kind() const override { return u"fake"_s; }
};

Entry entry(const QString& text, const QString& result, const QDateTime& when = {})
{
    Entry e;
    e.when = when;
    e.skillId = u"rewrite"_s;
    e.label = u"Tone · Elaborate"_s;
    e.app = u"KWrite"_s;
    e.text = text;
    e.result = result;
    e.options = R"({"tone":"elaborate"})";
    return e;
}

} // namespace

class TstHistory : public QObject
{
    Q_OBJECT

    QTemporaryDir m_settingsDir; ///< the tests' own QSettings, never the user's

private Q_SLOTS:
    void initTestCase()
    {
        QCoreApplication::setOrganizationName(u"paraphrase-tests"_s);
        QSettings::setPath(QSettings::NativeFormat, QSettings::UserScope, m_settingsDir.path());
    }

    static void init() { QSettings().clear(); }

    static void sealsAndOpens()
    {
        const QByteArray key = deriveKey("secret");
        QCOMPARE(key.size(), kKeyBytes);
        QCOMPARE(deriveKey("secret"), key);
        QVERIFY(deriveKey("other") != key);

        const QByteArray sealed = seal(key, "Meet at 6 near the station");
        QVERIFY(!sealed.contains("station"));
        QCOMPARE(open(key, sealed), QByteArray("Meet at 6 near the station"));
        // A fresh nonce each time.
        QVERIFY(seal(key, "same") != seal(key, "same"));
        QCOMPARE(open(key, seal(key, {})), QByteArray());

        QByteArray changed = sealed;
        changed[20] = static_cast<char>(static_cast<unsigned char>(changed.at(20)) ^ 1U);
        QVERIFY(!open(key, changed));
        QVERIFY(!open(deriveKey("other"), sealed));
        QVERIFY(!open(key, "short"));
    }

    static void keepsListsAndRemoves()
    {
        const QTemporaryDir dir;
        FakeSecrets secrets;
        const QString path = dir.filePath(u"history.sqlite"_s);
        auto store = Store::open(path, secrets);
        QVERIFY(store);

        const QDateTime now = QDateTime::currentDateTime();
        const qint64 older = store->add(entry(u"First text"_s, u"First result"_s, now.addDays(-2)));
        const qint64 newer = store->add(entry(u"दूसरा पाठ"_s, u"Second result"_s, now));
        QVERIFY(older > 0 && newer > older);

        const QList<Entry> all = store->entries();
        QCOMPARE(all.size(), 2);
        QCOMPARE(all.at(0).id, newer);
        QCOMPARE(all.at(0).text, u"दूसरा पाठ"_s);
        QCOMPARE(all.at(0).result, u"Second result"_s);
        QCOMPARE(all.at(0).label, u"Tone · Elaborate"_s);
        QCOMPARE(all.at(0).app, u"KWrite"_s);
        QCOMPARE(all.at(0).options, QByteArray(R"({"tone":"elaborate"})"));
        QCOMPARE(all.at(1).when.toMSecsSinceEpoch(), now.addDays(-2).toMSecsSinceEpoch());

        const Stats stats = store->stats();
        QCOMPARE(stats.count, 2);
        QCOMPARE(stats.oldest.toMSecsSinceEpoch(), now.addDays(-2).toMSecsSinceEpoch());
        QVERIFY(stats.bytes > 0);

        QVERIFY(store->updateResult(newer, u"Edited result"_s));
        QCOMPARE(store->entries().at(0).result, u"Edited result"_s);
        QCOMPARE(store->prune(now.addDays(-1)), 1);
        QVERIFY(store->remove(newer));
        QVERIFY(!store->remove(newer));
        QCOMPARE(store->entries().size(), 0);
    }

    static void textNeverReachesTheDiskInClear()
    {
        const QTemporaryDir dir;
        FakeSecrets secrets;
        const QString path = dir.filePath(u"history.sqlite"_s);
        {
            auto store = Store::open(path, secrets);
            QVERIFY(store);
            store->add(entry(u"my bank PIN is 4711"_s, u"a result about Mia"_s));
            const qint64 gone = store->add(entry(u"deleted words zebra"_s, u"gone result"_s));
            QVERIFY(store->remove(gone));
        }
        QFile file(path);
        QVERIFY(file.open(QIODevice::ReadOnly));
        const QByteArray bytes = file.readAll();
        QVERIFY(bytes.contains("KWrite")); // metadata stays in clear (ADR-017)
        for (const char* word : {"4711", "Mia", "zebra", "elaborate"}) {
            QVERIFY2(!bytes.contains(word), word);
        }
    }

    static void reopensOnlyWithTheSameSecret()
    {
        const QTemporaryDir dir;
        FakeSecrets secrets;
        const QString path = dir.filePath(u"history.sqlite"_s);
        Store::open(path, secrets)->add(entry(u"kept"_s, u"result"_s));

        auto again = Store::open(path, secrets);
        QVERIFY(again);
        QCOMPARE(again->entries().value(0).text, u"kept"_s);
        again.reset();

        FakeSecrets other;
        other.secret = QByteArray("another secret");
        Store::OpenError error{};
        QVERIFY(!Store::open(path, other, &error));
        QCOMPARE(error, Store::OpenError::WrongKey);
        QVERIFY(Store::open(path, secrets)); // the file was left as it was

        FakeSecrets none;
        none.secret.reset();
        QVERIFY(!Store::open(path, none, &error));
        QCOMPARE(error, Store::OpenError::NoSecret);
    }

    static void deleteAllRemovesTheFileAndForgets()
    {
        const QTemporaryDir dir;
        FakeSecrets secrets;
        const QString path = dir.filePath(u"sub/history.sqlite"_s);
        Store::open(path, secrets)->add(entry(u"kept"_s, u"result"_s));
        QVERIFY(QFile::exists(path));
        QVERIFY(Store::destroy(path, secrets));
        QVERIFY(!QFile::exists(path));
        QCOMPARE(secrets.forgotten, 1);
        // A new file starts empty, with a new salt.
        auto fresh = Store::open(path, secrets);
        QVERIFY(fresh);
        QCOMPARE(fresh->stats().count, 0);
    }

    static void keeperKeepsOnlyWhenOnAndNotExcluded()
    {
        const QTemporaryDir dir;
        int asked = 0;
        Keeper keeper(dir.filePath(u"history.sqlite"_s), [&asked](const QString& /*kind*/) {
            ++asked;
            return std::unique_ptr<SecretSource>(new FakeSecrets);
        });
        QVERIFY(!Keeper::isOn());
        QCOMPARE(keeper.record(entry(u"text"_s, u"result"_s)), 0);
        QCOMPARE(asked, 0); // off: the keyring is never asked
        QVERIFY(!QFile::exists(dir.filePath(u"history.sqlite"_s)));

        const QSignalSpy changed(&keeper, &Keeper::changed);
        QCOMPARE(keeper.turnOn(), Keeper::Problem::None);
        QVERIFY(Keeper::isOn());
        const qint64 kept = keeper.record(entry(u"text"_s, u"result"_s));
        QVERIFY(kept != 0);
        QVERIFY(keeper.update(kept, u"another version"_s));
        QCOMPARE(keeper.entries().value(0).result, u"another version"_s);
        Entry fromKeePass = entry(u"a password"_s, u"result"_s);
        fromKeePass.app = u"keepassxc"_s;
        QCOMPARE(keeper.record(fromKeePass), 0);
        QVERIFY(Keeper::isExcluded(u"Bitwarden"_s));
        QVERIFY(!Keeper::isExcluded(QString())); // the main window is always kept
        Entry fromWindow = entry(u"window text"_s, u"window result"_s);
        fromWindow.app = QString(); // null, as the window leaves it
        QVERIFY(keeper.record(fromWindow) != 0);
        QVERIFY(keeper.remove(keeper.entries().value(0).id));
        QCOMPARE(keeper.entries().size(), 1);
        QVERIFY(changed.size() >= 2);

        // Older than keepDays goes.
        keeper.record(entry(u"old"_s, u"old result"_s, QDateTime::currentDateTime().addDays(-40)));
        QCOMPARE(keeper.stats().count, 2);
        QCOMPARE(keeper.prune(), 1);
        keeper.setKeepDays(0);
        keeper.record(entry(u"old"_s, u"old result"_s, QDateTime::currentDateTime().addDays(-400)));
        QCOMPARE(keeper.prune(), 0);

        QVERIFY(keeper.deleteAll());
        QVERIFY(Keeper::isOn()); // stays on for what comes next
        QCOMPARE(keeper.entries().size(), 0);
        keeper.turnOff(true);
        QVERIFY(!Keeper::isOn());
        QVERIFY(!QFile::exists(dir.filePath(u"history.sqlite"_s)));
    }

    static void keeperWithoutAKeyringStaysOff()
    {
        const QTemporaryDir dir;
        Keeper keeper(dir.filePath(u"history.sqlite"_s), [](const QString& /*kind*/) { return nullptr; });
        QCOMPARE(keeper.turnOn(), Keeper::Problem::NoKeyring);
        QVERIFY(!Keeper::isOn());
    }
};

QTEST_GUILESS_MAIN(TstHistory)
#include "tst_history.moc"

// The D-Bus API end to end over a private peer connection (the daemon's private mode, ADR-010)
// with a fake engine: requests stream and finish, bad input and a missing model are error
// replies, the skill's tier picks the model, long input is refused, cancel and cancel-on-change
// work, signals reach only the client that asked, the language retry is announced, read-ahead
// warms the cache, and idle timers unload the model and end the daemon.

#include "api/daemon_service.h"

#include <QCryptographicHash>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusServer>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTest>
#include <QThread>

#include <atomic>

using namespace Qt::StringLiterals;
using paraphrase::api::DaemonService;
using paraphrase::api::ServiceSetup;
using paraphrase::engine::Engine;
using paraphrase::engine::EngineConfig;
using paraphrase::engine::Generation;
using paraphrase::engine::GenerationRequest;
using paraphrase::engine::TextSink;
using paraphrase::util::Result;

namespace {

constexpr int kTimeoutMs = 10'000;
const QString kHindi = u"मैं कल सुबह बाज़ार जाऊँगा और रात के खाने के लिए कुछ ताज़ी सब्ज़ियाँ खरीदूँगा।"_s;
const QString kEnglish = u"I will go to the market tomorrow morning to buy fresh vegetables for dinner."_s;

/// What the fake engine did, shared with the test across threads.
struct FakeState
{
    QMutex lock;
    QStringList answers; ///< scripted, one per generate(); the default answer after
    QString defaultAnswer = u"They are going home."_s;
    int pieceDelayMs = 0; ///< per streamed word
    QList<EngineConfig> loads;
    QStringList warmed;    ///< user prompts read ahead
    QList<int> cachedHits; ///< per generate(): whether the prompt matched the last warm
    bool loaded = false;
    std::atomic<bool> cancel{false};
};

class FakeEngine final : public Engine
{
public:
    explicit FakeEngine(std::shared_ptr<FakeState> state)
        : m_state(std::move(state))
    {}

    Result<bool> load(const EngineConfig& config) override
    {
        const QMutexLocker lock(&m_state->lock);
        m_state->loads.append(config);
        m_state->loaded = true;
        m_config = config;
        return Result<bool>::success(true);
    }
    void unload() override
    {
        const QMutexLocker lock(&m_state->lock);
        m_state->loaded = false;
        m_config = {};
    }
    [[nodiscard]] bool isLoaded() const override
    {
        const QMutexLocker lock(&m_state->lock);
        return m_state->loaded;
    }
    [[nodiscard]] EngineConfig loadedConfig() const override { return m_config; }
    [[nodiscard]] int countTokens(const QString& text) const override
    {
        return static_cast<int>(text.split(u' ', Qt::SkipEmptyParts).size());
    }
    Result<bool> warm(const GenerationRequest& request) override
    {
        const QMutexLocker lock(&m_state->lock);
        m_state->warmed.append(request.user);
        return Result<bool>::success(true);
    }
    void cancel() override { m_state->cancel.store(true); }
    void resetCancel() override { m_state->cancel.store(false); }

    Result<Generation> generate(const GenerationRequest& request, const TextSink& sink) override
    {
        QString answer;
        int delay = 0;
        {
            const QMutexLocker lock(&m_state->lock);
            answer = m_state->answers.isEmpty() ? m_state->defaultAnswer : m_state->answers.takeFirst();
            delay = m_state->pieceDelayMs;
            m_state->cachedHits.append(m_state->warmed.contains(request.user) ? 1 : 0);
        }
        Generation g;
        g.finishReason = u"stop"_s;
        QString done;
        for (const QString& word : answer.split(u' ')) {
            if (m_state->cancel.load()) {
                g.finishReason = u"cancelled"_s;
                break;
            }
            const QString piece = done.isEmpty() ? word : u' ' + word;
            done += piece;
            if (sink) {
                sink(0, piece);
            }
            ++g.generatedTokens;
            QThread::msleep(static_cast<unsigned long>(delay));
        }
        g.texts = {done};
        return Result<Generation>::success(g);
    }

private:
    std::shared_ptr<FakeState> m_state;
    EngineConfig m_config;
};

bool writeFile(const QString& path, const QByteArray& data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write(data) == data.size();
}

/// A catalog of tiny models, so they can be "downloaded" into a temporary folder.
QByteArray tinyCatalog()
{
    const auto entry = [](const QString& id, const QString& role, const QJsonArray& tiers, double ram) {
        const QByteArray content = id.toUtf8();
        QJsonObject o{
            {u"id"_s, id},
            {u"name"_s, id},
            {u"role"_s, role},
            {u"family"_s, u"test"_s},
            {u"hf_repo"_s, u"owner/repo"_s},
            {u"revision"_s, QString(40, u'a')},
            {
                u"files"_s,
                QJsonArray{
                    QJsonObject{
                        {u"file"_s, id + u".gguf"_s},
                        {u"size"_s, content.size()},
                        {
                            u"sha256"_s,
                            QString::fromLatin1(
                                QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex()),
                        },
                    },
                },
            },
            {u"ctx"_s, 4096},
            {u"licence"_s, u"MIT"_s},
            {u"licence_url"_s, u"https://example.org"_s},
            {u"tiers"_s, tiers},
            {u"ram_required"_s, ram},
            {u"memory_gb"_s, ram / 2},
        };
        if (role == u"draft"_s) {
            o.insert(u"draft_for"_s, QJsonArray{u"fast-model"_s});
        }
        return o;
    };
    const QJsonArray models{
        entry(u"fast-model"_s, u"model"_s, {u"fast"_s}, 8),
        entry(u"quality-model"_s, u"model"_s, {u"quality"_s}, 16),
        entry(u"fast-draft"_s, u"draft"_s, {}, 0),
    };
    return QJsonDocument(QJsonObject{{u"schema_version"_s, 2}, {u"models"_s, models}}).toJson();
}

/// Installs the tiny models with `ids` into `root`, as a finished download would.
bool install(const paraphrase::models::ModelCatalog& catalog, const QString& root, const QStringList& ids)
{
    const paraphrase::models::ModelStore store(root);
    return std::ranges::all_of(ids, [&](const QString& id) {
        const auto* m = catalog.find(id);
        return m != nullptr && writeFile(store.partPath(*m, m->files.front()), id.toUtf8()) &&
               store.installPart(*m, m->files.front()).ok();
    });
}

/// The daemon side: the service and a private D-Bus server on their own thread, as in the
/// daemon (a blocking D-Bus call cannot be answered by the thread that makes it).
class ServerThread : public QThread
{
public:
    ServerThread(ServiceSetup setup, QString address)
        : m_setup(std::move(setup))
        , m_address(std::move(address))
    {}

    std::atomic<bool> idleTimedOut{false};
    std::atomic<bool> ready{false};

protected:
    void run() override
    {
        DaemonService service(m_setup);
        const QDBusServer server(m_address);
        QObject::connect(&server, &QDBusServer::newConnection, &service,
                         [&service](const QDBusConnection& peer) { (void)service.exportOn(peer); });
        QObject::connect(&service, &DaemonService::idleTimeout, &service,
                         [this] { idleTimedOut.store(true); });
        ready.store(server.isConnected());
        exec();
    }

private:
    ServiceSetup m_setup;
    QString m_address;
};

/// Collects the signals one client receives.
class Listener : public QObject
{
    Q_OBJECT

public:
    QStringList tokens;
    QStringList retries;
    QHash<QString, QJsonObject> done;
    QHash<QString, QString> errors; ///< request id -> code

    bool listen(QDBusConnection& c)
    {
        const QString path = QString::fromLatin1(DaemonService::kObjectPath);
        const QString iface = QString::fromLatin1(DaemonService::kInterface);
        // QtDBus connects signals by name only (no pointer-to-member form).
        return c.connect({}, path, iface, u"Token"_s, this, SLOT(onToken(QString, int, QString))) &&
               c.connect({}, path, iface, u"Retrying"_s, this, SLOT(onRetrying(QString, QString))) &&
               c.connect({}, path, iface, u"Done"_s, this, SLOT(onDone(QString, QString))) &&
               c.connect({}, path, iface, u"Error"_s, this, SLOT(onError(QString, QString, QString)));
    }

public Q_SLOTS:
    void onToken(const QString& id, int variant, const QString& text)
    {
        Q_UNUSED(variant)
        tokens.append(id + u':' + text);
    }
    void onRetrying(const QString& id, const QString& reason) { retries.append(id + u':' + reason); }
    void onDone(const QString& id, const QString& result)
    {
        done.insert(id, QJsonDocument::fromJson(result.toUtf8()).object());
    }
    void onError(const QString& id, const QString& code, const QString& message)
    {
        Q_UNUSED(message)
        errors.insert(id, code);
    }
};

} // namespace

class TestApi : public QObject
{
    Q_OBJECT

private:
    std::unique_ptr<QTemporaryDir> m_dir;
    std::shared_ptr<FakeState> m_state;
    std::unique_ptr<ServerThread> m_server;
    paraphrase::models::ModelCatalog m_catalog;
    int m_peers = 0;

    void startServer(double memoryGb, const QStringList& installed, int idleUnloadMs = 60'000,
                     int idleExitMs = 60'000)
    {
        m_dir = std::make_unique<QTemporaryDir>();
        m_state = std::make_shared<FakeState>();
        m_catalog = paraphrase::models::ModelCatalog::parse(tinyCatalog()).value();
        QVERIFY(install(m_catalog, m_dir->path() + u"/models"_s, installed));
        auto skills =
            paraphrase::skills::SkillRegistry::load(QStringLiteral(PARAPHRASE_REPO_ROOT) + u"/skills"_s);
        QVERIFY(skills.ok());
        paraphrase::models::HardwareInfo hw;
        hw.totalMemory = static_cast<qint64>(memoryGb * 1024 * 1024 * 1024);
        hw.threads = 4;
        const auto state = m_state;
        m_server = std::make_unique<ServerThread>(
            ServiceSetup{
                .skills = std::move(skills).value(),
                .catalog = m_catalog,
                .modelsRoot = m_dir->path() + u"/models"_s,
                .hardware = hw,
                .engineFactory = [state] { return std::make_unique<FakeEngine>(state); },
                .idleUnloadMs = idleUnloadMs,
                .idleExitMs = idleExitMs,
            },
            u"unix:path="_s + m_dir->path() + u"/daemon.sock"_s);
        m_server->start();
        QTRY_VERIFY_WITH_TIMEOUT(m_server->ready.load(), kTimeoutMs);
    }

    QDBusConnection connect()
    {
        return QDBusConnection::connectToPeer(u"unix:path="_s + m_dir->path() + u"/daemon.sock"_s,
                                              u"peer%1"_s.arg(++m_peers));
    }

    static QDBusMessage call(QDBusConnection& c, const QString& method, const QVariantList& args)
    {
        QDBusMessage msg =
            QDBusMessage::createMethodCall({}, QString::fromLatin1(DaemonService::kObjectPath),
                                           QString::fromLatin1(DaemonService::kInterface), method);
        msg.setArguments(args);
        return c.call(msg, QDBus::BlockWithGui);
    }

    static QString runSkill(QDBusConnection& c, const QString& skill, const QVariantMap& input)
    {
        const QDBusMessage reply = call(c, u"RunSkill"_s, {skill, input, QVariantMap()});
        return reply.type() == QDBusMessage::ReplyMessage ? reply.arguments().value(0).toString()
                                                          : reply.errorName();
    }

private Q_SLOTS:
    void cleanup()
    {
        for (int i = 1; i <= m_peers; ++i) {
            QDBusConnection::disconnectFromPeer(u"peer%1"_s.arg(i));
        }
        if (m_server) {
            m_server->quit();
            m_server->wait();
            m_server.reset();
        }
    }

    void listsSkills()
    {
        startServer(32, {u"fast-model"_s});
        QDBusConnection c = connect();
        QVERIFY(c.isConnected());
        const QJsonArray skills =
            QJsonDocument::fromJson(call(c, u"ListSkills"_s, {}).arguments().value(0).toString().toUtf8())
                .array();
        QStringList ids;
        for (const auto& s : skills) {
            ids.append(s.toObject().value(u"id"_s).toString());
        }
        QVERIFY(ids.contains(u"proofread"_s) && ids.contains(u"reply"_s) &&
                ids.contains(u"rewrite.professional"_s));
    }

    void runsASkillAndStreams()
    {
        startServer(32, {u"fast-model"_s, u"fast-draft"_s});
        QDBusConnection c = connect();
        Listener l;
        QVERIFY(l.listen(c));
        const QString id = runSkill(c, u"proofread"_s, {{u"text"_s, u"Their going home."_s}});
        QVERIFY(!id.startsWith(u"com."_s));
        QTRY_VERIFY_WITH_TIMEOUT(l.done.contains(id), kTimeoutMs);
        const QJsonObject done = l.done.value(id);
        QCOMPARE(done.value(u"output"_s).toString(), u"They are going home."_s);
        QCOMPARE(done.value(u"model"_s).toString(), u"fast-model"_s);
        QCOMPARE(done.value(u"skill"_s).toString(), u"proofread"_s);
        QCOMPARE(done.value(u"finish"_s).toString(), u"stop"_s);
        QCOMPARE(done.value(u"calls"_s).toArray().size(), 1);
        QString streamed;
        for (const QString& t : std::as_const(l.tokens)) {
            streamed += t.section(u':', 1);
        }
        QCOMPARE(streamed, u"They are going home."_s);
        // The fast model, with its draft head, on the probed threads.
        const QMutexLocker lock(&m_state->lock);
        QCOMPARE(m_state->loads.size(), 1);
        QVERIFY(m_state->loads.front().modelPath.endsWith(u"fast-model/fast-model.gguf"_s));
        QVERIFY(m_state->loads.front().draftModelPath.endsWith(u"fast-draft/fast-draft.gguf"_s));
        QCOMPARE(m_state->loads.front().threads, 4);
    }

    void refusesBadInput()
    {
        startServer(32, {u"fast-model"_s});
        QDBusConnection c = connect();
        const QString invalid = u"com.ktechpit.paraphrase.Error.InvalidInput"_s;
        QCOMPARE(runSkill(c, u"nonsense"_s, {{u"text"_s, u"x"_s}}), invalid);
        QCOMPARE(runSkill(c, u"proofread"_s, {{u"txt"_s, u"x"_s}}), invalid);
        QCOMPARE(runSkill(c, u"proofread"_s, {{u"text"_s, 5}}), invalid);
        QCOMPARE(runSkill(c, u"proofread"_s, {{u"text"_s, u"  "_s}}), invalid);
        QCOMPARE(runSkill(c, u"reply"_s, {{u"text"_s, u"ok"_s}}), invalid); // needs the message
        QCOMPARE(
            runSkill(c, u"rewrite.professional"_s, {{u"text"_s, u"x"_s}, {u"target_lang"_s, u"klingon"_s}}),
            invalid);
    }

    void saysWhenNoModelIsInstalled()
    {
        startServer(32, {});
        QDBusConnection c = connect();
        QCOMPARE(runSkill(c, u"proofread"_s, {{u"text"_s, u"x"_s}}),
                 u"com.ktechpit.paraphrase.Error.NoModel"_s);
    }

    void picksTheModelForTheTier()
    {
        startServer(32, {u"fast-model"_s, u"quality-model"_s});
        QDBusConnection c = connect();
        Listener l;
        QVERIFY(l.listen(c));
        const QString id = runSkill(c, u"rewrite.professional"_s, {{u"text"_s, u"hey send it"_s}});
        QTRY_VERIFY_WITH_TIMEOUT(l.done.contains(id), kTimeoutMs);
        QCOMPARE(l.done.value(id).value(u"model"_s).toString(), u"quality-model"_s);
        cleanup();

        // An 8 GB computer runs rewrite on the fast model.
        startServer(7.6, {u"fast-model"_s, u"quality-model"_s});
        QDBusConnection small = connect();
        Listener ls;
        QVERIFY(ls.listen(small));
        const QString id2 = runSkill(small, u"rewrite.professional"_s, {{u"text"_s, u"hey send it"_s}});
        QTRY_VERIFY_WITH_TIMEOUT(ls.done.contains(id2), kTimeoutMs);
        QCOMPARE(ls.done.value(id2).value(u"model"_s).toString(), u"fast-model"_s);
    }

    void refusesTooLongInput()
    {
        startServer(32, {u"fast-model"_s});
        QDBusConnection c = connect();
        Listener l;
        QVERIFY(l.listen(c));
        const QString id = runSkill(c, u"proofread"_s, {{u"text"_s, QString(u"word "_s).repeated(1600)}});
        QTRY_VERIFY_WITH_TIMEOUT(l.errors.contains(id), kTimeoutMs);
        QCOMPARE(l.errors.value(id), u"too-long"_s);
    }

    void cancels()
    {
        startServer(32, {u"fast-model"_s});
        m_state->defaultAnswer = QString(u"word "_s).repeated(200).trimmed();
        m_state->pieceDelayMs = 20;
        QDBusConnection c = connect();
        Listener l;
        QVERIFY(l.listen(c));
        const QString id = runSkill(c, u"proofread"_s, {{u"text"_s, u"Their going home."_s}});
        QTRY_VERIFY_WITH_TIMEOUT(!l.tokens.isEmpty(), kTimeoutMs);
        call(c, u"Cancel"_s, {id});
        QTRY_VERIFY_WITH_TIMEOUT(l.errors.contains(id), kTimeoutMs);
        QCOMPARE(l.errors.value(id), u"cancelled"_s);
        QVERIFY(l.tokens.size() < 100);
    }

    void aNewRequestCancelsTheClientsLastOne()
    {
        startServer(32, {u"fast-model"_s});
        m_state->defaultAnswer = QString(u"word "_s).repeated(100).trimmed();
        m_state->pieceDelayMs = 10;
        QDBusConnection c = connect();
        Listener l;
        QVERIFY(l.listen(c));
        const QString first = runSkill(c, u"proofread"_s, {{u"text"_s, u"one"_s}});
        QTRY_VERIFY_WITH_TIMEOUT(!l.tokens.isEmpty(), kTimeoutMs);
        const QString queued = runSkill(c, u"proofread"_s, {{u"text"_s, u"two"_s}});
        const QString last = runSkill(c, u"proofread"_s, {{u"text"_s, u"three"_s}});
        QTRY_VERIFY_WITH_TIMEOUT(l.done.contains(last), kTimeoutMs);
        QCOMPARE(l.errors.value(first), u"cancelled"_s);
        QCOMPARE(l.errors.value(queued), u"cancelled"_s);
    }

    void signalsReachOnlyTheClientThatAsked()
    {
        startServer(32, {u"fast-model"_s});
        m_state->pieceDelayMs = 50;
        QDBusConnection a = connect();
        QDBusConnection b = connect();
        Listener la;
        Listener lb;
        QVERIFY(la.listen(a) && lb.listen(b));
        const QString id = runSkill(a, u"proofread"_s, {{u"text"_s, u"Their going home."_s}});
        QTRY_VERIFY_WITH_TIMEOUT(!la.tokens.isEmpty(), kTimeoutMs);
        call(b, u"Cancel"_s, {id}); // not b's request: ignored
        QTRY_VERIFY_WITH_TIMEOUT(la.done.contains(id), kTimeoutMs);
        QVERIFY(lb.tokens.isEmpty() && lb.done.isEmpty() && lb.errors.isEmpty());
    }

    void announcesTheLanguageRetry()
    {
        startServer(32, {u"fast-model"_s});
        m_state->answers = {kEnglish, kHindi};
        QDBusConnection c = connect();
        Listener l;
        QVERIFY(l.listen(c));
        const QString id = runSkill(c, u"proofread"_s, {{u"text"_s, kHindi}});
        QTRY_VERIFY_WITH_TIMEOUT(l.done.contains(id), kTimeoutMs);
        QCOMPARE(l.retries, QStringList{id + u":Hindi answered outside Devanagari"_s});
        QVERIFY(l.done.value(id).value(u"retried"_s).toBool());
        QCOMPARE(l.done.value(id).value(u"output"_s).toString(), kHindi);
        QCOMPARE(l.done.value(id).value(u"calls"_s).toArray().size(), 2);
    }

    void readsAhead()
    {
        startServer(32, {u"fast-model"_s});
        QDBusConnection c = connect();
        Listener l;
        QVERIFY(l.listen(c));
        const QVariantMap input{{u"text"_s, u"Their going home."_s}};
        call(c, u"Warm"_s, {u"proofread"_s, input});
        QTRY_VERIFY_WITH_TIMEOUT(
            [this] {
                const QMutexLocker lock(&m_state->lock);
                return m_state->warmed.size() == 1;
            }(),
            kTimeoutMs);
        const QString id = runSkill(c, u"proofread"_s, input);
        QTRY_VERIFY_WITH_TIMEOUT(l.done.contains(id), kTimeoutMs);
        const QMutexLocker lock(&m_state->lock);
        QCOMPARE(m_state->cachedHits, QList<int>{1}); // the same prompt was read ahead
    }

    void unloadsAndExitsWhenIdle()
    {
        startServer(32, {u"fast-model"_s}, 200, 600);
        QDBusConnection c = connect();
        Listener l;
        QVERIFY(l.listen(c));
        const QString id = runSkill(c, u"proofread"_s, {{u"text"_s, u"Their going home."_s}});
        QTRY_VERIFY_WITH_TIMEOUT(l.done.contains(id), kTimeoutMs);
        QTRY_VERIFY_WITH_TIMEOUT(
            [this] {
                const QMutexLocker lock(&m_state->lock);
                return !m_state->loaded;
            }(),
            kTimeoutMs);
        const QJsonObject status =
            QJsonDocument::fromJson(call(c, u"Status"_s, {}).arguments().value(0).toString().toUtf8())
                .object();
        QCOMPARE(status.value(u"model"_s).toString(), QString());
        QCOMPARE(status.value(u"installed"_s).toArray(), QJsonArray{u"fast-model"_s});
        QTRY_VERIFY_WITH_TIMEOUT(m_server->idleTimedOut.load(), kTimeoutMs);
    }
};

QTEST_GUILESS_MAIN(TestApi)
#include "tst_api.moc"

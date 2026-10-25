// The D-Bus API end to end over a private peer connection (the daemon's private mode, ADR-010)
// with a fake engine: requests stream and finish, bad input and a missing model are error
// replies, the skill's tier picks the model, long input is refused, cancel and cancel-on-change
// work, signals reach only the client that asked, the language retry is announced, read-ahead
// warms the cache, and idle timers unload the model and end the daemon.

#include "api/daemon_service.h"
#include "fake_daemon.h"
#include "test_http_server.h"

#include <QCryptographicHash>
#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusReply>
#include <QDBusServer>
#include <QDir>
#include <QElapsedTimer>
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
using paraphrase::engine::TextSink;

using namespace paraphrase::testing;

namespace {

constexpr int kTimeoutMs = 10'000;
const QString kHindi = u"मैं कल सुबह बाज़ार जाऊँगा और रात के खाने के लिए कुछ ताज़ी सब्ज़ियाँ खरीदूँगा।"_s;
const QString kEnglish = u"I will go to the market tomorrow morning to buy fresh vegetables for dinner."_s;

/// Collects the signals one client receives.
class Listener : public QObject
{
    Q_OBJECT

public:
    QStringList tokens;
    QStringList retries;
    QHash<QString, QJsonObject> done;
    QHash<QString, QString> errors; ///< request id -> code
    QList<QJsonObject> progress;

    bool listen(QDBusConnection& c)
    {
        const QString path = QString::fromLatin1(DaemonService::kObjectPath);
        const QString iface = QString::fromLatin1(DaemonService::kInterface);
        // QtDBus connects signals by name only (no pointer-to-member form).
        return c.connect({}, path, iface, u"Token"_s, this, SLOT(onToken(QString, int, QString))) &&
               c.connect({}, path, iface, u"Retrying"_s, this, SLOT(onRetrying(QString, QString))) &&
               c.connect({}, path, iface, u"Done"_s, this, SLOT(onDone(QString, QString))) &&
               c.connect({}, path, iface, u"Error"_s, this, SLOT(onError(QString, QString, QString))) &&
               c.connect({}, path, iface, u"DownloadProgress"_s, this, SLOT(onProgress(QString)));
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
    void onProgress(const QString& status)
    {
        progress.append(QJsonDocument::fromJson(status.toUtf8()).object());
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
    std::unique_ptr<TestHttpServer> m_http; ///< where the tiny models download from
    paraphrase::models::ModelCatalog m_catalog;
    int m_peers = 0;
    QString m_settingsPath; ///< kept by the daemon when set (SetTierModel, SetSpeedUp)
    QString m_skillsDir;    ///< the skills to load; the shipped ones when empty

    void startServer(double memoryGb, const QStringList& installed, int idleUnloadMs = 60'000,
                     int idleExitMs = 60'000)
    {
        m_dir = std::make_unique<QTemporaryDir>();
        m_state = std::make_shared<FakeState>();
        m_catalog = paraphrase::models::ModelCatalog::parse(tinyCatalog()).value();
        QVERIFY(install(m_catalog, m_dir->path() + u"/models"_s, installed));
        const QString skillsDir =
            m_skillsDir.isEmpty() ? QStringLiteral(PARAPHRASE_REPO_ROOT) + u"/skills"_s : m_skillsDir;
        auto skills = paraphrase::skills::SkillRegistry::load(skillsDir);
        QVERIFY2(skills.ok(), qPrintable(skills.ok() ? QString() : skills.error()));
        paraphrase::models::HardwareInfo hw;
        hw.totalMemory = static_cast<qint64>(memoryGb * 1024 * 1024 * 1024);
        hw.threads = 4;
        m_http = std::make_unique<TestHttpServer>();
        for (const auto& m : m_catalog.models) {
            m_http->addRoute(u'/' + m.files.front().file, {.body = m.id.toUtf8(), .supportsRange = true});
        }
        const QUrl base = m_http->urlFor(u"/"_s);
        m_server = std::make_unique<ServerThread>(
            ServiceSetup{
                .skills = std::move(skills).value(),
                .catalog = m_catalog,
                .modelsRoot = m_dir->path() + u"/models"_s,
                .hardware = hw,
                .engineFactory = [state = m_state] { return std::make_unique<FakeEngine>(state); },
                .urlResolver =
                    [base](const paraphrase::models::ModelEntry&, const paraphrase::models::ModelFile& f) {
                        return base.resolved(QUrl(f.file));
                    },
                .idleUnloadMs = idleUnloadMs,
                .idleExitMs = idleExitMs,
                .settingsPath = m_settingsPath,
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
    void anotherVersionSamplesFreer()
    {
        // RunSkill's `variety` option (owner, 2026-10-25: "Another version" came back almost the
        // same): a sampled skill runs hotter, a greedy one stays greedy.
        startServer(32, {u"fast-model"_s, u"quality-model"_s});
        QDBusConnection c = connect();
        Listener l;
        QVERIFY(l.listen(c));
        const QVariantMap text{{u"text"_s, u"hey send it"_s}};
        const QVariantMap variety{{u"variety"_s, true}};
        const QString plain = runSkill(c, u"rewrite.professional"_s, text);
        QTRY_VERIFY_WITH_TIMEOUT(l.done.contains(plain), kTimeoutMs);
        const QString again = call(c, u"RunSkill"_s, {u"rewrite.professional"_s, text, variety})
                                  .arguments()
                                  .value(0)
                                  .toString();
        QTRY_VERIFY_WITH_TIMEOUT(l.done.contains(again), kTimeoutMs);
        const QString greedy =
            call(c, u"RunSkill"_s, {u"proofread"_s, text, variety}).arguments().value(0).toString();
        QTRY_VERIFY_WITH_TIMEOUT(l.done.contains(greedy), kTimeoutMs);
        const QMutexLocker lock(&m_state->lock);
        const QList<double>& t = m_state->temperatures;
        QCOMPARE(t.size(), 3);
        QVERIFY(t.at(0) < DaemonService::kVarietyTemperature);
        QCOMPARE(t.at(1), DaemonService::kVarietyTemperature);
        QCOMPARE(t.at(2), 0.0);
    }

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
        m_http.reset();
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
        // Only rewrite's prompt takes a target language; the others would ignore it silently.
        QCOMPARE(runSkill(c, u"reply"_s, {{u"context"_s, u"hi"_s}, {u"target_lang"_s, u"fr"_s}}), invalid);
        QCOMPARE(runSkill(c, u"proofread"_s, {{u"text"_s, u"hi"_s}, {u"target_lang"_s, u"fr"_s}}), invalid);
        QCOMPARE(
            runSkill(c, u"rewrite.professional"_s, {{u"text"_s, u"x"_s}, {u"target_lang"_s, u"klingon"_s}}),
            invalid);
        // intent and tone reach only prompts that use them; a preset's tone cannot be replaced.
        QCOMPARE(runSkill(c, u"proofread"_s, {{u"text"_s, u"hi"_s}, {u"intent"_s, u"say no"_s}}), invalid);
        QCOMPARE(runSkill(c, u"proofread"_s, {{u"text"_s, u"hi"_s}, {u"tone"_s, u"casual"_s}}), invalid);
        QCOMPARE(runSkill(c, u"rewrite.professional"_s, {{u"text"_s, u"hi"_s}, {u"tone"_s, u"rude"_s}}),
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

    void keepsTheUsersModelChoices()
    {
        const QTemporaryDir config;
        m_settingsPath = config.filePath(u"paraphrase/daemon.json"_s);
        startServer(32, {u"fast-model"_s, u"quality-model"_s, u"fast-draft"_s});
        QDBusConnection c = connect();
        const auto status = [&c] {
            return QJsonDocument::fromJson(call(c, u"Status"_s, {}).arguments().value(0).toString().toUtf8())
                .object();
        };
        QCOMPARE(status().value(u"tier_models"_s).toObject().value(u"quality"_s).toString(),
                 u"quality-model"_s);
        QVERIFY(status().value(u"speed_up"_s).toBool());
        // Settings: the quality tier on the fast model, no speed-up.
        QCOMPARE(call(c, u"SetTierModel"_s, {u"quality"_s, u"fast-model"_s}).type(),
                 QDBusMessage::ReplyMessage);
        QCOMPARE(call(c, u"SetSpeedUp"_s, {false}).type(), QDBusMessage::ReplyMessage);
        QCOMPARE(call(c, u"SetTierModel"_s, {u"huge"_s, u"fast-model"_s}).type(), QDBusMessage::ErrorMessage);
        QCOMPARE(call(c, u"SetTierModel"_s, {u"fast"_s, u"fast-draft"_s}).type(), QDBusMessage::ErrorMessage);
        Listener l;
        QVERIFY(l.listen(c));
        const QString id = runSkill(c, u"rewrite.professional"_s, {{u"text"_s, u"hey send it"_s}});
        QTRY_VERIFY_WITH_TIMEOUT(l.done.contains(id), kTimeoutMs);
        QCOMPARE(l.done.value(id).value(u"model"_s).toString(), u"fast-model"_s);
        {
            const QMutexLocker lock(&m_state->lock);
            QVERIFY(m_state->loads.last().draftModelPath.isEmpty()); // the speed-up is off
        }
        cleanup();

        // Kept across a restart; "" goes back to the automatic choice.
        startServer(32, {u"fast-model"_s, u"quality-model"_s});
        QDBusConnection again = connect();
        const QJsonObject s =
            QJsonDocument::fromJson(call(again, u"Status"_s, {}).arguments().value(0).toString().toUtf8())
                .object();
        QCOMPARE(s.value(u"tier_choice"_s).toObject().value(u"quality"_s).toString(), u"fast-model"_s);
        QVERIFY(!s.value(u"speed_up"_s).toBool());
        QCOMPARE(call(again, u"SetTierModel"_s, {u"quality"_s, QString()}).type(),
                 QDBusMessage::ReplyMessage);
        const QJsonObject reset =
            QJsonDocument::fromJson(call(again, u"Status"_s, {}).arguments().value(0).toString().toUtf8())
                .object();
        QCOMPARE(reset.value(u"tier_models"_s).toObject().value(u"quality"_s).toString(), u"quality-model"_s);
        m_settingsPath.clear();
    }

    void runsOnTheRequestedModel()
    {
        startServer(32, {u"fast-model"_s, u"quality-model"_s});
        QDBusConnection c = connect();
        Listener l;
        QVERIFY(l.listen(c));
        const QDBusMessage reply = call(c, u"RunSkill"_s,
                                        {
                                            u"proofread"_s,
                                            QVariantMap{{u"text"_s, u"x y"_s}},
                                            QVariantMap{{u"model"_s, u"quality-model"_s}},
                                        });
        const QString id = reply.arguments().value(0).toString();
        QTRY_VERIFY_WITH_TIMEOUT(l.done.contains(id), kTimeoutMs);
        QCOMPARE(l.done.value(id).value(u"model"_s).toString(), u"quality-model"_s);
        QVERIFY(l.done.value(id).value(u"load_s"_s).toDouble() >= 0.0);
        const auto run = [&](const QVariantMap& options) {
            return call(c, u"RunSkill"_s, {u"proofread"_s, QVariantMap{{u"text"_s, u"x"_s}}, options})
                .errorName();
        };
        QCOMPARE(run({{u"model"_s, u"fast-draft"_s}}), u"com.ktechpit.paraphrase.Error.InvalidInput"_s);
        QCOMPARE(run({{u"colour"_s, u"red"_s}}), u"com.ktechpit.paraphrase.Error.InvalidInput"_s);
        cleanup();
        startServer(32, {u"fast-model"_s});
        QDBusConnection c2 = connect();
        QCOMPARE(call(c2, u"RunSkill"_s,
                      {u"proofread"_s, QVariantMap{{u"text"_s, u"x"_s}},
                       QVariantMap{{u"model"_s, u"quality-model"_s}}})
                     .errorName(),
                 u"com.ktechpit.paraphrase.Error.NoModel"_s);
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

    void keepsTheUsersOwnActions()
    {
        // A skill that takes user presets (schema v5): the user's actions become skills,
        // run with their own tier and instruction, and are kept next to the settings.
        const QTemporaryDir skillsDir;
        QVERIFY(QFile::copy(QStringLiteral(PARAPHRASE_REPO_ROOT "/skills/proofread.yaml"),
                            skillsDir.filePath(u"proofread.yaml"_s)));
        QVERIFY(writeFile(
            skillsDir.filePath(u"custom.yaml"_s),
            "id: custom\nversion: 5\ntitle: My actions\ntier: fast\nmax_input_tokens: 500\n"
            "max_output_tokens: 200\nsampling: { temperature: 0.2 }\noutput: { type: text }\n"
            "user_presets: { values: [instruction] }\nsystem: Do what the user asks with the text.\n"
            "template: |\n  {{instruction}}\n  <text>\n  {{text}}\n  </text>\n"));
        const QTemporaryDir config;
        m_skillsDir = skillsDir.path();
        m_settingsPath = config.filePath(u"paraphrase/daemon.json"_s);
        startServer(32, {u"fast-model"_s, u"quality-model"_s});
        QDBusConnection c = connect();
        QVERIFY(
            QJsonDocument::fromJson(call(c, u"ListActions"_s, {}).arguments().value(0).toString().toUtf8())
                .object()
                .value(u"supported"_s)
                .toBool());
        // The template itself is not listed, but runs with an instruction: Try it before saving.
        QVERIFY(!call(c, u"ListSkills"_s, {}).arguments().value(0).toString().contains(u"\"custom\""_s));
        {
            Listener l;
            QVERIFY(l.listen(c));
            const QVariantMap input{
                {u"text"_s, u"Launch on March 12."_s},
                {u"instruction"_s, u"Make it a haiku"_s},
            };
            const QDBusMessage tried =
                call(c, u"RunSkill"_s, {u"custom"_s, input, QVariantMap{{u"tier"_s, u"fast"_s}}});
            const QString triedId = tried.arguments().value(0).toString();
            QTRY_VERIFY_WITH_TIMEOUT(l.done.contains(triedId), kTimeoutMs);
            QCOMPARE(l.done.value(triedId).value(u"model"_s).toString(), u"fast-model"_s);
            const QMutexLocker lock(&m_state->lock);
            QVERIFY(m_state->prompts.last().contains(u"Make it a haiku"_s));
        }
        const QString invalid = u"com.ktechpit.paraphrase.Error.InvalidInput"_s;
        QCOMPARE(runSkill(c, u"custom"_s, {{u"text"_s, u"x"_s}}), invalid); // no instruction
        const QDBusMessage saved =
            call(c, u"SaveAction"_s, {QString(), u"Tweet"_s, u"Turn this into a tweet"_s, u"quality"_s});
        QCOMPARE(saved.arguments().value(0).toString(), u"custom.tweet"_s);
        QVERIFY(call(c, u"ListSkills"_s, {}).arguments().value(0).toString().contains(u"custom.tweet"_s));
        QCOMPARE(call(c, u"SaveAction"_s, {QString(), QString(), u"x"_s, u"fast"_s}).type(),
                 QDBusMessage::ErrorMessage);

        Listener l;
        QVERIFY(l.listen(c));
        const QString id = runSkill(c, u"custom.tweet"_s, {{u"text"_s, u"We launch on March 12."_s}});
        QTRY_VERIFY_WITH_TIMEOUT(l.done.contains(id), kTimeoutMs);
        QCOMPARE(l.done.value(id).value(u"model"_s).toString(), u"quality-model"_s); // the action's tier
        {
            const QMutexLocker lock(&m_state->lock);
            QVERIFY(m_state->prompts.last().contains(u"Turn this into a tweet"_s));
        }
        // Kept across restarts, and deleted for good.
        cleanup();
        startServer(32, {u"fast-model"_s});
        QDBusConnection again = connect();
        QVERIFY(call(again, u"ListSkills"_s, {}).arguments().value(0).toString().contains(u"custom.tweet"_s));
        QCOMPARE(call(again, u"DeleteAction"_s, {u"tweet"_s}).type(), QDBusMessage::ReplyMessage);
        QVERIFY(
            !call(again, u"ListSkills"_s, {}).arguments().value(0).toString().contains(u"custom.tweet"_s));
        m_skillsDir.clear();
        m_settingsPath.clear();
    }

    void keepsBothModelsLoadedWhenTheyFit()
    {
        // Fast (4 GB) and quality (8 GB) plus a 6 GB reserve fit in 32 GB: switching between
        // proofread and a tone loads each model once. In 16 GB they take turns.
        for (const auto& [memoryGb, loads] : {std::pair{32.0, 2}, std::pair{16.0, 3}}) {
            startServer(memoryGb, {u"fast-model"_s, u"quality-model"_s});
            QDBusConnection c = connect();
            Listener l;
            QVERIFY(l.listen(c));
            for (const QString& skill : {u"proofread"_s, u"rewrite.polite"_s, u"proofread"_s}) {
                const QString id = runSkill(c, skill, {{u"text"_s, u"Their going home."_s}});
                QTRY_VERIFY_WITH_TIMEOUT(l.done.contains(id), kTimeoutMs);
            }
            qsizetype loaded = 0;
            {
                const QMutexLocker lock(&m_state->lock);
                loaded = m_state->loads.size();
            }
            QCOMPARE(loaded, loads);
            cleanup();
        }
    }

    void cancelsAtOnceDuringAModelLoad()
    {
        // Ctrl+C while the model loads: the client hears "cancelled" at once, not after the
        // load, and nothing of the dropped request follows.
        startServer(32, {u"fast-model"_s});
        m_state->loadDelayMs = 1500;
        QDBusConnection c = connect();
        Listener l;
        QVERIFY(l.listen(c));
        const QString id = runSkill(c, u"proofread"_s, {{u"text"_s, u"Their going home."_s}});
        QTest::qWait(100);
        QElapsedTimer timer;
        timer.start();
        call(c, u"Cancel"_s, {id});
        QTRY_VERIFY_WITH_TIMEOUT(l.errors.contains(id), kTimeoutMs);
        QVERIFY2(timer.elapsed() < 1000, "the cancel waited for the model load");
        QCOMPARE(l.errors.value(id), u"cancelled"_s);
        // The next request runs once the load is over; the cancelled one never produced text.
        const QString next = runSkill(c, u"proofread"_s, {{u"text"_s, u"Their going home."_s}});
        QTRY_VERIFY_WITH_TIMEOUT(l.done.contains(next), kTimeoutMs);
        QVERIFY(!l.done.contains(id));
        QVERIFY(std::ranges::none_of(l.tokens, [&](const QString& t) { return t.startsWith(id + u':'); }));
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

    void managesModels()
    {
        startServer(32, {});
        QDBusConnection c = connect();
        Listener l;
        QVERIFY(l.listen(c));
        const auto models = [&] {
            QHash<QString, QJsonObject> out;
            const auto list = QJsonDocument::fromJson(
                call(c, u"ListModels"_s, {}).arguments().value(0).toString().toUtf8());
            for (const auto& m : list.array()) {
                out.insert(m.toObject().value(u"id"_s).toString(), m.toObject());
            }
            return out;
        };
        const auto listed = models();
        QCOMPARE(listed.size(), 3);
        QVERIFY(!listed.value(u"fast-model"_s).value(u"installed"_s).toBool());
        QVERIFY(listed.value(u"fast-model"_s).value(u"recommended"_s).toBool());
        QVERIFY(listed.value(u"fast-model"_s)
                    .value(u"source"_s)
                    .toString()
                    .startsWith(u"https://huggingface.co/"_s));

        // The model and its draft head download, are checked, and are installed.
        call(c, u"DownloadModel"_s, {u"fast-model"_s});
        QTRY_VERIFY_WITH_TIMEOUT(models().value(u"fast-model"_s).value(u"installed"_s).toBool() &&
                                     models().value(u"fast-draft"_s).value(u"installed"_s).toBool(),
                                 kTimeoutMs);
        QTRY_VERIFY_WITH_TIMEOUT(std::ranges::any_of(l.progress,
                                                     [](const QJsonObject& p) {
                                                         return p.value(u"model"_s) == u"fast-model"_s &&
                                                                p.value(u"state"_s) == u"installed"_s;
                                                     }),
                                 kTimeoutMs);
        QCOMPARE(call(c, u"DownloadModel"_s, {u"nonsense"_s}).errorName(),
                 u"com.ktechpit.paraphrase.Error.InvalidInput"_s);

        // In use: not deleted.
        Listener l2;
        QVERIFY(l2.listen(c));
        const QString id = runSkill(c, u"proofread"_s, {{u"text"_s, u"Their going home."_s}});
        QTRY_VERIFY_WITH_TIMEOUT(l.done.contains(id), kTimeoutMs);
        QCOMPARE(call(c, u"DeleteModel"_s, {u"fast-model"_s}).errorName(),
                 u"com.ktechpit.paraphrase.Error.Busy"_s);
    }

    void deletesAModelNotInUse()
    {
        startServer(32, {u"fast-model"_s, u"fast-draft"_s});
        QDBusConnection c = connect();
        QCOMPARE(call(c, u"DeleteModel"_s, {u"fast-model"_s}).type(), QDBusMessage::ReplyMessage);
        QVERIFY(!QFile::exists(m_dir->path() + u"/models/fast-model"_s));
        QVERIFY(!QFile::exists(m_dir->path() + u"/models/fast-draft"_s));
        QCOMPARE(runSkill(c, u"proofread"_s, {{u"text"_s, u"x"_s}}),
                 u"com.ktechpit.paraphrase.Error.NoModel"_s);
    }
};

QTEST_GUILESS_MAIN(TestApi)
#include "tst_api.moc"

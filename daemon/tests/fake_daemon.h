#pragma once

// A daemon for tests: the real DaemonService on a private D-Bus socket, on its own thread,
// with a fake engine whose answers the test scripts and a tiny catalog of models that can be
// "downloaded" from a test server. Used by tst_api and the app's window tests.

#include "api/daemon_service.h"

#include <QCryptographicHash>
#include <QDBusConnection>
#include <QDBusServer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QThread>

#include <atomic>
#include <memory>

namespace paraphrase::testing {

using namespace Qt::StringLiterals;
using paraphrase::api::DaemonService;
using paraphrase::api::ServiceSetup;
using paraphrase::engine::Engine;
using paraphrase::engine::EngineConfig;
using paraphrase::engine::Generation;
using paraphrase::engine::GenerationRequest;
using paraphrase::engine::TextSink;
using paraphrase::util::Result;

/// What the fake engine did, shared with the test across threads.
struct FakeState
{
    QMutex lock;
    QStringList answers; ///< scripted, one per generate(); the default answer after
    QString defaultAnswer = u"They are going home."_s;
    int pieceDelayMs = 0; ///< per streamed word
    int loadDelayMs = 0;  ///< per model load
    QList<EngineConfig> loads;
    QStringList warmed;         ///< user prompts read ahead
    QList<int> cachedHits;      ///< per generate(): whether the prompt matched the last warm
    QStringList prompts;        ///< per generate(): the user prompt the engine was given
    QList<double> temperatures; ///< per generate(): the sampling temperature it was given
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
        int delay = 0;
        {
            const QMutexLocker lock(&m_state->lock);
            delay = m_state->loadDelayMs;
        }
        QThread::msleep(static_cast<unsigned long>(delay));
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
            m_state->prompts.append(request.user);
            m_state->temperatures.append(request.sampling.value(u"temperature"_s).toDouble());
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

inline bool writeFile(const QString& path, const QByteArray& data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    return f.open(QIODevice::WriteOnly) && f.write(data) == data.size();
}

/// A catalog of tiny models, so they can be "downloaded" into a temporary folder.
inline QByteArray tinyCatalog()
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
inline bool install(const paraphrase::models::ModelCatalog& catalog, const QString& root,
                    const QStringList& ids)
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

} // namespace paraphrase::testing

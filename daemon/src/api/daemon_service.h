#pragma once

#include "inference_worker.h"
#include "models/hardware_probe.h"
#include "models/model_catalog.h"
#include "models/model_store.h"
#include "skills/skill_registry.h"
#include "util/language_detector.h"

#include <QDBusConnection>
#include <QDBusContext>
#include <QHash>
#include <QList>
#include <QObject>
#include <QSet>
#include <QString>
#include <QThread>
#include <QTimer>
#include <QVariantMap>

namespace paraphrase::api {

/// Everything the service runs on; the daemon fills it from the system, tests with fakes.
struct ServiceSetup
{
    skills::SkillRegistry skills;
    models::ModelCatalog catalog;
    QString modelsRoot;
    models::HardwareInfo hardware;
    InferenceWorker::EngineFactory engineFactory;
    int idleUnloadMs = 10 * 60 * 1000; ///< PLAN 5.3: unload the model after 10 quiet minutes
    int idleExitMs = 30 * 60 * 1000;   ///< ADR-010: exit after a quiet period
};

/// The daemon's D-Bus API (PLAN 5.1, DOCS/api.md): `com.ktechpit.paraphrase.Daemon1` at
/// `/com/ktechpit/paraphrase/Daemon`, on the session bus or on a private peer connection
/// (ADR-010), the same object either way.
///
/// Requests queue for the inference thread, one at a time. A new request from a client cancels
/// that client's earlier ones (PLAN 5.3, "Cancel on change"). The streaming signals go only to
/// the client that asked, never broadcast: other programs on the bus do not see the text.
class DaemonService : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.ktechpit.paraphrase.Daemon1")
    Q_DISABLE_COPY_MOVE(DaemonService)

public:
    static constexpr auto kServiceName = "com.ktechpit.paraphrase.Daemon";
    static constexpr auto kObjectPath = "/com/ktechpit/paraphrase/Daemon";
    static constexpr auto kInterface = "com.ktechpit.paraphrase.Daemon1";

    explicit DaemonService(ServiceSetup setup, QObject* parent = nullptr);
    ~DaemonService() override;

    /// Exports the object on `connection` (the session bus, or a private peer).
    [[nodiscard]] bool exportOn(QDBusConnection connection);

    // D-Bus member names are PascalCase by D-Bus convention, not ours.
    // NOLINTBEGIN(readability-identifier-naming)
public Q_SLOTS:
    /// Queues a skill run; the answer comes as Token, Retrying, Done or Error signals.
    /// `input`: text, context, target_lang. Errors for bad input and a missing model are
    /// D-Bus error replies (DOCS/api.md lists them).
    Q_SCRIPTABLE QString RunSkill(const QString& skillId, const QVariantMap& input,
                                  const QVariantMap& options);
    /// Stops one of the caller's requests; it ends with Error(id, "cancelled", ...).
    Q_SCRIPTABLE void Cancel(const QString& requestId);
    /// Reads the text ahead for the action the user will likely pick (ADR-005). Best effort:
    /// never an error, dropped as soon as a real request arrives.
    Q_SCRIPTABLE void Warm(const QString& skillId, const QVariantMap& input);
    [[nodiscard]] Q_SCRIPTABLE QString ListSkills() const;
    Q_SCRIPTABLE QString Status();

Q_SIGNALS:
    // Declared for introspection; sent as targeted signals to the requesting client only.
    Q_SCRIPTABLE void Token(const QString& requestId, int variant, const QString& text);
    Q_SCRIPTABLE void Retrying(const QString& requestId, const QString& reason);
    Q_SCRIPTABLE void Done(const QString& requestId, const QString& result);
    Q_SCRIPTABLE void Error(const QString& requestId, const QString& code, const QString& message);
    // NOLINTEND(readability-identifier-naming)

    /// Nothing happened for idleExitMs: the daemon may exit (not on D-Bus).
    void idleTimeout();

private:
    struct Client
    {
        QString connection; ///< QDBusConnection name
        QString service;    ///< unique bus name; empty on a peer connection
        [[nodiscard]] bool operator==(const Client&) const = default;
    };

    [[nodiscard]] Client caller() const;
    void sendTo(const QString& requestId, const char* signal, const QVariantList& args);
    void replyError(const QString& name, const QString& message);
    /// Checks the input and builds the skill's values; an error message when it is wrong.
    [[nodiscard]] QString buildJob(const QString& skillId, const QVariantMap& input, Job& job) const;
    [[nodiscard]] QSet<QString> installedModels() const;
    /// The model for the job's tier (TierSelector), with its draft head when installed.
    [[nodiscard]] bool chooseModel(Job& job) const;
    void dispatchNext();
    void cancelClientRequests(const Client& client);
    void finishRequest(const QString& id);
    void onIdle();
    void restartIdleTimers();

    void onToken(const QString& id, int variant, const QString& text);
    void onRetrying(const QString& id, const QString& reason);
    void onFinished(const QString& id, const QString& result);
    void onFailed(const QString& id, const QString& code, const QString& message);

    ServiceSetup m_setup;
    util::LanguageDetector m_detector;
    models::ModelStore m_store;
    int m_maxVariants = 1;

    QThread m_thread;
    InferenceWorker* m_worker; ///< lives on m_thread, deleted when it finishes
    QList<Job> m_queue;
    QString m_running; ///< id of the job on the inference thread, empty when idle
    bool m_busy = false;
    QHash<QString, Client> m_clients; ///< open requests
    QString m_loadedModel;

    QTimer m_unloadTimer;
    QTimer m_exitTimer;
};

} // namespace paraphrase::api

#pragma once

#include <QDBusConnection>
#include <QDBusMessage>
#include <QObject>
#include <QProcess>
#include <QSet>
#include <QString>
#include <QVariantList>

#include <memory>

namespace paraphrase::client {

/// A connection to paraphrase-daemon (DOCS/api.md), found or started as ADR-010 says: the
/// daemon on the session bus; else one started detached, once its name appears; else (no bus,
/// or the name may not be owned) a private daemon run as this process's child on a 0600 Unix
/// socket. The same calls and signals either way. Shared by the CLI and the app.
class DaemonClient : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(DaemonClient)

public:
    explicit DaemonClient(QObject* parent = nullptr);
    ~DaemonClient() override;

    /// Empty on success, else a message for the user. $PARAPHRASE_DAEMON_ADDRESS (a D-Bus
    /// peer address) connects to a daemon that already serves privately, for tests.
    [[nodiscard]] QString connectToDaemon();
    [[nodiscard]] bool isPrivate() const { return m_private != nullptr; }

    /// A blocking call; the reply, or an error message whose name says what went wrong.
    /// Requests that RunSkill starts are tracked: if the daemon goes away before they end, each
    /// gets error(id, "daemon-gone", ...) so no caller waits for an answer that cannot come.
    [[nodiscard]] QDBusMessage call(const QString& method, const QVariantList& args);

Q_SIGNALS:
    void token(const QString& requestId, int variant, const QString& text);
    void retrying(const QString& requestId, const QString& reason);
    void done(const QString& requestId, const QString& result);
    void error(const QString& requestId, const QString& code, const QString& message);
    void downloadProgress(const QString& status);
    void skillsChanged(); ///< an action was saved or deleted: ListSkills again

private Q_SLOTS:
    // QtDBus delivers signals to slots named in strings, so these forward to the signals above.
    void onToken(const QString& requestId, int variant, const QString& text);
    void onRetrying(const QString& requestId, const QString& reason);
    void onDone(const QString& requestId, const QString& result);
    void onError(const QString& requestId, const QString& code, const QString& message);
    void onDownloadProgress(const QString& status);
    void onSkillsChanged();
    void onDisconnected(); ///< the private or peer connection closed

private:
    /// Ends every request still running with a daemon-gone error.
    void daemonLost();
    void watchDaemon();

    [[nodiscard]] static QString daemonProgram();
    [[nodiscard]] QString startPrivately();
    /// Starts the daemon on the session bus unless it already owns its name; waits for it.
    static void startOnBus();
    /// Before a call: starts the daemon again if it quit while idle.
    void keepRunning();
    [[nodiscard]] bool subscribe();
    /// Drops the daemon's signals, before the connection closes.
    void unsubscribe();

    QDBusConnection m_connection;
    std::unique_ptr<QProcess> m_private; ///< the private daemon, when there is one
    QString m_socketPath;
    bool m_peer = false;       ///< connected to $PARAPHRASE_DAEMON_ADDRESS, a daemon someone else runs
    bool m_subscribed = false; ///< listening to the daemon: connected, so connecting again does nothing
    QSet<QString> m_running;   ///< ids from RunSkill without a Done or Error yet
};

} // namespace paraphrase::client

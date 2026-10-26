#include "daemon_client.h"

#include <QCoreApplication>
#include <QDBusConnectionInterface>
#include <QDBusServiceWatcher>
#include <QDeadlineTimer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QThread>

#include <utility>

using namespace Qt::StringLiterals;

namespace paraphrase::client {
namespace {

const QString kService = u"com.ktechpit.paraphrase.Daemon"_s;
const QString kPath = u"/com/ktechpit/paraphrase/Daemon"_s;
const QString kInterface = u"com.ktechpit.paraphrase.Daemon1"_s;
const QString kPrivateName = u"paraphrase-private"_s;
constexpr int kStartTimeoutMs = 15'000;
constexpr int kPollMs = 50;

/// Runs the event loop for a moment while waiting for something outside it.
void pause()
{
    QCoreApplication::processEvents(QEventLoop::AllEvents, kPollMs);
    QThread::msleep(kPollMs);
}

} // namespace

DaemonClient::DaemonClient(QObject* parent)
    : QObject(parent)
    , m_connection(u"none"_s)
{}

DaemonClient::~DaemonClient()
{
    // Stop the daemon's signals before the connection closes: closing it delivers one more
    // ("Disconnected"), and a delivery queued for this half-destroyed client crashed in Qt
    // (tst_cli, about 1 run in 30). Queued ones are dropped while the connection still lives.
    unsubscribe();
    if (m_private) {
        QObject::disconnect(m_private.get(), nullptr, this, nullptr); // its exit is expected now
    }
    QCoreApplication::removePostedEvents(this);
    if (m_peer) {
        QDBusConnection::disconnectFromPeer(kPrivateName);
    }
    if (m_private) {
        QDBusConnection::disconnectFromPeer(kPrivateName);
        m_private->terminate();
        m_private->waitForFinished(3000);
        QFile::remove(m_socketPath);
    }
}

QString DaemonClient::daemonProgram()
{
    // Packages install both programs side by side; development builds know the build tree.
    const QStringList candidates = {
        qEnvironmentVariable("PARAPHRASE_DAEMON"),
        QCoreApplication::applicationDirPath() + u"/paraphrase-daemon"_s,
        QStringLiteral(PARAPHRASE_BUILD_DAEMON),
        QStandardPaths::findExecutable(u"paraphrase-daemon"_s),
    };
    for (const QString& path : candidates) {
        if (!path.isEmpty() && QFileInfo(path).isExecutable()) {
            return path;
        }
    }
    return {};
}

QString DaemonClient::connectToDaemon()
{
    // Several parts of the app share one client: the first one connects, the others reuse it,
    // so there is one daemon, one subscription and one private daemon at most.
    if (m_subscribed && m_connection.isConnected()) {
        return {};
    }
    // A daemon already serving on a private socket (tests, and a debugging session).
    if (const QString address = qEnvironmentVariable("PARAPHRASE_DAEMON_ADDRESS"); !address.isEmpty()) {
        m_connection = QDBusConnection::connectToPeer(address, kPrivateName);
        if (!m_connection.isConnected()) {
            return tr("Cannot connect to the daemon: %1").arg(m_connection.lastError().message());
        }
        m_peer = true;
        return subscribe() ? QString() : tr("Cannot listen to the daemon.");
    }
    const bool forcePrivate = qEnvironmentVariable("PARAPHRASE_PRIVATE") == u"1"_s;
    const QDBusConnection bus = QDBusConnection::sessionBus();
    if (!forcePrivate && bus.isConnected()) {
        const QDBusConnectionInterface* names = bus.interface();
        if (!names->isServiceRegistered(kService) && daemonProgram().isEmpty()) {
            return tr("paraphrase-daemon was not found; set PARAPHRASE_DAEMON to its path.");
        }
        startOnBus();
        if (names->isServiceRegistered(kService)) {
            m_connection = bus;
            return subscribe() ? QString() : tr("Cannot listen to the daemon on the session bus.");
        }
        // It could not own its name (a snap waiting for its name declaration): run it privately.
    }
    return startPrivately();
}

void DaemonClient::startOnBus()
{
    const QDBusConnection bus = QDBusConnection::sessionBus();
    const QDBusConnectionInterface* names = bus.interface();
    if (names->isServiceRegistered(kService)) {
        return;
    }
    const QString program = daemonProgram();
    if (program.isEmpty()) {
        return;
    }
    // Its logs must not land in this terminal, in the middle of an answer.
    QProcess daemon;
    daemon.setProgram(program);
    daemon.setStandardOutputFile(QProcess::nullDevice());
    daemon.setStandardErrorFile(QProcess::nullDevice());
    daemon.startDetached();
    const QDeadlineTimer deadline(kStartTimeoutMs);
    while (!names->isServiceRegistered(kService) && !deadline.hasExpired()) {
        pause();
    }
}

void DaemonClient::keepRunning()
{
    // The daemon quits after a long quiet spell (--idle-exit, 30 minutes); the app stays. A
    // call after that starts it again instead of failing ("Paraphrase is not running", live
    // check 2026-10-25). The signals follow the bus name to the new daemon.
    if (!m_subscribed || m_peer) {
        return;
    }
    if (m_private) {
        if (m_private->state() == QProcess::NotRunning) {
            m_subscribed = false;
            m_private = nullptr;
            QDBusConnection::disconnectFromPeer(kPrivateName); // else the dead socket's connection is reused
            (void)startPrivately();
        }
        return;
    }
    startOnBus();
}

QString DaemonClient::startPrivately()
{
    const QString program = daemonProgram();
    if (program.isEmpty()) {
        return tr("paraphrase-daemon was not found; set PARAPHRASE_DAEMON to its path.");
    }
    QString runtime = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (runtime.isEmpty()) {
        runtime = QDir::tempPath();
    }
    const QString dir = runtime + u"/paraphrase"_s;
    QDir().mkpath(dir);
    QFile::setPermissions(dir, QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    m_socketPath = dir + u"/cli-%1.sock"_s.arg(QCoreApplication::applicationPid());
    QFile::remove(m_socketPath);

    m_private = std::make_unique<QProcess>();
    m_private->setStandardOutputFile(QProcess::nullDevice());
    m_private->setProcessChannelMode(QProcess::SeparateChannels);
    m_private->start(program, {u"--private-socket"_s, m_socketPath, u"--idle-exit"_s, u"600"_s});
    const QDeadlineTimer deadline(kStartTimeoutMs);
    while (!QFileInfo::exists(m_socketPath) && m_private->state() != QProcess::NotRunning &&
           !deadline.hasExpired()) {
        pause();
    }
    if (!QFileInfo::exists(m_socketPath)) {
        const QString why = QString::fromUtf8(m_private->readAllStandardError()).trimmed().section(u'\n', -1);
        return tr("The daemon did not start: %1").arg(why.isEmpty() ? tr("no answer") : why);
    }
    m_connection = QDBusConnection::connectToPeer(u"unix:path="_s + m_socketPath, kPrivateName);
    if (!m_connection.isConnected()) {
        return tr("Cannot connect to the daemon: %1").arg(m_connection.lastError().message());
    }
    return subscribe() ? QString() : tr("Cannot listen to the daemon.");
}

bool DaemonClient::subscribe()
{
    // On the bus only the daemon's own signals: they are addressed to this connection anyway.
    const QString service = (m_private || m_peer) ? QString() : kService;
    m_subscribed =
        m_connection.connect(service, kPath, kInterface, u"Token"_s, this,
                             SLOT(onToken(QString, int, QString))) &&
        m_connection.connect(service, kPath, kInterface, u"Retrying"_s, this,
                             SLOT(onRetrying(QString, QString))) &&
        m_connection.connect(service, kPath, kInterface, u"Done"_s, this, SLOT(onDone(QString, QString))) &&
        m_connection.connect(service, kPath, kInterface, u"Error"_s, this,
                             SLOT(onError(QString, QString, QString))) &&
        m_connection.connect(service, kPath, kInterface, u"DownloadProgress"_s, this,
                             SLOT(onDownloadProgress(QString))) &&
        m_connection.connect(service, kPath, kInterface, u"SkillsChanged"_s, this, SLOT(onSkillsChanged()));
    if (m_subscribed) {
        watchDaemon();
    }
    return m_subscribed;
}

void DaemonClient::unsubscribe()
{
    if (!m_subscribed) {
        return;
    }
    m_subscribed = false;
    const QString service = (m_private || m_peer) ? QString() : kService;
    m_connection.disconnect(service, kPath, kInterface, u"Token"_s, this,
                            SLOT(onToken(QString, int, QString)));
    m_connection.disconnect(service, kPath, kInterface, u"Retrying"_s, this,
                            SLOT(onRetrying(QString, QString)));
    m_connection.disconnect(service, kPath, kInterface, u"Done"_s, this, SLOT(onDone(QString, QString)));
    m_connection.disconnect(service, kPath, kInterface, u"Error"_s, this,
                            SLOT(onError(QString, QString, QString)));
    m_connection.disconnect(service, kPath, kInterface, u"DownloadProgress"_s, this,
                            SLOT(onDownloadProgress(QString)));
    m_connection.disconnect(service, kPath, kInterface, u"SkillsChanged"_s, this, SLOT(onSkillsChanged()));
    if (m_private || m_peer) {
        m_connection.disconnect(QString(), u"/org/freedesktop/DBus/Local"_s, u"org.freedesktop.DBus.Local"_s,
                                u"Disconnected"_s, this, SLOT(onDisconnected()));
    }
}

void DaemonClient::watchDaemon()
{
    // A daemon can die mid-request (a crash, a kill, a rebuild); without this, a caller would
    // wait for its Done forever.
    if (m_private || m_peer) {
        m_connection.connect(QString(), u"/org/freedesktop/DBus/Local"_s, u"org.freedesktop.DBus.Local"_s,
                             u"Disconnected"_s, this, SLOT(onDisconnected()));
    } else {
        const auto* watcher = new QDBusServiceWatcher(kService, m_connection,
                                                      QDBusServiceWatcher::WatchForUnregistration, this);
        connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, &DaemonClient::daemonLost);
    }
    if (m_private) {
        connect(m_private.get(), &QProcess::finished, this, &DaemonClient::daemonLost);
    }
}

void DaemonClient::onDisconnected()
{
    daemonLost();
}

void DaemonClient::daemonLost()
{
    const QSet<QString> running = std::exchange(m_running, {});
    for (const QString& id : running) {
        Q_EMIT error(id, u"daemon-gone"_s, tr("The daemon stopped before it answered."));
    }
}

QDBusMessage DaemonClient::call(const QString& method, const QVariantList& args)
{
    keepRunning();
    QDBusMessage msg = QDBusMessage::createMethodCall((m_private || m_peer) ? QString() : kService, kPath,
                                                      kInterface, method);
    msg.setArguments(args);
    QDBusMessage reply = m_connection.call(msg, QDBus::Block, 60'000);
    if (method == u"RunSkill"_s && reply.type() == QDBusMessage::ReplyMessage) {
        m_running.insert(reply.arguments().value(0).toString());
    }
    return reply;
}

void DaemonClient::onToken(const QString& requestId, int variant, const QString& text)
{
    Q_EMIT token(requestId, variant, text);
}

void DaemonClient::onRetrying(const QString& requestId, const QString& reason)
{
    Q_EMIT retrying(requestId, reason);
}

void DaemonClient::onDone(const QString& requestId, const QString& result)
{
    m_running.remove(requestId);
    Q_EMIT done(requestId, result);
}

void DaemonClient::onError(const QString& requestId, const QString& code, const QString& message)
{
    m_running.remove(requestId);
    Q_EMIT error(requestId, code, message);
}

void DaemonClient::onSkillsChanged()
{
    Q_EMIT skillsChanged();
}

void DaemonClient::onDownloadProgress(const QString& status)
{
    Q_EMIT downloadProgress(status);
}

} // namespace paraphrase::client

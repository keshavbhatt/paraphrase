#include "daemon_client.h"

#include <QCoreApplication>
#include <QDBusConnectionInterface>
#include <QDeadlineTimer>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QStandardPaths>
#include <QThread>

using namespace Qt::StringLiterals;

namespace paraphrase::cli {
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
    const bool forcePrivate = qEnvironmentVariable("PARAPHRASE_PRIVATE") == u"1"_s;
    const QDBusConnection bus = QDBusConnection::sessionBus();
    if (!forcePrivate && bus.isConnected()) {
        const QDBusConnectionInterface* names = bus.interface();
        if (!names->isServiceRegistered(kService)) {
            const QString program = daemonProgram();
            if (program.isEmpty()) {
                return tr("paraphrase-daemon was not found; set PARAPHRASE_DAEMON to its path.");
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
        if (names->isServiceRegistered(kService)) {
            m_connection = bus;
            return subscribe() ? QString() : tr("Cannot listen to the daemon on the session bus.");
        }
        // It could not own its name (a snap waiting for its name declaration): run it privately.
    }
    return startPrivately();
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
    const QString service = m_private ? QString() : kService;
    return m_connection.connect(service, kPath, kInterface, u"Token"_s, this,
                                SLOT(onToken(QString, int, QString))) &&
           m_connection.connect(service, kPath, kInterface, u"Retrying"_s, this,
                                SLOT(onRetrying(QString, QString))) &&
           m_connection.connect(service, kPath, kInterface, u"Done"_s, this,
                                SLOT(onDone(QString, QString))) &&
           m_connection.connect(service, kPath, kInterface, u"Error"_s, this,
                                SLOT(onError(QString, QString, QString))) &&
           m_connection.connect(service, kPath, kInterface, u"DownloadProgress"_s, this,
                                SLOT(onDownloadProgress(QString)));
}

QDBusMessage DaemonClient::call(const QString& method, const QVariantList& args)
{
    QDBusMessage msg =
        QDBusMessage::createMethodCall(m_private ? QString() : kService, kPath, kInterface, method);
    msg.setArguments(args);
    return m_connection.call(msg, QDBus::Block, 60'000);
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
    Q_EMIT done(requestId, result);
}

void DaemonClient::onError(const QString& requestId, const QString& code, const QString& message)
{
    Q_EMIT error(requestId, code, message);
}

void DaemonClient::onDownloadProgress(const QString& status)
{
    Q_EMIT downloadProgress(status);
}

} // namespace paraphrase::cli

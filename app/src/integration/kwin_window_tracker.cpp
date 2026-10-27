#include "kwin_window_tracker.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDeadlineTimer>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

const QString kKWin = u"org.kde.KWin"_s;
constexpr int kFirstReportMs = 1000;

QDBusMessage scripting(const QString& method)
{
    return QDBusMessage::createMethodCall(kKWin, u"/Scripting"_s, u"org.kde.kwin.Scripting"_s, method);
}

} // namespace

KWinWindowTracker::KWinWindowTracker(QObject* parent)
    : QObject(parent)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.interface()->isServiceRegistered(kKWin).value() ||
        !bus.registerObject(QString::fromLatin1(kObjectPath), this, QDBusConnection::ExportScriptableSlots)) {
        return;
    }
    m_loaded = load();
    // The script reports the active window as soon as it runs.
    const QDeadlineTimer deadline(kFirstReportMs);
    while (m_loaded && m_active.isEmpty() && !deadline.hasExpired()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    }
}

KWinWindowTracker::~KWinWindowTracker()
{
    if (m_loaded) {
        unload();
    }
    QDBusConnection::sessionBus().unregisterObject(QString::fromLatin1(kObjectPath));
}

QString KWinWindowTracker::script(const QString& service)
{
    // internalId is a stable UUID per window; resourceClass names the app ("org.telegram.desktop").
    return uR"(function report() {
    const w = workspace.activeWindow;
    callDBus("%1", "%2", "com.ktechpit.paraphrase.WindowTracker", "activated",
             w ? w.internalId.toString() : "", w ? String(w.resourceClass) : "");
}
workspace.windowActivated.connect(report);
report();
)"_s.arg(service, QString::fromLatin1(kObjectPath));
}

QString KWinWindowTracker::scriptPath()
{
    // KWin reads the script from a path, so it must be one KWin sees: a snap has its own /tmp,
    // but its runtime folder (/run/user/<uid>/snap.<name>) is a real one. A Flatpak's runtime
    // folder is private except app/<app id>, which the host sees at the same path. One file,
    // overwritten each run: a killed run leaves nothing behind.
    QString dir = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (const QString flatpak = qEnvironmentVariable("FLATPAK_ID"); !flatpak.isEmpty() && !dir.isEmpty()) {
        dir += u"/app/"_s + flatpak;
    }
    if (dir.isEmpty() || !QDir().mkpath(dir)) {
        return {};
    }
    return dir + u"/paraphrase-window-tracker.js"_s;
}

bool KWinWindowTracker::load()
{
    const QString path = scriptPath();
    if (path.isEmpty()) {
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly) ||
        file.write(script(QDBusConnection::sessionBus().baseService()).toUtf8()) < 0) {
        return false;
    }
    file.close();
    unload(); // a copy left by an earlier run that did not end cleanly
    QDBusMessage call = scripting(u"loadScript"_s);
    call << path << QString::fromLatin1(kPluginName);
    const QDBusMessage reply = QDBusConnection::sessionBus().call(call, QDBus::Block, 2000);
    const int id = reply.arguments().value(0, -1).toInt();
    if (reply.type() != QDBusMessage::ReplyMessage || id < 0) {
        return false;
    }
    const QDBusMessage run = QDBusMessage::createMethodCall(kKWin, u"/Scripting/Script%1"_s.arg(id),
                                                            u"org.kde.kwin.Script"_s, u"run"_s);
    return QDBusConnection::sessionBus().call(run, QDBus::Block, 2000).type() == QDBusMessage::ReplyMessage;
}

void KWinWindowTracker::unload()
{
    QDBusMessage call = scripting(u"unloadScript"_s);
    call << QString::fromLatin1(kPluginName);
    QDBusConnection::sessionBus().call(call, QDBus::Block, 2000);
}

void KWinWindowTracker::activated(const QString& window, const QString& app)
{
    m_active = window;
    m_app = app;
}

bool KWinWindowTracker::waitUntilActive(const QString& window, int timeoutMs)
{
    const QDeadlineTimer deadline(timeoutMs);
    while (m_active != window && !deadline.hasExpired()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
    return m_active == window && !window.isEmpty();
}

} // namespace paraphrase::app

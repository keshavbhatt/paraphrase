#include "ui/core/autostart.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCall>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

const QString kFile = u"com.ktechpit.paraphrase.desktop"_s;
const QString kFlatpakKey = u"general/startAtLogin"_s;

QString entryPath()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + u"/autostart/"_s + kFile;
}

bool inFlatpak()
{
    return !qEnvironmentVariable("FLATPAK_ID").isEmpty();
}

/// The Background portal: the sandbox's way to ask for login start.
void requestBackground(bool on)
{
    QDBusMessage call = QDBusMessage::createMethodCall(
        u"org.freedesktop.portal.Desktop"_s, u"/org/freedesktop/portal/desktop"_s,
        u"org.freedesktop.portal.Background"_s, u"RequestBackground"_s);
    const QVariantMap options{
        {u"reason"_s, QObject::tr("Start Paraphrase when you log in, so the shortcut works at once.")},
        {u"autostart"_s, on},
        {u"commandline"_s, QStringList{u"paraphrase-app"_s, u"--hidden"_s}},
    };
    call << QString() << options;
    QDBusConnection::sessionBus().asyncCall(call);
}

} // namespace

bool Autostart::enabled()
{
    if (inFlatpak()) {
        return QSettings().value(kFlatpakKey, false).toBool();
    }
    return QFile::exists(entryPath());
}

void Autostart::setEnabled(bool on)
{
    if (inFlatpak()) {
        QSettings().setValue(kFlatpakKey, on);
        requestBackground(on);
        return;
    }
    if (!on) {
        QFile::remove(entryPath());
        return;
    }
    QDir().mkpath(QFileInfo(entryPath()).absolutePath());
    QSaveFile file(entryPath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write("[Desktop Entry]\nType=Application\nName=Paraphrase\nExec=paraphrase-app --hidden\n"
                   "Icon=com.ktechpit.paraphrase\nX-GNOME-Autostart-enabled=true\nNoDisplay=true\n");
        (void)file.commit();
    }
}

} // namespace paraphrase::app

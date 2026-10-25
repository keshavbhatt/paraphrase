#include "ui/core/preferences.h"

#include "ui/core/autostart.h"

#include <QSettings>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

const QString kKeepInTray = u"general/keepInTray"_s;
const QString kAppearance = u"general/appearance"_s;
const QString kShortcutOpens = u"shortcut/opens"_s;

} // namespace

bool Preferences::startAtLogin()
{
    return Autostart::enabled();
}

void Preferences::setStartAtLogin(bool on)
{
    Autostart::setEnabled(on);
    Q_EMIT startAtLoginChanged(on);
}

bool Preferences::keepInTray()
{
    return QSettings().value(kKeepInTray, true).toBool();
}

void Preferences::setKeepInTray(bool on)
{
    QSettings().setValue(kKeepInTray, on);
    Q_EMIT keepInTrayChanged(on);
}

QString Preferences::appearance()
{
    const QString value = QSettings().value(kAppearance, u"system"_s).toString();
    return value == u"light"_s || value == u"dark"_s ? value : u"system"_s;
}

void Preferences::setAppearance(const QString& appearance)
{
    QSettings().setValue(kAppearance, appearance);
    Q_EMIT appearanceChanged();
}

bool Preferences::shortcutOpensWindow()
{
    return QSettings().value(kShortcutOpens, u"popup"_s).toString() == u"window"_s;
}

void Preferences::setShortcutOpensWindow(bool window)
{
    QSettings().setValue(kShortcutOpens, window ? u"window"_s : u"popup"_s);
}

} // namespace paraphrase::app

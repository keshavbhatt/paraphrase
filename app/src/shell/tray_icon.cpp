#include "shell/tray_icon.h"

#include <QMenu>
#include <QSystemTrayIcon>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

TrayIcon::TrayIcon(QObject* parent)
    : QObject(parent)
    , m_tray(new QSystemTrayIcon(this))
{
    auto* menu = new QMenu;
    menu->addAction(tr("Open Paraphrase"), this, &TrayIcon::windowRequested);
    menu->addAction(tr("Work on the selected text"), this, &TrayIcon::popupRequested);
    menu->addSeparator();
    menu->addAction(tr("Settings"), this, &TrayIcon::settingsRequested);
    menu->addAction(tr("Quit"), this, &TrayIcon::quitRequested);
    m_tray->setContextMenu(menu);
    m_tray->setToolTip(tr("Paraphrase: writing help on this device"));
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            Q_EMIT windowRequested();
        }
    });
    m_tray->show();
}

TrayIcon::~TrayIcon()
{
    delete m_tray->contextMenu();
}

bool TrayIcon::available()
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void TrayIcon::applyTheme(const Tokens& t)
{
    m_tray->setIcon(Theme::icon(u"pen"_s, t.accent, 64));
}

} // namespace paraphrase::app

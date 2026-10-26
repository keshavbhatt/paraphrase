#include "shell/tray_icon.h"

#include <QAction>
#include <QIcon>
#include <QMenu>
#include <QSystemTrayIcon>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {
constexpr int kMenuIconSize = 16;
} // namespace

TrayIcon::TrayIcon(QObject* parent)
    : QObject(parent)
    , m_tray(new QSystemTrayIcon(this))
{
    auto* menu = new QMenu;
    // An icon per item, as in the app's menus (menus.html). The desktop draws this menu in
    // its own theme, so its icons come first; ours, in the muted colour, where it has none.
    const auto item = [this, menu](const QString& text, auto signal, const QString& themed,
                                   const QString& own) {
        QAction* action = menu->addAction(text, this, signal);
        m_icons.append({action, {themed, own}});
    };
    item(tr("Open Paraphrase"), &TrayIcon::windowRequested, u"document-edit"_s, u"pen"_s);
    item(tr("Work on the selected text"), &TrayIcon::popupRequested, u"edit-select-text"_s,
         u"text-cursor-input"_s);
    menu->addSeparator();
    item(tr("Settings"), &TrayIcon::settingsRequested, u"configure"_s, u"settings"_s);
    item(tr("Quit"), &TrayIcon::quitRequested, u"application-exit"_s, u"power"_s);
    m_tray->setContextMenu(menu);
    m_tray->setToolTip(tr("Paraphrase: writing help on this device"));
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            Q_EMIT windowRequested();
        }
    });
    applyTheme(Theme::current()); // an icon before it shows: none set made Qt warn
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
    for (const auto& [action, names] : std::as_const(m_icons)) {
        action->setIcon(QIcon::fromTheme(names.first, Theme::icon(names.second, t.muted, kMenuIconSize)));
    }
}

} // namespace paraphrase::app

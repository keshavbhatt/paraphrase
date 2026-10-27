#include "shell/application.h"

#include "daemon_client.h"
#include "history/history_keeper.h"
#include "integration/integrations.h"
#include "popup/popup_controller.h"
#include "shell/app_flow.h"
#include "shell/app_service.h"
#include "shell/desktop_description.h"
#include "shell/tray_icon.h"
#include "ui/core/preferences.h"
#include "ui/main/main_window.h"

#include <QApplication>

namespace paraphrase::app {

Application::Application(QObject* parent)
    : QObject(parent)
    , m_client(std::make_unique<client::DaemonClient>())
    , m_integrations(std::make_unique<Integrations>(Integrations::forThisSession(this)))
    , m_flow(new AppFlow(*m_client, this))
    , m_popup(new PopupController(*m_integrations, m_client.get(), this))
    , m_service(new AppService(this))
    , m_preferences(new Preferences(this))
    , m_history(new history::Keeper(this))
{
    m_flow->setHistory(m_history);
    m_popup->setHistory(m_history);
    applyKeepInTray();
    describe();
    wire();
    // At login the app may start before the panel's tray: it gets one when the tray turns up.
    TrayIcon::whenAvailable(this, [this] { addTray(); });
}

Application::~Application() = default;

void Application::applyKeepInTray()
{
    QApplication::setQuitOnLastWindowClosed(m_tray == nullptr || !Preferences::keepInTray());
}

void Application::describe()
{
    // What Settings and setup say about this desktop, the shortcut's keys among it.
    m_flow->setSettingsContext(m_preferences, m_tray != nullptr, describeDesktop(*m_integrations));
}

void Application::retheme()
{
    for (QWidget* w : QApplication::topLevelWidgets()) {
        QEvent change(QEvent::ThemeChange);
        QCoreApplication::sendEvent(w, &change);
    }
}

void Application::wire()
{
    connect(m_preferences, &Preferences::keepInTrayChanged, this, &Application::applyKeepInTray);
    connect(m_preferences, &Preferences::appearanceChanged, this, [this] {
        retheme();
        if (m_tray != nullptr) {
            m_tray->applyTheme(Theme::current());
        }
    });
    connect(m_service, &AppService::popupRequested, this, &Application::openFromShortcut);
    connect(m_service, &AppService::windowRequested, m_flow, &AppFlow::showMain);
    connect(&m_integrations->shortcut(), &ShortcutBackend::activated, this, &Application::openFromShortcut);
    connect(&m_integrations->shortcut(), &ShortcutBackend::keysChanged, this, &Application::describe);
    connect(m_popup, &PopupController::newActionRequested, m_flow, &AppFlow::newAction);
    connect(m_popup, &PopupController::openInWindow, this, [this](const Handover& handover) {
        m_flow->showMain();
        m_flow->mainWindow().takeOver(handover);
    });
}

void Application::addTray()
{
    if (m_tray != nullptr) {
        return;
    }
    m_tray = new TrayIcon(this);
    m_tray->applyTheme(Theme::current());
    connect(m_tray, &TrayIcon::windowRequested, m_flow, &AppFlow::showMain);
    connect(m_tray, &TrayIcon::popupRequested, m_popup, &PopupController::show);
    connect(m_tray, &TrayIcon::settingsRequested, m_flow, &AppFlow::showSettings);
    connect(m_tray, &TrayIcon::quitRequested, qApp, &QApplication::quit);
    applyKeepInTray(); // closing the window now leaves the app in the tray
    describe();        // Settings offers "keep running in the tray"
}

void Application::openFromShortcut()
{
    // Settings, Shortcut: the popup over the app, or the window with the selected text in it.
    if (!Preferences::shortcutOpensWindow()) {
        m_popup->show();
        return;
    }
    const auto captured = m_integrations->capture().capture();
    m_flow->showMain();
    if (captured && !captured->text.trimmed().isEmpty()) {
        m_flow->mainWindow().takeText(captured->text);
    }
}

void Application::start(bool show)
{
    (void)m_integrations->shortcut().bind(QKeySequence(QStringLiteral("Ctrl+Alt+Space")));
    m_flow->setGlobalShortcut(m_integrations->hasGlobalShortcut());
    // Without a tray the window is the app, so it always shows.
    m_flow->start(show || m_tray == nullptr);
}

} // namespace paraphrase::app

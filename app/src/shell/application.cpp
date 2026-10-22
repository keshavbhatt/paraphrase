#include "shell/application.h"

#include "daemon_client.h"
#include "integration/integrations.h"
#include "popup/popup_controller.h"
#include "shell/app_flow.h"
#include "shell/app_service.h"
#include "shell/tray_icon.h"
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
{
    if (TrayIcon::available()) {
        m_tray = new TrayIcon(this);
        m_tray->applyTheme(Theme::current());
        // With a tray the app keeps running for the shortcut when the window closes.
        QApplication::setQuitOnLastWindowClosed(false);
    }
    wire();
}

Application::~Application() = default;

void Application::wire()
{
    connect(m_service, &AppService::popupRequested, m_popup, &PopupController::show);
    connect(m_service, &AppService::windowRequested, m_flow, &AppFlow::showMain);
    connect(&m_integrations->shortcut(), &ShortcutBackend::activated, m_popup, &PopupController::show);
    connect(m_popup, &PopupController::newActionRequested, m_flow, &AppFlow::newAction);
    connect(m_popup, &PopupController::openInWindow, this, [this](const QString& text) {
        m_flow->showMain();
        m_flow->mainWindow().takeText(text);
    });
    if (m_tray != nullptr) {
        connect(m_tray, &TrayIcon::windowRequested, m_flow, &AppFlow::showMain);
        connect(m_tray, &TrayIcon::popupRequested, m_popup, &PopupController::show);
        connect(m_tray, &TrayIcon::settingsRequested, m_flow, &AppFlow::showSettings);
        connect(m_tray, &TrayIcon::quitRequested, qApp, &QApplication::quit);
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

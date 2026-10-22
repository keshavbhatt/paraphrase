#include "shell/application.h"

#include "integration/integrations.h"
#include "popup/popup_controller.h"
#include "shell/app_service.h"
#include "shell/tray_icon.h"
#include "ui/main_window.h"

#include <QApplication>

namespace paraphrase::app {

Application::Application(QObject* parent)
    : QObject(parent)
    , m_integrations(std::make_unique<Integrations>(Integrations::forThisSession(this)))
    , m_window(std::make_unique<MainWindow>())
    , m_popup(new PopupController(*m_integrations, this))
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
    connect(m_service, &AppService::windowRequested, this, &Application::showWindow);
    connect(&m_integrations->shortcut(), &ShortcutBackend::activated, m_popup, &PopupController::show);
    connect(m_popup, &PopupController::openInWindow, this, [this](const QString& text) {
        showWindow();
        m_window->takeText(text);
    });
    if (m_tray != nullptr) {
        connect(m_tray, &TrayIcon::windowRequested, this, &Application::showWindow);
        connect(m_tray, &TrayIcon::popupRequested, m_popup, &PopupController::show);
        connect(m_tray, &TrayIcon::settingsRequested, m_window.get(), &MainWindow::settingsRequested);
        connect(m_tray, &TrayIcon::quitRequested, qApp, &QApplication::quit);
    }
}

void Application::start(bool show)
{
    (void)m_integrations->shortcut().bind(QKeySequence(QStringLiteral("Ctrl+Alt+Space")));
    m_window->setGlobalShortcut(m_integrations->hasGlobalShortcut());
    if (show || m_tray == nullptr) {
        showWindow();
    }
    // Connect after the window is up: starting the daemon can take a moment.
    QMetaObject::invokeMethod(m_window.get(), &MainWindow::start, Qt::QueuedConnection);
}

void Application::showWindow()
{
    m_window->show();
    m_window->raise();
    m_window->activateWindow();
}

} // namespace paraphrase::app

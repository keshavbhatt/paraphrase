#pragma once

#include "ui/core/desktop_support.h"

#include <QObject>

#include <memory>

namespace paraphrase::client {
class DaemonClient;
}

namespace paraphrase::app {

class MainWindow;
class Preferences;
class SettingsWindow;
class SetupWindow;

/// Which windows the app shows: first run (no model installed) opens the setup window, then
/// the main window; Settings opens from the main window, the notice or the tray.
class AppFlow : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(AppFlow)

public:
    explicit AppFlow(client::DaemonClient& client, QObject* parent = nullptr);

    /// What Settings shows on General and Shortcut, from the app.
    void setSettingsContext(Preferences* preferences, bool hasTray, const DesktopSupport& support);
    ~AppFlow() override;

    /// Opens setup on first run, else the main window (or keeps it hidden: tray autostart).
    void start(bool showWindow = true);
    /// Brings the main window up, creating it when needed (not during setup).
    void showMain();
    void showSettings();
    /// Settings, My actions, with the editor on a new action.
    void newAction();
    /// The main window, created on first use.
    [[nodiscard]] MainWindow& mainWindow();
    /// Whether the desktop gave a global shortcut; the main window's notice depends on it.
    void setGlobalShortcut(bool available);

private:
    [[nodiscard]] bool hasModel();
    void showSetup();

    client::DaemonClient& m_client; ///< the app's one connection, for every window
    std::unique_ptr<MainWindow> m_main;
    std::unique_ptr<SetupWindow> m_setup;
    std::unique_ptr<SettingsWindow> m_settings;
    bool m_globalShortcut = false;
    Preferences* m_preferences = nullptr;
    bool m_hasTray = false;
    DesktopSupport m_support;
};

} // namespace paraphrase::app

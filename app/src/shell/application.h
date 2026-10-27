#pragma once

#include <QObject>

#include <memory>

namespace paraphrase::client {
class DaemonClient;
}

namespace paraphrase::app {

namespace history {
class Keeper;
}
class AppFlow;
class AppService;
class Integrations;
class PopupController;
class Preferences;
class TrayIcon;

/// The running app: the windows (AppFlow), the popup, the tray, the backends of this session
/// and the D-Bus service other copies forward to. It only wires them together.
class Application : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Application)

public:
    explicit Application(QObject* parent = nullptr);
    ~Application() override;

    /// Starts the app; `showWindow` false starts in the tray (login autostart).
    void start(bool showWindow);
    [[nodiscard]] AppService* service() const { return m_service; }

private:
    void wire();
    /// The tray, once the desktop has one (TrayIcon::whenAvailable).
    void addTray();
    /// The shortcut or `paraphrase-app --popup`: the popup, or the window if Settings says so.
    void openFromShortcut();
    /// With a tray and "keep running in the tray", closing the window leaves the app running.
    void applyKeepInTray();
    /// Appearance changed in Settings: every window takes the new colours.
    void describe();
    static void retheme();

    std::unique_ptr<client::DaemonClient> m_client; ///< one connection to the daemon for everything
    std::unique_ptr<Integrations> m_integrations;
    AppFlow* m_flow = nullptr;
    PopupController* m_popup = nullptr;
    TrayIcon* m_tray = nullptr;
    AppService* m_service = nullptr;
    Preferences* m_preferences = nullptr;
    history::Keeper* m_history = nullptr; ///< the opt-in history, shared by the window and the popup
};

} // namespace paraphrase::app

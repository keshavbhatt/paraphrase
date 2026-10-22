#pragma once

#include <QObject>

#include <memory>

namespace paraphrase::client {
class DaemonClient;
}

namespace paraphrase::app {

class MainWindow;
class SettingsWindow;
class SetupWindow;

/// Which windows the app shows: first run (no model installed) opens the setup window, then
/// the main window; Settings opens from the main window.
class AppFlow : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(AppFlow)

public:
    explicit AppFlow(QObject* parent = nullptr);
    ~AppFlow() override;

    void start();

private:
    [[nodiscard]] bool hasModel();
    void showSetup();
    void showMain();
    void showSettings();

    std::unique_ptr<client::DaemonClient> m_client; ///< for setup and settings
    std::unique_ptr<MainWindow> m_main;
    std::unique_ptr<SetupWindow> m_setup;
    std::unique_ptr<SettingsWindow> m_settings;
};

} // namespace paraphrase::app

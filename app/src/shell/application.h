#pragma once

#include <QObject>

#include <memory>

namespace paraphrase::app {

class AppService;
class Integrations;
class MainWindow;
class PopupController;
class TrayIcon;

/// The running app: the main window, the popup, the tray, the backends of this session and
/// the D-Bus service other copies forward to. It only wires them together.
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
    void showWindow();
    void wire();

    std::unique_ptr<Integrations> m_integrations;
    std::unique_ptr<MainWindow> m_window;
    PopupController* m_popup = nullptr;
    TrayIcon* m_tray = nullptr;
    AppService* m_service = nullptr;
};

} // namespace paraphrase::app

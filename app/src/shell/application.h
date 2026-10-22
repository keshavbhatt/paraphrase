#pragma once

#include <QObject>

#include <memory>

namespace paraphrase::app {

class AppFlow;
class AppService;
class Integrations;
class PopupController;
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

    std::unique_ptr<Integrations> m_integrations;
    AppFlow* m_flow = nullptr;
    PopupController* m_popup = nullptr;
    TrayIcon* m_tray = nullptr;
    AppService* m_service = nullptr;
};

} // namespace paraphrase::app

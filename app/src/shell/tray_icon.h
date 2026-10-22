#pragma once

#include "ui/theme.h"

#include <QObject>

class QSystemTrayIcon;

namespace paraphrase::app {

/// The tray icon, where the desktop has a tray: open the window, open the popup on the
/// selection, Settings, Quit. Without a tray the window is the app and closing it quits.
class TrayIcon : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TrayIcon)

public:
    explicit TrayIcon(QObject* parent = nullptr);
    ~TrayIcon() override;

    [[nodiscard]] static bool available();
    void applyTheme(const Tokens& t);

Q_SIGNALS:
    void windowRequested();
    void popupRequested();
    void settingsRequested();
    void quitRequested();

private:
    QSystemTrayIcon* m_tray = nullptr;
};

} // namespace paraphrase::app

#pragma once

#include "ui/common/theme.h"

#include <QList>
#include <QObject>
#include <QString>

#include <functional>
#include <utility>

class QAction;
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
    /// Calls `ready` once the desktop has a tray: now, or when its StatusNotifierWatcher and a
    /// host turn up later, as when the app starts at login before the panel. Qt checks for a
    /// tray once per run and keeps a "no", so it is asked only once the watcher is there.
    /// `watcher` names another service in place of org.kde.StatusNotifierWatcher (tests).
    static void whenAvailable(QObject* context, const std::function<void()>& ready,
                              const QString& watcher = {});
    void applyTheme(const Tokens& t);

Q_SIGNALS:
    void windowRequested();
    void popupRequested();
    void settingsRequested();
    void quitRequested();

private:
    QSystemTrayIcon* m_tray = nullptr;
    /// Each item and its icon: the desktop's own name, and ours where the theme has none.
    QList<std::pair<QAction*, std::pair<QString, QString>>> m_icons;
};

} // namespace paraphrase::app

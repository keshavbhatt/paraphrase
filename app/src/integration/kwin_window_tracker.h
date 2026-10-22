#pragma once

#include "window_tracker.h"

#include <QObject>
#include <QString>
#include <QTemporaryDir>

namespace paraphrase::app {

/// The active window on KDE Plasma (Wayland or X11), which Wayland itself never tells an app:
/// a small KWin script, loaded for the app's lifetime, reports every activation to this
/// object over D-Bus (window id and application only, never titles). Unavailable outside
/// KWin, or when a sandbox blocks org.kde.KWin; then nothing is typed (PLAN 6).
class KWinWindowTracker final : public QObject, public WindowTracker
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "com.ktechpit.paraphrase.WindowTracker")
    Q_DISABLE_COPY_MOVE(KWinWindowTracker)

public:
    static constexpr char kObjectPath[] = "/com/ktechpit/paraphrase/WindowTracker";
    static constexpr char kPluginName[] = "paraphrase-window-tracker";

    explicit KWinWindowTracker(QObject* parent = nullptr);
    ~KWinWindowTracker() override;

    [[nodiscard]] bool available() const override { return m_loaded && !m_active.isEmpty(); }
    [[nodiscard]] QString activeWindow() const override { return m_active; }
    [[nodiscard]] QString activeApp() const override { return m_app; }
    [[nodiscard]] bool waitUntilActive(const QString& window, int timeoutMs) override;

    /// The script KWin runs: reports activations to `service` (this process's bus name).
    [[nodiscard]] static QString script(const QString& service);

public Q_SLOTS:
    /// Called by the KWin script.
    Q_SCRIPTABLE void activated(const QString& window, const QString& app);

private:
    [[nodiscard]] bool load();
    static void unload();

    QTemporaryDir m_dir;
    bool m_loaded = false;
    QString m_active;
    QString m_app;
};

} // namespace paraphrase::app

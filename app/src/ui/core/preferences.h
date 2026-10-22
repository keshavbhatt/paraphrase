#pragma once

#include <QObject>
#include <QString>

namespace paraphrase::app {

/// The user's app preferences (settings-general-shortcut.html): start at login, keep running
/// in the tray, appearance. Kept in QSettings; login start goes through Autostart.
class Preferences : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Preferences)

public:
    explicit Preferences(QObject* parent = nullptr)
        : QObject(parent)
    {}
    ~Preferences() override = default;

    [[nodiscard]] static bool startAtLogin();
    void setStartAtLogin(bool on);
    [[nodiscard]] static bool keepInTray();
    void setKeepInTray(bool on);
    /// "system", "light" or "dark" (Theme::current reads it).
    [[nodiscard]] static QString appearance();
    void setAppearance(const QString& appearance);

Q_SIGNALS:
    void startAtLoginChanged(bool on);
    void keepInTrayChanged(bool on);
    void appearanceChanged();
};

} // namespace paraphrase::app

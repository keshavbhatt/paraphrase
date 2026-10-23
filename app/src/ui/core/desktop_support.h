#pragma once

#include <QList>
#include <QString>

#include <cstdint>
#include <functional>

namespace paraphrase::app {

/// What this desktop lets Paraphrase do, in plain words (settings-general-shortcut.html,
/// note 2): filled by the app from its backends, shown by Settings. No backend names.
struct DesktopSupport
{
    struct Step
    {
        QString step;   ///< "Open over any app"
        QString how;    ///< "Shortcut from the desktop"
        QString status; ///< "Works", "Asks once", "By hand", "Copy first", "You paste"
        bool works = true;
    };

    /// Whether a result can be typed back (setup-desktop.html, "Put the result back").
    enum class Typing : std::uint8_t
    {
        Ready,    ///< nothing to allow on this desktop
        NotAsked, ///< the desktop asks once
        Allowed,
        Refused,     ///< results go on the clipboard
        Unavailable, ///< no way to type here: results go on the clipboard
    };

    bool globalShortcut = false;
    Typing typing = Typing::Unavailable;
    /// Opens the desktop's own dialog and waits for the answer; empty where nothing is asked.
    std::function<Typing()> askTyping;
    QString shortcut; ///< "Ctrl+Alt+Space", or "" without a global shortcut
    QString changeIt; ///< where to change it, e.g. "System Settings, Shortcuts"
    QString command = QStringLiteral("paraphrase-app --popup");
    QList<Step> steps;
};

} // namespace paraphrase::app

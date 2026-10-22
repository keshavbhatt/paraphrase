#pragma once

#include <QList>
#include <QString>

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

    bool globalShortcut = false;
    QString shortcut; ///< "Ctrl+Alt+Space", or "" without a global shortcut
    QString changeIt; ///< where to change it, e.g. "System Settings, Shortcuts"
    QString command = QStringLiteral("paraphrase-app --popup");
    QList<Step> steps;
};

} // namespace paraphrase::app

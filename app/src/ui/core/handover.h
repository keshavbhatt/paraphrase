#pragma once

#include <QJsonObject>
#include <QString>

namespace paraphrase::app {

/// What the popup hands the main window with "Open in window" (popup-open-in-window.html):
/// the text, and when a result page sent it, the action, its language and the result on
/// screen. Kept in memory only; nothing of it is written anywhere.
struct Handover
{
    QString text;
    QString skillId;    ///< empty: the text alone, from the actions page
    QString targetLang; ///< a language the user chose for the result
    /// The finished result as the daemon answered, its output as the popup shows it now
    /// (a proofread with some changes undone). Empty while it still ran.
    QJsonObject result;
};

} // namespace paraphrase::app

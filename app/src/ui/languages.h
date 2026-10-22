#pragma once

#include <QList>
#include <QString>

namespace paraphrase::util {
class LanguageDetector;
}

namespace paraphrase::app {

/// A language the user can pick as the output language (Translate).
struct MenuLanguage
{
    QString code; ///< a code the language detector knows
    bool tested;  ///< checked in depth by the eval (DOCS/models.md); the rest is best effort
};

/// Tested languages first, then best effort.
[[nodiscard]] const QList<MenuLanguage>& menuLanguages();

/// "Hindi" for "hi": the detector's plain name without its note in brackets.
[[nodiscard]] QString languageName(const util::LanguageDetector& detector, const QString& code);

} // namespace paraphrase::app

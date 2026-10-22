#pragma once

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QSet>
#include <QString>

#include <optional>
#include <utility>

namespace paraphrase::util {

/// A language code with a set of words or letters. Kept as an ordered list, not a map:
/// when two languages score the same, the one listed first wins (as in the Python reference).
using OrderedWordSets = QList<std::pair<QString, QSet<QString>>>;

/// The language data shared with the Python eval harness: daemon/data/language.json,
/// embedded as the resource `:/paraphrase/language.json` (DOCS/CODING_STANDARDS.md 12).
struct LanguageData
{
    char32_t devanagariFirst = 0x0900;
    char32_t devanagariLast = 0x097F;
    double hinglishMinRatio = 0.15;
    QSet<QString> hinglishWords;
    OrderedWordSets latinStopwords;
    OrderedWordSets latinLetterHints; // each set holds single letters
    OrderedWordSets devanagariMarkers;
    OrderedWordSets arabicMarkers;
    OrderedWordSets cyrillicMarkers;
    QSet<QString> ukrainianLetters;
    QHash<QString, QString> scriptLanguage; // "TAMIL" -> "ta"
    QHash<QString, QString> names;          // "ta" -> "Tamil"
    QHash<QString, QString> promptNames;    // en, hi, hinglish -> the prompt's description
    QString unknownPromptName;

    /// Parses the JSON; std::nullopt when it is not the expected shape.
    [[nodiscard]] static std::optional<LanguageData> fromJson(const QByteArray& json);
    /// The copy built into the daemon. Asserts in debug builds if the resource is broken.
    [[nodiscard]] static LanguageData builtIn();
};

} // namespace paraphrase::util

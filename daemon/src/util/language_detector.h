#pragma once

#include "language_data.h"

#include <QString>
#include <QStringList>
#include <QStringView>

namespace paraphrase::util {

/// The result of LanguageDetector::check: whether an output stayed in the expected language,
/// and if not, why (shown to the user with the output, never instead of it; ADR-006).
struct LanguageCheck
{
    bool ok = true;
    QString reason;
};

/// Detects the language of a text and checks that an output stayed in it. A line-by-line
/// port of the Python reference (eval/paraeval/langdetect.py, scripts.py and
/// metrics.language_check); tests/vectors/language.json holds both to the same answers.
///
/// Codes: "en", "hi" (Devanagari), "hinglish" (Hindi in Latin letters), another code from
/// the data ("fr", "ta", "mr", ...), or "und" when the language is not clear.
class LanguageDetector
{
public:
    explicit LanguageDetector(LanguageData data);

    [[nodiscard]] QString detect(QStringView text) const;
    /// How a prompt names the language (`lang_name`): never a guess for "und".
    [[nodiscard]] QString promptName(const QString& lang) const;
    /// The plain name ("Hinglish (Hindi in Latin letters)"), or "" if there is none.
    [[nodiscard]] QString plainName(const QString& lang) const;
    /// The owner's floor for en, hi and hinglish (PLAN 11 Q6); for other languages a switch
    /// is reported only on clear evidence (a different script, or mostly English words).
    [[nodiscard]] LanguageCheck check(const QString& lang, QStringView source, QStringView output) const;

private:
    struct ScriptShares
    {
        double devanagari = 0.0;
        double latin = 0.0;
    };

    [[nodiscard]] bool isDevanagari(char32_t cp) const;
    [[nodiscard]] ScriptShares scriptShares(QStringView text) const;
    [[nodiscard]] double hinglishRatio(QStringView text) const;
    [[nodiscard]] bool devanagariBroken(QStringView text) const;
    [[nodiscard]] LanguageCheck checkOther(const QString& lang, QStringView source, QStringView output) const;
    [[nodiscard]] QString latinLanguage(QStringView text) const;
    [[nodiscard]] QString otherLanguage(QStringView text, const QString& script) const;
    [[nodiscard]] double wordShare(QStringView text, const QString& lang, const QString& against) const;

    LanguageData m_data;
};

/// Unicode script family of a code point as the Python reference names it from the
/// character name ("LATIN", "DEVANAGARI", "HAN", "KANA", ...), or "" for none.
[[nodiscard]] QString scriptOf(char32_t cp);
/// The script of most letters (ties: the first seen), or "" for text without letters.
[[nodiscard]] QString dominantScript(QStringView text);
/// Lower-cased words, split on spaces, digits and punctuation (not on combining signs).
[[nodiscard]] QStringList words(QStringView text);

} // namespace paraphrase::util

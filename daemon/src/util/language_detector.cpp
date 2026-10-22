#include "language_detector.h"

#include <QChar>
#include <QRegularExpression>

#include <algorithm>
#include <utility>
#include <vector>

using namespace Qt::StringLiterals;

namespace paraphrase::util {
namespace {

bool isMark(char32_t cp)
{
    const auto cat = QChar::category(cp);
    return cat == QChar::Mark_NonSpacing || cat == QChar::Mark_SpacingCombining;
}

/// Python's str.isalpha(): Unicode letter categories.
bool isLetter(char32_t cp)
{
    return QChar::isLetter(cp);
}

QString titleCase(const QString& upper)
{
    return upper.isEmpty() ? upper : upper.left(1) + upper.mid(1).toLower();
}

/// The Python reference splits words on this character class (scripts._WORD).
const QRegularExpression& wordPattern()
{
    static const QRegularExpression re(uR"([^\s\d.,!?;:"()\[\]{}…।॥؟،¿¡«»“”‘’/\\|*#@&%+=<>~^`_]+)"_s,
                                       QRegularExpression::UseUnicodePropertiesOption);
    return re;
}

int countIn(const QStringList& list, const QSet<QString>& set)
{
    return static_cast<int>(
        std::ranges::count_if(list, [&set](const QString& w) { return set.contains(w); }));
}

/// The language with the most marker words (ties: listed first), or "" if none appears.
QString marked(const QStringList& list, const OrderedWordSets& markers)
{
    QString best;
    int bestHits = 0;
    for (const auto& [code, set] : markers) {
        const int hits = countIn(list, set);
        if (hits > bestHits) {
            best = code;
            bestHits = hits;
        }
    }
    return best;
}

QSet<QString> lettersOf(QStringView text)
{
    QSet<QString> out;
    for (const char32_t cp : text.toString().toLower().toUcs4()) {
        out.insert(QString::fromUcs4(&cp, 1));
    }
    return out;
}

} // namespace

QString scriptOf(char32_t cp)
{
    switch (QChar::script(cp)) {
    case QChar::Script_Latin:
        return u"LATIN"_s;
    case QChar::Script_Devanagari:
        return u"DEVANAGARI"_s;
    case QChar::Script_Bengali:
        return u"BENGALI"_s;
    case QChar::Script_Gurmukhi:
        return u"GURMUKHI"_s;
    case QChar::Script_Gujarati:
        return u"GUJARATI"_s;
    case QChar::Script_Oriya:
        return u"ORIYA"_s;
    case QChar::Script_Tamil:
        return u"TAMIL"_s;
    case QChar::Script_Telugu:
        return u"TELUGU"_s;
    case QChar::Script_Kannada:
        return u"KANNADA"_s;
    case QChar::Script_Malayalam:
        return u"MALAYALAM"_s;
    case QChar::Script_Sinhala:
        return u"SINHALA"_s;
    case QChar::Script_Thai:
        return u"THAI"_s;
    case QChar::Script_Lao:
        return u"LAO"_s;
    case QChar::Script_Khmer:
        return u"KHMER"_s;
    case QChar::Script_Myanmar:
        return u"MYANMAR"_s;
    case QChar::Script_Hangul:
        return u"HANGUL"_s;
    case QChar::Script_Greek:
        return u"GREEK"_s;
    case QChar::Script_Hebrew:
        return u"HEBREW"_s;
    case QChar::Script_Georgian:
        return u"GEORGIAN"_s;
    case QChar::Script_Armenian:
        return u"ARMENIAN"_s;
    case QChar::Script_Ethiopic:
        return u"ETHIOPIC"_s;
    case QChar::Script_Arabic:
        return u"ARABIC"_s;
    case QChar::Script_Cyrillic:
        return u"CYRILLIC"_s;
    case QChar::Script_Han:
        return u"HAN"_s;
    case QChar::Script_Hiragana:
    case QChar::Script_Katakana:
        return u"KANA"_s;
    case QChar::Script_Inherited:
        return u"COMBINING"_s;
    case QChar::Script_Unknown:
        return {};
    default:
        return u"SCRIPT%1"_s.arg(static_cast<int>(QChar::script(cp)));
    }
}

QString dominantScript(QStringView text)
{
    std::vector<std::pair<QString, int>> counts; // insertion order breaks ties
    for (const char32_t cp : text.toUcs4()) {
        if (!isLetter(cp) && !isMark(cp)) {
            continue;
        }
        const QString s = scriptOf(cp);
        if (s.isEmpty()) {
            continue;
        }
        const auto it = std::ranges::find_if(counts, [&s](const auto& entry) { return entry.first == s; });
        if (it == counts.end()) {
            counts.emplace_back(s, 1);
        } else {
            ++it->second;
        }
    }
    QString best;
    int bestCount = 0;
    for (const auto& [script, count] : counts) {
        if (count > bestCount) {
            best = script;
            bestCount = count;
        }
    }
    return best;
}

QStringList words(QStringView text)
{
    QStringList out;
    auto it = wordPattern().globalMatchView(text);
    while (it.hasNext()) {
        out.append(it.next().captured(0).toLower());
    }
    return out;
}

LanguageDetector::LanguageDetector(LanguageData data)
    : m_data(std::move(data))
{}

bool LanguageDetector::isDevanagari(char32_t cp) const
{
    return cp >= m_data.devanagariFirst && cp <= m_data.devanagariLast;
}

LanguageDetector::ScriptShares LanguageDetector::scriptShares(QStringView text) const
{
    int deva = 0;
    int latin = 0;
    for (const char32_t cp : text.toUcs4()) {
        if (!isLetter(cp) && !isMark(cp)) {
            continue;
        }
        if (isDevanagari(cp)) {
            ++deva;
        } else if (cp < 0x80) {
            ++latin;
        }
    }
    const int total = deva + latin;
    if (total == 0) {
        return {};
    }
    return {.devanagari = static_cast<double>(deva) / total, .latin = static_cast<double>(latin) / total};
}

double LanguageDetector::hinglishRatio(QStringView text) const
{
    static const QRegularExpression ascii(u"[A-Za-z]+"_s);
    int all = 0;
    int hits = 0;
    auto it = ascii.globalMatchView(text);
    while (it.hasNext()) {
        ++all;
        if (m_data.hinglishWords.contains(it.next().captured(0).toLower())) {
            ++hits;
        }
    }
    return all == 0 ? 0.0 : static_cast<double>(hits) / all;
}

bool LanguageDetector::devanagariBroken(QStringView text) const
{
    if (text.contains(QChar(0xFFFD))) {
        return true;
    }
    char32_t prev = 0;
    bool havePrev = false;
    for (const char32_t cp : text.toUcs4()) {
        const bool sign = isDevanagari(cp) && isMark(cp);
        if (sign && !(havePrev && isDevanagari(prev))) {
            return true;
        }
        prev = cp;
        havePrev = true;
    }
    return false;
}

QString LanguageDetector::detect(QStringView text) const
{
    const ScriptShares shares = scriptShares(text);
    if (shares.devanagari >= 0.3) {
        const QString marker = marked(words(text), m_data.devanagariMarkers);
        return marker.isEmpty() ? u"hi"_s : marker;
    }
    const QString script = dominantScript(text);
    if (!script.isEmpty() && script != u"LATIN"_s && script != u"DEVANAGARI"_s) {
        return otherLanguage(text, script);
    }
    if (shares.latin > 0 && hinglishRatio(text) >= m_data.hinglishMinRatio) {
        return u"hinglish"_s;
    }
    const QString latin = latinLanguage(text);
    return latin.isEmpty() ? u"en"_s : latin;
}

QString LanguageDetector::latinLanguage(QStringView text) const
{
    const QStringList list = words(text);
    if (list.size() < 3) {
        return {};
    }
    const QSet<QString> letters = lettersOf(text);
    std::vector<std::pair<QString, double>> ratios;
    double english = 0.0;
    for (const auto& [code, stop] : m_data.latinStopwords) {
        double ratio = static_cast<double>(countIn(list, stop)) / static_cast<double>(list.size());
        for (const auto& [hintCode, hints] : m_data.latinLetterHints) {
            if (hintCode == code && letters.intersects(hints)) {
                ratio += 0.1;
            }
        }
        if (code == u"en"_s) {
            english = ratio;
        } else {
            ratios.emplace_back(code, ratio);
        }
    }
    QString foreign;
    double best = -1.0;
    for (const auto& [code, ratio] : ratios) {
        if (ratio > best) {
            foreign = code;
            best = ratio;
        }
    }
    if (!foreign.isEmpty() && best >= 0.2 && best > english + 0.05) {
        return foreign;
    }
    return {};
}

QString LanguageDetector::otherLanguage(QStringView text, const QString& script) const
{
    if (const auto it = m_data.scriptLanguage.constFind(script); it != m_data.scriptLanguage.constEnd()) {
        return it.value();
    }
    if (script == u"KANA"_s) {
        return u"ja"_s;
    }
    if (script == u"HAN"_s) {
        const auto ucs4 = text.toUcs4();
        const bool kana = std::ranges::any_of(ucs4, [](char32_t cp) { return scriptOf(cp) == u"KANA"_s; });
        return kana ? u"ja"_s : u"zh"_s;
    }
    const QStringList list = words(text);
    if (script == u"ARABIC"_s) {
        const QString m = marked(list, m_data.arabicMarkers);
        return m.isEmpty() ? u"ar"_s : m;
    }
    if (script == u"CYRILLIC"_s) {
        if (lettersOf(text).intersects(m_data.ukrainianLetters)) {
            return u"uk"_s;
        }
        const QString m = marked(list, m_data.cyrillicMarkers);
        return m.isEmpty() ? u"und"_s : m;
    }
    return u"und"_s;
}

double LanguageDetector::wordShare(QStringView text, const QString& lang, const QString& against) const
{
    QStringList list;
    for (const QString& w : words(text)) {
        if (w.toUcs4().size() > 1) {
            list.append(w);
        }
    }
    const auto find = [this](const QString& code) -> const QSet<QString>* {
        for (const auto& [c, set] : m_data.latinStopwords) {
            if (c == code) {
                return &set;
            }
        }
        return nullptr;
    };
    const QSet<QString>* stop = find(lang);
    if (list.isEmpty() || stop == nullptr) {
        return 0.0;
    }
    const QSet<QString>* shared = find(against);
    const auto hits = std::ranges::count_if(list, [&](const QString& w) {
        return stop->contains(w) && (shared == nullptr || !shared->contains(w));
    });
    return static_cast<double>(hits) / static_cast<double>(list.size());
}

QString LanguageDetector::promptName(const QString& lang) const
{
    if (const auto it = m_data.promptNames.constFind(lang); it != m_data.promptNames.constEnd()) {
        return it.value();
    }
    return m_data.names.value(lang, m_data.unknownPromptName);
}

QString LanguageDetector::plainName(const QString& lang) const
{
    return m_data.names.value(lang);
}

LanguageCheck LanguageDetector::check(const QString& lang, QStringView source, QStringView output) const
{
    const double deva = scriptShares(output).devanagari;
    if (lang == u"hi"_s) {
        if (devanagariBroken(output)) {
            return {.ok = false, .reason = u"broken Devanagari"_s};
        }
        // Code-mixed Hindi keeps English words in Latin letters, so compare with the source.
        if (deva < std::min(0.6, 0.5 * scriptShares(source).devanagari)) {
            return {.ok = false, .reason = u"Hindi answered outside Devanagari"_s};
        }
    } else if (lang == u"hinglish"_s) {
        if (deva > 0.2) {
            return {.ok = false, .reason = u"Hinglish turned into Devanagari"_s};
        }
        const double src = hinglishRatio(source);
        if (src > 0 && hinglishRatio(output) < 0.4 * src) {
            return {.ok = false, .reason = u"Hinglish turned into English"_s};
        }
    } else if (lang == u"en"_s) {
        if (deva > 0.2) {
            return {.ok = false, .reason = u"English answered in Devanagari"_s};
        }
        if (hinglishRatio(output) >= 0.2 && hinglishRatio(source) < 0.05) {
            return {.ok = false, .reason = u"English answered in Hinglish"_s};
        }
    } else {
        return checkOther(lang, source, output);
    }
    return {};
}

LanguageCheck LanguageDetector::checkOther(const QString& lang, QStringView source, QStringView output) const
{
    const QString name = m_data.names.value(lang, u"the input language"_s);
    const QString src = dominantScript(source);
    const QString out = dominantScript(output);
    if (!src.isEmpty() && !out.isEmpty() && src != out) {
        return {.ok = false, .reason = u"%1 answered in %2 script"_s.arg(name, titleCase(out))};
    }
    static const QRegularExpression space(u"\\s+"_s, QRegularExpression::UseUnicodePropertiesOption);
    const auto outputWords = output.toString().split(space, Qt::SkipEmptyParts).size();
    if (src == u"LATIN"_s && lang != u"und"_s && outputWords >= 4) {
        const double en = wordShare(output, u"en"_s, lang);
        const double own = wordShare(output, lang, u"en"_s);
        if (en >= 0.2 && en > own + 0.05) {
            return {.ok = false, .reason = u"%1 answered in English"_s.arg(name)};
        }
    }
    return {};
}

} // namespace paraphrase::util

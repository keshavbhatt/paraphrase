#include "language_data.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

using namespace Qt::StringLiterals;

namespace paraphrase::util {
namespace {

QSet<QString> stringSet(const QJsonArray& array)
{
    QSet<QString> out;
    for (const auto& v : array) {
        out.insert(v.toString());
    }
    return out;
}

QSet<QString> letterSet(const QString& letters)
{
    QSet<QString> out;
    for (const char32_t cp : letters.toUcs4()) {
        out.insert(QString::fromUcs4(&cp, 1));
    }
    return out;
}

/// [[code, [words...]], ...] or, with `letters`, [[code, "abc"], ...].
OrderedWordSets orderedSets(const QJsonArray& array, bool letters = false)
{
    OrderedWordSets out;
    for (const auto& entry : array) {
        const QJsonArray pair = entry.toArray();
        const QString code = pair.at(0).toString();
        out.emplace_back(code, letters ? letterSet(pair.at(1).toString()) : stringSet(pair.at(1).toArray()));
    }
    return out;
}

QHash<QString, QString> stringMap(const QJsonObject& object)
{
    QHash<QString, QString> out;
    for (auto it = object.begin(); it != object.end(); ++it) {
        out.insert(it.key(), it.value().toString());
    }
    return out;
}

} // namespace

std::optional<LanguageData> LanguageData::fromJson(const QByteArray& json)
{
    const QJsonDocument doc = QJsonDocument::fromJson(json);
    if (!doc.isObject()) {
        return std::nullopt;
    }
    const QJsonObject o = doc.object();
    const QJsonArray range = o.value(u"devanagari_range"_s).toArray();
    if (range.size() != 2 || !o.value(u"hinglish_words"_s).isArray()) {
        return std::nullopt;
    }
    LanguageData d;
    d.devanagariFirst = static_cast<char32_t>(range.at(0).toInt());
    d.devanagariLast = static_cast<char32_t>(range.at(1).toInt());
    d.hinglishMinRatio = o.value(u"hinglish_min_ratio"_s).toDouble();
    d.hinglishWords = stringSet(o.value(u"hinglish_words"_s).toArray());
    d.latinStopwords = orderedSets(o.value(u"latin_stopwords"_s).toArray());
    d.latinLetterHints = orderedSets(o.value(u"latin_letter_hints"_s).toArray(), true);
    d.devanagariMarkers = orderedSets(o.value(u"devanagari_markers"_s).toArray());
    d.arabicMarkers = orderedSets(o.value(u"arabic_markers"_s).toArray());
    d.cyrillicMarkers = orderedSets(o.value(u"cyrillic_markers"_s).toArray());
    d.ukrainianLetters = letterSet(o.value(u"ukrainian_letters"_s).toString());
    d.scriptLanguage = stringMap(o.value(u"script_language"_s).toObject());
    d.names = stringMap(o.value(u"names"_s).toObject());
    d.promptNames = stringMap(o.value(u"prompt_names"_s).toObject());
    d.unknownPromptName = o.value(u"unknown_prompt_name"_s).toString();
    return d;
}

LanguageData LanguageData::builtIn()
{
    QFile file(u":/paraphrase/language.json"_s);
    const bool opened = file.open(QIODevice::ReadOnly);
    Q_ASSERT_X(opened, "LanguageData::builtIn", "resource :/paraphrase/language.json missing");
    const auto data = fromJson(file.readAll());
    Q_ASSERT_X(data.has_value(), "LanguageData::builtIn", "language.json has the wrong shape");
    return data.value_or(LanguageData{});
}

} // namespace paraphrase::util

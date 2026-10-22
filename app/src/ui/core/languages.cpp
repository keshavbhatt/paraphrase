#include "ui/core/languages.h"

#include "util/language_detector.h"

using namespace Qt::StringLiterals;

namespace paraphrase::app {

const QList<MenuLanguage>& menuLanguages()
{
    static const QList<MenuLanguage> kLanguages = {
        {.code = u"en"_s, .tested = true},       {.code = u"hi"_s, .tested = true},
        {.code = u"hinglish"_s, .tested = true}, {.code = u"mr"_s, .tested = false},
        {.code = u"bn"_s, .tested = false},      {.code = u"ta"_s, .tested = false},
        {.code = u"te"_s, .tested = false},      {.code = u"gu"_s, .tested = false},
        {.code = u"pa"_s, .tested = false},      {.code = u"ur"_s, .tested = false},
        {.code = u"es"_s, .tested = false},      {.code = u"fr"_s, .tested = false},
        {.code = u"de"_s, .tested = false},      {.code = u"pt"_s, .tested = false},
        {.code = u"it"_s, .tested = false},      {.code = u"ar"_s, .tested = false},
        {.code = u"zh"_s, .tested = false},      {.code = u"ja"_s, .tested = false},
        {.code = u"ko"_s, .tested = false},      {.code = u"ru"_s, .tested = false},
        {.code = u"id"_s, .tested = false},      {.code = u"tr"_s, .tested = false},
        {.code = u"nl"_s, .tested = false},
    };
    return kLanguages;
}

QString languageName(const util::LanguageDetector& detector, const QString& code)
{
    return detector.plainName(code).section(u" ("_s, 0, 0);
}

} // namespace paraphrase::app

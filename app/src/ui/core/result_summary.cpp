#include "ui/core/result_summary.h"

#include <QObject>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

QString resultSummary(const ResultFacts& facts)
{
    QString line;
    if (facts.proofread) {
        if (facts.changes == 0) {
            return QObject::tr("· nothing to fix");
        }
        // Plurals written out: tr()'s %n needs a loaded translation to pluralise.
        line =
            facts.changes == 1 ? QObject::tr("· 1 change") : QObject::tr("· %1 changes").arg(facts.changes);
    }
    if (!facts.replyKind.isEmpty()) {
        line += u"· "_s + facts.replyKind;
    }
    if (!facts.language.isEmpty()) {
        const bool kept = facts.kind == SkillKind::Text && !facts.chosenLanguage;
        line += (kept ? QObject::tr(" · kept in %1") : QObject::tr(" · in %1")).arg(facts.language);
    }
    if (!facts.facts.isEmpty()) {
        line += u" · "_s + facts.facts;
    }
    return line.trimmed();
}

bool keepsFacts(const QString& base)
{
    return base == u"rewrite"_s || base == u"format"_s || base == u"compose"_s;
}

QString languageWarning(const QString& language)
{
    return QObject::tr("· may not be in %1: check before using it")
        .arg(language.isEmpty() ? QObject::tr("the right language") : language);
}

QString historyLabel(const SkillInfo& skill, const QString& language, int changes)
{
    QString label = skill.title;
    label.replace(u": "_s, u" · "_s);
    if (!language.isEmpty()) {
        label += u" · "_s + QObject::tr("into %1").arg(language);
    }
    if (changes >= 0) {
        label += u" · "_s + (changes == 1 ? QObject::tr("1 change") : QObject::tr("%1 changes").arg(changes));
    }
    return label;
}

} // namespace paraphrase::app

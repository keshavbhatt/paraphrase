#include "ui/core/facts_check.h"

#include <QObject>
#include <QRegularExpression>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr qsizetype kShown = 4; ///< facts named in the line; the rest are counted

QStringList protectedTokens(const QString& text)
{
    static const QRegularExpression kProtected(
        uR"(https?://\S+|www\.\S+|[\w.+-]+@[\w-]+\.[\w.]+|[@#]\w+|`[^`]+`|\d[\d,.:/-]*\d\p{L}*|\d\p{L}*)"_s);
    static const QRegularExpression kTrailing(u"[.,;:!?)]+$"_s);
    QStringList out;
    for (auto it = kProtected.globalMatch(text); it.hasNext();) {
        out.append(it.next().captured(0).remove(kTrailing));
    }
    return out;
}

QStringList names(const QString& text)
{
    // A capitalised Latin word that does not start a sentence: "Sam", "Friday", "March".
    static const QRegularExpression kName(uR"((?<![.!?]\s)(?<!^)(?<=\s)\p{Lu}[\p{Ll}]{1,})"_s,
                                          QRegularExpression::MultilineOption);
    QStringList out;
    for (auto it = kName.globalMatch(text); it.hasNext();) {
        const QString word = it.next().captured(0);
        if (word != u"I"_s && !out.contains(word)) {
            out.append(word);
        }
    }
    return out;
}

} // namespace

FactsCheck checkFacts(const QString& source, const QString& result)
{
    FactsCheck check;
    QStringList facts = protectedTokens(source);
    for (const QString& n : names(source)) {
        if (!facts.contains(n)) {
            facts.append(n);
        }
    }
    for (const QString& f : facts) {
        (result.contains(f) ? check.kept : check.missing).append(f);
    }
    check.kept.removeDuplicates();
    check.missing.removeDuplicates();
    return check;
}

QString factsLine(const FactsCheck& check)
{
    if (!check.missing.isEmpty()) {
        const QString list = check.missing.mid(0, kShown).join(u", "_s);
        return check.missing.size() == 1 ? QObject::tr("check: %1 is missing").arg(list)
                                         : QObject::tr("check: %1 are missing").arg(list);
    }
    if (check.kept.isEmpty()) {
        return {};
    }
    QString list = check.kept.mid(0, kShown).join(u", "_s);
    if (check.kept.size() > kShown) {
        list += QObject::tr(" and %1 more").arg(check.kept.size() - kShown);
    }
    return QObject::tr("kept: %1").arg(list);
}

} // namespace paraphrase::app

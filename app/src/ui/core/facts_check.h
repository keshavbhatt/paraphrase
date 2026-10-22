#pragma once

#include <QString>
#include <QStringList>

namespace paraphrase::app {

/// The facts of a text that a rewrite must keep (popup-rewrite.html, note 4): numbers, dates,
/// times, money, links, email addresses, @mentions, #tags, code, and names (capitalised words
/// inside a sentence). The same tokens as the eval's protected_kept (eval/paraeval/metrics.py),
/// plus names, checked in the result as plain substrings.
struct FactsCheck
{
    QStringList kept;
    QStringList missing;

    [[nodiscard]] bool isEmpty() const { return kept.isEmpty() && missing.isEmpty(); }
};

[[nodiscard]] FactsCheck checkFacts(const QString& source, const QString& result);
/// "kept: Sam, Friday" or "check: Friday is missing", "" when the text has no such facts.
[[nodiscard]] QString factsLine(const FactsCheck& check);

} // namespace paraphrase::app

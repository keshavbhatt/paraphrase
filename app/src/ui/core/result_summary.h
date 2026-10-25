#pragma once

#include "ui/core/skill_catalog.h"

#include <QString>

namespace paraphrase::app {

/// What the line after a result's title says.
struct ResultFacts
{
    SkillKind kind = SkillKind::Text;
    bool proofread = false;      ///< a word diff is shown
    qsizetype changes = 0;       ///< for proofread
    QString language;            ///< plain name of the result's language, may be empty
    bool chosenLanguage = false; ///< the user picked the language (Translate)
    QString replyKind;           ///< "email" or "chat message" for a full reply
    QString facts;               ///< "kept: Sam, Friday" or "check: Friday is missing" (facts_check.h)
};

/// Skills whose result must keep the text's facts: the line names them, a missing one warns.
[[nodiscard]] bool keepsFacts(const QString& base);

/// "· 4 changes · kept in English", "· in Hindi", "· email · in English", "· nothing to fix".
[[nodiscard]] QString resultSummary(const ResultFacts& facts);
/// The warning when the language check failed after its retry: shown, never hidden (ADR-006).
[[nodiscard]] QString languageWarning(const QString& language);
/// The action as the history lists it: "Tone · Elaborate · into Hindi", "Proofread · 5 changes".
/// `language` is a chosen language's name ("" for none); `changes` < 0 says nothing of them.
[[nodiscard]] QString historyLabel(const SkillInfo& skill, const QString& language, int changes = -1);

} // namespace paraphrase::app

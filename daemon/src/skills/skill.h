#pragma once

#include "template_renderer.h"

#include <QJsonObject>
#include <QString>

namespace paraphrase::skills {

/// One skill, or one preset of a skill (`rewrite.professional`), as loaded from skills/*.yaml
/// (schema v3, DOCS/skills.md). Plain data: the engine reads it, never changes it.
struct Skill
{
    QString id;     ///< "proofread", or "<base>.<preset>"
    QString baseId; ///< the file's `id`
    int version = 0;
    int revision = 1;
    QString title;
    QString tier; ///< fast | quality | any
    int maxInputTokens = 0;
    int maxOutputTokens = 0;
    int variants = 1;
    QString speculative = QStringLiteral("none"); ///< none | draft (ADR-004)
    QJsonObject sampling;                         ///< temperature, top_p, top_k, min_p, repeat_penalty, seed
    QJsonObject output;                           ///< type, language, schema, items, diff
    QString system;
    QString templ;
    QString grammar; ///< GBNF text, empty when the skill has none
    Values preset;   ///< extra template values of a preset (title excluded)

    struct Prompt
    {
        QString system;
        QString user;
    };

    /// The system and user messages for these inputs: preset values first, the request's on
    /// top, every value fenced against the template's tags (captured text is untrusted).
    [[nodiscard]] Prompt render(const Values& inputs) const;

    [[nodiscard]] QString outputType() const { return output.value(QStringLiteral("type")).toString(); }
    /// The input the skill works on: "context" for a skill that answers a message (its
    /// template uses {{context}}; "text" is then an optional draft), otherwise "text".
    [[nodiscard]] QString mainInput() const;
    /// The request inputs the prompt uses, in a fixed order: text, context, intent, tone,
    /// target_lang (a preset's own values excluded). Clients build their forms from it.
    [[nodiscard]] QStringList requestInputs() const;
    /// The shortest answer allowed, as a fraction of the input text (schema v4); 0 for none.
    [[nodiscard]] double minLength() const
    {
        return output.value(QStringLiteral("min_length")).toDouble(0.0);
    }
    [[nodiscard]] bool keepsLanguage() const
    {
        return output.value(QStringLiteral("language")).toString() == QStringLiteral("same");
    }
};

} // namespace paraphrase::skills

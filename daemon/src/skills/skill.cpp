#include "skill.h"

#include "template_renderer.h"

namespace paraphrase::skills {

Skill::Prompt Skill::render(const Values& inputs) const
{
    Values values = preset;
    for (auto it = inputs.begin(); it != inputs.end(); ++it) {
        values.insert(it.key(), it.value());
    }
    const QSet<QString> tags = delimiterTags(templ);
    for (auto it = values.begin(); it != values.end(); ++it) {
        it.value() = neutralise(it.value(), tags);
    }
    return {.system = system.trimmed(), .user = renderTemplate(templ, values).trimmed()};
}

QStringList Skill::requestInputs() const
{
    const QSet<QString> used = templateVariables(templ);
    QStringList out;
    for (const QString& key : {
             QStringLiteral("text"),
             QStringLiteral("context"),
             QStringLiteral("intent"),
             QStringLiteral("tone"),
             QStringLiteral("target_lang"),
         }) {
        if (used.contains(key) && !preset.contains(key)) {
            out.append(key);
        }
    }
    return out;
}

QString Skill::mainInput() const
{
    return templateVariables(templ).contains(QStringLiteral("context")) ? QStringLiteral("context")
                                                                        : QStringLiteral("text");
}

} // namespace paraphrase::skills

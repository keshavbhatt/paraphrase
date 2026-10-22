#pragma once

#include <QHash>
#include <QSet>
#include <QString>

namespace paraphrase::skills {

using Values = QHash<QString, QString>;

/// Renders a skill template (DOCS/skills.md, "Template rules"): `{{name}}` inserts a value,
/// `{{#if name}}...{{/if}}` keeps its body when the value is not blank, `{{#unless name}}...
/// {{/unless}}` when it is. Blocks resolve from the inside out; values are inserted in one
/// final pass, so braces inside a value stay literal. A port of eval/paraeval/skills.py.
[[nodiscard]] QString renderTemplate(const QString& templ, const Values& values);

/// Variable names a template uses, in values and in blocks.
[[nodiscard]] QSet<QString> templateVariables(const QString& templ);

/// Tag names the template uses to fence data (`<text>`, `<message>`, `<draft>`).
[[nodiscard]] QSet<QString> delimiterTags(const QString& templ);

/// Stops data from opening or closing one of those fences: `</text>` becomes `< /text>`.
[[nodiscard]] QString neutralise(QString value, const QSet<QString>& tags);

} // namespace paraphrase::skills

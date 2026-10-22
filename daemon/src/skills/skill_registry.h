#pragma once

#include "skill.h"
#include "util/result.h"

#include <QList>
#include <QString>
#include <QStringList>

namespace paraphrase::skills {

inline constexpr int kSchemaVersion = 4;

/// Every skill the daemon can run, loaded from skills/*.yaml (PLAN D4). A file that breaks the
/// schema is rejected with a message naming it, never half-loaded. A port of the Python
/// loader (eval/paraeval/skills.py) plus the template checks of eval/validate_skills.py.
class SkillRegistry
{
public:
    /// All `*.yaml` files in `skillsDir`, in name order; each file in `overrides` replaces the
    /// skills it defines (a candidate revision under evaluation).
    [[nodiscard]] static util::Result<SkillRegistry> load(const QString& skillsDir,
                                                          const QStringList& overrides = {});

    /// The skills of one file: one skill, or one per preset.
    [[nodiscard]] static util::Result<QList<Skill>> loadFile(const QString& path);

    [[nodiscard]] const Skill* find(const QString& id) const;
    [[nodiscard]] const QList<Skill>& skills() const { return m_skills; }

private:
    QList<Skill> m_skills; // load order: file name order, then preset order
};

} // namespace paraphrase::skills

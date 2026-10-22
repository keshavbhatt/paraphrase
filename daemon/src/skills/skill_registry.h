#pragma once

#include "skill.h"
#include "user_actions.h"
#include "util/result.h"

#include <QList>
#include <QString>
#include <QStringList>

namespace paraphrase::skills {

inline constexpr int kSchemaVersion = 5;
/// Files written for an older schema still load when nothing they use has changed.
inline constexpr int kMinSchemaVersion = 4;

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

    /// Whether a skill takes the user's own actions (schema v5 `user_presets`).
    [[nodiscard]] bool takesUserActions() const { return !m_userBases.isEmpty(); }
    /// The skill id of a user action: `<skill>.<action id>`.
    [[nodiscard]] QString userActionSkillId(const QString& actionId) const
    {
        return m_userBases.isEmpty() ? QString() : m_userBases.front().baseId + u'.' + actionId;
    }
    /// Replaces the user's actions: each becomes `<skill>.<action id>` of the first skill that
    /// takes user presets, with the action's tier and its instruction as the preset value.
    void setUserActions(const QList<UserAction>& actions);

private:
    QList<Skill> m_skills;    // load order: file name order, then preset order; user actions last
    QList<Skill> m_userBases; // skills that take user presets, kept out of m_skills
    qsizetype m_builtIn = 0;  // m_skills before the user's actions
};

} // namespace paraphrase::skills

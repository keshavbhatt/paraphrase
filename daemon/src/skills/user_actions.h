#pragma once

#include "util/result.h"

#include <QList>
#include <QString>

namespace paraphrase::skills {

/// One of the user's own actions (DOCS/skills.md, "User presets"): a name, what it does in the
/// user's words, and the model tier it runs on. It becomes a preset of a skill that takes
/// user presets, e.g. `custom.tweet`.
struct UserAction
{
    QString id;          ///< a slug of the name, unique: "tweet", "meeting-notes"
    QString name;        ///< shown in the menu, at most kMaxName characters
    QString instruction; ///< the user's instruction, at most kMaxInstruction characters
    QString tier;        ///< fast | quality
};

/// The user's actions, kept as JSON in the config folder (never shared, never logged).
class UserActions
{
public:
    static constexpr qsizetype kMaxActions = 20;
    static constexpr qsizetype kMaxName = 40;
    static constexpr qsizetype kMaxInstruction = 600;

    UserActions() = default;
    explicit UserActions(QString path)
        : m_path(std::move(path))
    {}

    /// Reads the file; a missing file is no actions, a broken one is an error.
    [[nodiscard]] util::Result<bool> load();
    [[nodiscard]] const QList<UserAction>& actions() const { return m_actions; }

    /// Adds an action (empty `id`) or changes one; returns its id, or why it was refused.
    [[nodiscard]] util::Result<QString> save(const UserAction& action);
    [[nodiscard]] util::Result<bool> remove(const QString& id);

private:
    [[nodiscard]] static QString check(const UserAction& action);
    [[nodiscard]] QString uniqueId(const QString& name) const;
    [[nodiscard]] util::Result<bool> write() const;

    QString m_path;
    QList<UserAction> m_actions;
};

} // namespace paraphrase::skills

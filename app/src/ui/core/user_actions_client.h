#pragma once

#include <QList>
#include <QString>

namespace paraphrase::client {
class DaemonClient;
}

namespace paraphrase::app {

/// One of the user's own actions as the daemon lists it (ListActions, DOCS/api.md).
struct ActionInfo
{
    QString id;
    QString skill; ///< the skill id to run it: `custom.<id>`
    QString name;
    QString instruction;
    QString tier; ///< fast | quality
};

/// The daemon's "My actions" calls, for Settings: list, save and delete the user's actions.
/// No widgets; the daemon broadcasts SkillsChanged after a change.
class UserActionsClient
{
public:
    explicit UserActionsClient(client::DaemonClient& client)
        : m_client(client)
    {}

    /// Reads the actions; false when the daemon does not take user actions.
    [[nodiscard]] bool load();
    [[nodiscard]] bool supported() const { return m_supported; }
    [[nodiscard]] qsizetype max() const { return m_max; }
    /// The skill that runs an action not saved yet, with its instruction (Try it).
    [[nodiscard]] QString templateSkill() const { return m_template; }
    [[nodiscard]] const QList<ActionInfo>& actions() const { return m_actions; }

    /// Saves an action (empty id: a new one); "" when saved, else the daemon's message.
    [[nodiscard]] QString save(const ActionInfo& action, QString* skillId = nullptr);
    [[nodiscard]] QString remove(const QString& id);

private:
    client::DaemonClient& m_client;
    QList<ActionInfo> m_actions;
    bool m_supported = false;
    qsizetype m_max = 0;
    QString m_template;
};

} // namespace paraphrase::app

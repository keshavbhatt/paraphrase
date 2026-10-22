#pragma once

#include "ui/core/user_actions_client.h"

#include <QWidget>

class QLabel;

namespace paraphrase::client {
class DaemonClient;
}

namespace paraphrase::app {

class ActionEditor;
class ActionList;
class SampleRun;

/// Settings, My actions (settings-actions.html): the list of the user's actions and the editor
/// for one. It connects them to the daemon; the list and the editor only report clicks.
class ActionsPage : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ActionsPage)

public:
    explicit ActionsPage(client::DaemonClient& client, QWidget* parent = nullptr);
    ~ActionsPage() override = default;

    /// Reads the actions again (the page is opened).
    void reload();
    /// Opens the editor on a new action ("New action..." in a My actions menu).
    void startNew();

private:
    void edit(const QString& id);
    void save(const ActionInfo& action);
    void tryOut(const ActionInfo& action, const QString& sample);

    UserActionsClient m_actions;
    ActionList* m_list = nullptr;
    ActionEditor* m_editor = nullptr;
    SampleRun* m_sample = nullptr;
    QLabel* m_unsupported = nullptr;
};

} // namespace paraphrase::app

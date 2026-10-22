#pragma once

#include "ui/core/user_actions_client.h"

#include <QFrame>

class QLabel;
class QPushButton;
class QVBoxLayout;

namespace paraphrase::app {

/// The user's actions, one row each (settings-actions.html, note 1): name, what it does, its
/// model, Edit and Delete. Delete asks once in place (note 2) before it reports.
class ActionList : public QFrame
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ActionList)

public:
    explicit ActionList(QWidget* parent = nullptr);
    ~ActionList() override = default;

    void setActions(const QList<ActionInfo>& actions, qsizetype max);

Q_SIGNALS:
    void newClicked();
    void editClicked(const QString& id);
    void deleteConfirmed(const QString& id);

private:
    [[nodiscard]] QWidget* row(const ActionInfo& action);

    QVBoxLayout* m_rows = nullptr;
    QLabel* m_count = nullptr;
    QPushButton* m_new = nullptr;
};

} // namespace paraphrase::app

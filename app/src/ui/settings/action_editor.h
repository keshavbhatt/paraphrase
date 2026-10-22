#pragma once

#include "ui/core/user_actions_client.h"

#include <QFrame>

class QGridLayout;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;

namespace paraphrase::app {

class SegmentedControl;

/// Makes or changes one action (settings-actions.html, notes 3 and 4): name, what it does,
/// fast or quality, Try it on a sample before saving, Cancel and Save. It only reports.
class ActionEditor : public QFrame
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ActionEditor)

public:
    explicit ActionEditor(QWidget* parent = nullptr);
    ~ActionEditor() override = default;

    /// Starts editing `action`, or a new one when its id is empty.
    void edit(const ActionInfo& action);
    [[nodiscard]] ActionInfo action() const;
    void showTryResult(const QString& text, bool problem);
    void showProblem(const QString& message);

Q_SIGNALS:
    void tryClicked(const ActionInfo& action, const QString& sample);
    void saveClicked(const ActionInfo& action);
    void cancelClicked();

private:
    void buildForm(QGridLayout* form);

    QString m_id;
    QLabel* m_title = nullptr;
    QLineEdit* m_name = nullptr;
    QPlainTextEdit* m_instruction = nullptr;
    SegmentedControl* m_tier = nullptr;
    QPlainTextEdit* m_sample = nullptr;
    QLabel* m_tryResult = nullptr;
    QLabel* m_problem = nullptr;
};

} // namespace paraphrase::app

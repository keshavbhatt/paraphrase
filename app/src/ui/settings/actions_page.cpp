#include "ui/settings/actions_page.h"

#include "ui/common/widgets.h"
#include "ui/settings/action_editor.h"
#include "ui/settings/action_list.h"
#include "ui/setup/sample_run.h"

#include <QLabel>
#include <QVBoxLayout>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

ActionsPage::ActionsPage(client::DaemonClient& client, QWidget* parent)
    : QWidget(parent)
    , m_actions(client)
    , m_list(new ActionList)
    , m_editor(new ActionEditor)
    , m_sample(new SampleRun(client, this))
    , m_unsupported(
          makeLabel(tr("This version of Paraphrase does not keep your own actions yet."), u"muted"_s))
{
    auto* column = new QVBoxLayout(this);
    column->setContentsMargins(0, 0, 0, 0);
    column->setSpacing(18);
    column->addLayout(pageHeading(tr("My actions"),
                                  tr("Your own actions, in your words. They appear under My actions in the "
                                     "window and the popup, and run on this computer like the others.")));
    column->addWidget(m_unsupported);
    column->addWidget(m_list);
    column->addWidget(m_editor);
    column->addStretch();
    m_editor->hide();

    connect(m_list, &ActionList::newClicked, this, &ActionsPage::startNew);
    connect(m_list, &ActionList::editClicked, this, &ActionsPage::edit);
    connect(m_list, &ActionList::deleteConfirmed, this, [this](const QString& id) {
        if (const QString problem = m_actions.remove(id); !problem.isEmpty()) {
            m_editor->showProblem(problem);
        }
        reload();
    });
    connect(m_editor, &ActionEditor::saveClicked, this, &ActionsPage::save);
    connect(m_editor, &ActionEditor::cancelClicked, m_editor, &QWidget::hide);
    connect(m_editor, &ActionEditor::tryClicked, this, &ActionsPage::tryOut);
    connect(m_sample, &SampleRun::finished, this,
            [this](const QString& output) { m_editor->showTryResult(output, false); });
    connect(m_sample, &SampleRun::failed, this,
            [this](const QString& message) { m_editor->showTryResult(message, true); });
}

void ActionsPage::reload()
{
    const bool supported = m_actions.load();
    m_unsupported->setVisible(!supported);
    m_list->setVisible(supported);
    m_list->setActions(m_actions.actions(), m_actions.max());
}

void ActionsPage::startNew()
{
    m_editor->edit({.id = {}, .skill = {}, .name = {}, .instruction = {}, .tier = u"fast"_s});
    m_editor->show();
}

void ActionsPage::edit(const QString& id)
{
    const auto& all = m_actions.actions();
    const auto it = std::ranges::find(all, id, &ActionInfo::id);
    if (it != all.end()) {
        m_editor->edit(*it);
        m_editor->show();
    }
}

void ActionsPage::save(const ActionInfo& action)
{
    if (const QString problem = m_actions.save(action); !problem.isEmpty()) {
        m_editor->showProblem(problem);
        return;
    }
    m_editor->hide();
    m_list->setActions(m_actions.actions(), m_actions.max());
}

void ActionsPage::tryOut(const ActionInfo& action, const QString& sample)
{
    // The action's template with its instruction: it works before the action is saved.
    if (action.instruction.isEmpty() || sample.trimmed().isEmpty()) {
        m_editor->showTryResult(tr("Say what the action does, then try it."), true);
        return;
    }
    m_editor->showTryResult(tr("Writing..."), false);
    (void)m_sample->run(m_actions.templateSkill(),
                        {{u"text"_s, sample}, {u"instruction"_s, action.instruction}},
                        {{u"tier"_s, action.tier}});
}

} // namespace paraphrase::app

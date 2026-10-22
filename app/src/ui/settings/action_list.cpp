#include "ui/settings/action_list.h"

#include "ui/common/widgets.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

ActionList::ActionList(QWidget* parent)
    : QFrame(parent)
    , m_rows(new QVBoxLayout)
    , m_count(makeLabel(QString(), u"small"_s))
    , m_new(makeButton(tr("New action")))
{
    setObjectName(u"card"_s);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(6);
    auto* head = new QHBoxLayout;
    head->addWidget(makeLabel(tr("Your actions"), u"h2"_s));
    head->addWidget(m_count);
    head->addStretch();
    m_new->setObjectName(u"newAction"_s);
    head->addWidget(m_new);
    layout->addLayout(head);
    m_rows->setSpacing(0);
    layout->addLayout(m_rows);
    connect(m_new, &QPushButton::clicked, this, &ActionList::newClicked);
}

void ActionList::setActions(const QList<ActionInfo>& actions, qsizetype max)
{
    while (const QLayoutItem* item = m_rows->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    m_count->setText(tr("%1 of %2").arg(actions.size()).arg(max));
    m_new->setEnabled(actions.size() < max);
    if (actions.isEmpty()) {
        QLabel* empty =
            makeLabel(tr("No actions yet. Make one for something you often ask: \"Turn this into a "
                         "tweet\", \"Explain this simply\"."),
                      u"muted"_s);
        empty->setWordWrap(true);
        m_rows->addWidget(empty);
    }
    for (const ActionInfo& a : actions) {
        m_rows->addWidget(row(a));
    }
}

QWidget* ActionList::row(const ActionInfo& action)
{
    auto* w = new QFrame;
    w->setObjectName(u"actionRow"_s);
    auto* layout = new QHBoxLayout(w);
    layout->setContentsMargins(4, 10, 4, 10);
    layout->setSpacing(12);
    auto* what = new QVBoxLayout;
    what->setSpacing(2);
    what->addWidget(makeLabel(u"<b>%1</b>"_s.arg(action.name.toHtmlEscaped())));
    QLabel* instruction = makeLabel(action.instruction, u"small"_s);
    instruction->setTextFormat(Qt::PlainText);
    instruction->setMaximumWidth(520);
    what->addWidget(instruction);
    layout->addLayout(what, 1);
    QLabel* tier = makeLabel(action.tier == u"quality"_s ? tr("Quality") : tr("Fast"), u"pill"_s);
    tier->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    layout->addWidget(tier);
    QPushButton* edit = makeButton(tr("Edit"), u"flat"_s);
    edit->setObjectName(u"edit-"_s + action.id);
    QPushButton* remove = makeButton(tr("Delete"), u"danger"_s);
    remove->setObjectName(u"delete-"_s + action.id);
    layout->addWidget(edit);
    layout->addWidget(remove);
    const QString id = action.id;
    connect(edit, &QPushButton::clicked, this, [this, id] { Q_EMIT editClicked(id); });
    // Asks once in place: the first click turns the button into the question.
    connect(remove, &QPushButton::clicked, this, [this, remove, id, name = action.name] {
        if (remove->property("asking").toBool()) {
            Q_EMIT deleteConfirmed(id);
            return;
        }
        remove->setProperty("asking", true);
        remove->setText(tr("Delete %1?").arg(name));
    });
    return w;
}

} // namespace paraphrase::app

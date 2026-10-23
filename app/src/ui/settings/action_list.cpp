#include "ui/settings/action_list.h"

#include "ui/common/widgets.h"

#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {
constexpr int kPillFontPx = 12;   // window_style.cpp: QLabel[role="pill"]
constexpr int kPillPaddingPx = 8; // its horizontal padding
} // namespace

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
        QWidget* r = row(a);
        r->setProperty("last", &a == &actions.last()); // .arow:last-child: no rule under it
        m_rows->addWidget(r);
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
    instruction->setToolTip(action.instruction);
    // settings-actions.html .arow: the text takes the spare width (one line, cut when long) and
    // the controls keep their size, so they line up from row to row.
    instruction->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    what->addWidget(instruction);
    layout->addLayout(what, 1);
    QLabel* tier = makeLabel(action.tier == u"quality"_s ? tr("Quality") : tr("Fast"), u"pill"_s);
    tier->setAlignment(Qt::AlignCenter);
    tier->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    {
        // One width for Fast and Quality, so Edit and Delete line up from row to row. The style
        // sheet that pads the pill applies only once the row is in the window, so its numbers are
        // used here: 12 px text, 8 px padding and a 1 px border on each side.
        QFont font = tier->font();
        font.setPixelSize(kPillFontPx);
        const QFontMetrics metrics(font);
        const int text =
            std::max(metrics.horizontalAdvance(tr("Fast")), metrics.horizontalAdvance(tr("Quality")));
        tier->setFixedWidth(text + (2 * (kPillPaddingPx + 1)));
    }
    layout->addWidget(tier);
    QPushButton* edit = makeButton(tr("Edit"), u"flat"_s);
    edit->setObjectName(u"edit-"_s + action.id);
    QPushButton* remove = makeButton(tr("Delete"), u"danger"_s);
    remove->setObjectName(u"delete-"_s + action.id);
    reserveWidth(remove, {tr("Delete"), tr("Delete?")});
    for (QPushButton* b : {edit, remove}) {
        b->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }
    layout->addWidget(edit);
    layout->addWidget(remove);
    const QString id = action.id;
    connect(edit, &QPushButton::clicked, this, [this, id] { Q_EMIT editClicked(id); });
    // Asks once in place: the first click turns the button into the question.
    connect(remove, &QPushButton::clicked, this, [this, remove, id] {
        if (remove->property("asking").toBool()) {
            Q_EMIT deleteConfirmed(id);
            return;
        }
        remove->setProperty("asking", true);
        remove->setText(tr("Delete?")); // on this action's own row, so the name is not repeated
    });
    return w;
}

} // namespace paraphrase::app

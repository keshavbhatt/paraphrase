#include "ui/settings/action_editor.h"

#include "ui/common/widgets.h"

#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QScrollBar>
#include <QStyle>
#include <QVBoxLayout>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kMaxName = 40;
constexpr int kInstructionHeight = 84;
constexpr int kSampleHeight = 80;
constexpr int kResultMaxHeight = 160; // a long result scrolls instead of stretching the card
constexpr int kStackGap = 6;          // settings-actions.html: .stack gap under a field
constexpr int kTryGap = 8;            // the Try it column and its button row

QLabel* fieldLabel(const QString& text)
{
    QLabel* l = makeLabel(text, u"small"_s);
    l->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    return l;
}

QLabel* hint(const QString& text)
{
    QLabel* l = makeLabel(text, u"small"_s);
    l->setWordWrap(true);
    return l;
}

} // namespace

ActionEditor::ActionEditor(QWidget* parent)
    : QFrame(parent)
    , m_title(makeLabel(QString(), u"h2"_s))
    , m_name(new QLineEdit)
    , m_instruction(new QPlainTextEdit)
    , m_tier(new SegmentedControl({
          {.name = u"tierFast"_s, .text = tr("Fast")},
          {.name = u"tierQuality"_s, .text = tr("Quality")},
      }))
    , m_sample(new QPlainTextEdit)
    , m_tryResult(makeLabel(QString(), u"small"_s))
    , m_tryArea(new QScrollArea)
    , m_problem(makeLabel(QString(), u"danger"_s))
{
    setObjectName(u"card"_s);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(18, 16, 18, 16);
    layout->setSpacing(14);
    layout->addWidget(m_title);
    auto* form = new QGridLayout;
    form->setHorizontalSpacing(16);
    form->setVerticalSpacing(12);
    form->setColumnMinimumWidth(0, 140);
    buildForm(form);
    layout->addLayout(form);
    m_problem->setWordWrap(true);
    layout->addWidget(m_problem);
    auto* buttons = new QHBoxLayout;
    buttons->addStretch();
    QPushButton* cancel = makeButton(tr("Cancel"));
    QPushButton* save = makeButton(tr("Save"), u"primary"_s);
    save->setObjectName(u"saveAction"_s);
    buttons->addWidget(cancel);
    buttons->addWidget(save);
    layout->addLayout(buttons);
    connect(cancel, &QPushButton::clicked, this, &ActionEditor::cancelClicked);
    connect(save, &QPushButton::clicked, this, [this] { Q_EMIT saveClicked(action()); });
}

void ActionEditor::buildForm(QGridLayout* form)
{
    m_name->setObjectName(u"actionName"_s);
    m_name->setMaxLength(kMaxName);
    m_instruction->setObjectName(u"actionInstruction"_s);
    m_instruction->setFixedHeight(kInstructionHeight);
    m_instruction->setPlaceholderText(tr("For example: Turn this into a tweet under 280 characters"));
    m_sample->setObjectName(u"actionSample"_s);
    m_sample->setFixedHeight(kSampleHeight);
    m_sample->setPlainText(
        tr("We moved the launch to March 12 because the payment provider needs one more week "
           "of testing. Sorry for the wait, everyone!"));
    m_tryResult->setObjectName(u"tryResult"_s);
    m_tryResult->setWordWrap(true);
    m_tryResult->setTextFormat(Qt::PlainText);
    m_tryResult->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_tryResult->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_tryArea->setObjectName(u"tryResultArea"_s);
    m_tryArea->setWidget(m_tryResult);
    m_tryArea->setWidgetResizable(true);
    m_tryArea->setFrameShape(QFrame::NoFrame);
    m_tryArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_tryArea->viewport()->setAutoFillBackground(false);
    m_tryResult->setAutoFillBackground(false);

    // Nested in a grid whose row and column gaps differ, a layout inherits no gap: set each.
    auto* what = new QVBoxLayout;
    what->setSpacing(kStackGap);
    what->addWidget(m_instruction);
    what->addWidget(hint(
        tr("Say what to do with the text, as you would ask a person. The text itself is added for you.")));
    auto* model = new QVBoxLayout;
    model->setSpacing(kStackGap);
    model->addWidget(m_tier, 0, Qt::AlignLeft);
    model->addWidget(hint(tr("Fast answers in a few seconds; Quality writes better and takes longer.")));
    auto* tryIt = new QVBoxLayout;
    tryIt->setSpacing(kTryGap);
    tryIt->addWidget(m_sample);
    auto* tryRow = new QHBoxLayout;
    tryRow->setSpacing(kTryGap);
    QPushButton* tryButton = makeButton(tr("Try it"));
    tryButton->setObjectName(u"tryAction"_s);
    tryRow->addWidget(tryButton, 0, Qt::AlignTop);
    tryRow->addWidget(m_tryArea, 1);
    tryIt->addLayout(tryRow);
    m_tryButtonHeight = tryButton->sizeHint().height();
    connect(tryButton, &QPushButton::clicked, this,
            [this] { Q_EMIT tryClicked(action(), m_sample->toPlainText()); });

    form->addWidget(fieldLabel(tr("Name")), 0, 0);
    form->addWidget(m_name, 0, 1);
    form->addWidget(fieldLabel(tr("What it does")), 1, 0);
    form->addLayout(what, 1, 1);
    form->addWidget(fieldLabel(tr("Model")), 2, 0);
    form->addLayout(model, 2, 1);
    form->addWidget(fieldLabel(tr("Try it")), 3, 0);
    form->addLayout(tryIt, 3, 1);
}

void ActionEditor::edit(const ActionInfo& action)
{
    m_id = action.id;
    m_title->setText(action.id.isEmpty() ? tr("New action") : tr("Edit \"%1\"").arg(action.name));
    m_name->setText(action.name);
    m_instruction->setPlainText(action.instruction);
    m_tier->setCurrent(action.tier == u"quality"_s ? 1 : 0);
    m_tryResult->clear();
    fitTryResult();
    m_problem->clear();
    m_name->setFocus();
}

ActionInfo ActionEditor::action() const
{
    return {
        .id = m_id,
        .skill = {},
        .name = m_name->text().trimmed(),
        .instruction = m_instruction->toPlainText().trimmed(),
        .tier = m_tier->current() == 1 ? u"quality"_s : u"fast"_s,
    };
}

void ActionEditor::showTryResult(const QString& text, bool problem)
{
    m_tryResult->setText(text);
    m_tryResult->setProperty("role", problem ? u"danger"_s : u"small"_s);
    m_tryResult->style()->unpolish(m_tryResult);
    m_tryResult->style()->polish(m_tryResult);
    fitTryResult();
    m_tryArea->verticalScrollBar()->setValue(0);
}

void ActionEditor::resizeEvent(QResizeEvent* event)
{
    QFrame::resizeEvent(event);
    fitTryResult();
}

void ActionEditor::fitTryResult()
{
    // At least the button's height, so a one-line result sits centred beside it as in the mock.
    const int width = m_tryArea->viewport()->width();
    const int text = m_tryResult->text().isEmpty() ? 0 : m_tryResult->heightForWidth(width);
    m_tryArea->setFixedHeight(std::clamp(text, m_tryButtonHeight, kResultMaxHeight));
}

void ActionEditor::showProblem(const QString& message)
{
    m_problem->setText(message);
}

} // namespace paraphrase::app

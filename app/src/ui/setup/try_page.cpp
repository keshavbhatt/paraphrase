#include "ui/setup/try_page.h"

#include "ui/common/widgets.h"
#include "ui/common/window_style.h"
#include "ui/setup/sample_run.h"

#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

TryPage::TryPage(SampleRun& run, QWidget* parent)
    : QWidget(parent)
    , m_run(run)
    , m_sample(new QLineEdit(tr("Their going to the market tomorow, dont be late.")))
    , m_button(makeButton(tr("Proofread")))
    , m_answer(makeLabel(tr("The corrected sentence appears here."), u"muted"_s))
{
    auto* col = new QVBoxLayout(this);
    col->setContentsMargins(24, 22, 24, 22);
    col->setSpacing(12);
    col->addWidget(makeLabel(tr("Try it"), u"h1"_s));
    col->addWidget(makeLabel(tr("Proofread one sentence, or write your own."), u"muted"_s));
    auto* row = new QHBoxLayout;
    m_sample->setObjectName(u"sample"_s);
    row->addWidget(m_sample, 1);
    m_button->setObjectName(u"tryIt"_s);
    m_button->setProperty("size", u"big"_s);
    row->addWidget(m_button);
    col->addLayout(row);
    auto* card = new QFrame;
    card->setObjectName(u"card"_s);
    auto* cardCol = new QVBoxLayout(card);
    cardCol->setContentsMargins(18, 14, 18, 14);
    m_answer->setObjectName(u"tryResult"_s);
    m_answer->setWordWrap(true);
    m_answer->setTextInteractionFlags(Qt::TextSelectableByMouse);
    cardCol->addWidget(m_answer);
    col->addWidget(card);
    col->addStretch();
    connect(m_button, &QPushButton::clicked, this, &TryPage::tryIt);
    connect(m_sample, &QLineEdit::returnPressed, this, &TryPage::tryIt);
    connect(&m_run, &SampleRun::finished, this, [this](const QString& text) { showAnswer(text, {}); });
    connect(&m_run, &SampleRun::failed, this,
            [this](const QString& message) { showAnswer(message, u"danger"_s); });
}

void TryPage::tryIt()
{
    if (m_run.run(u"proofread"_s, m_sample->text())) {
        m_button->setEnabled(false);
        showAnswer(tr("Writing..."), u"muted"_s);
    }
}

void TryPage::showAnswer(const QString& text, const QString& role)
{
    m_button->setEnabled(!m_run.running());
    m_answer->setText(text);
    m_answer->setProperty("role", role);
    m_answer->setStyleSheet(u"font-size: 15px;"_s);
    repolish(m_answer);
}

} // namespace paraphrase::app

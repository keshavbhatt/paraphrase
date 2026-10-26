#include "ui/main/input_pane.h"

#include "ui/common/chip_button.h"
#include "ui/core/languages.h"
#include "ui/parts/input_pages.h"
#include "util/language_detector.h"

#include <QApplication>
#include <QClipboard>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QStyle>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kRunWidth = 236; ///< main-window-run.html .run: one width whatever the label

} // namespace

InputPane::InputPane(const util::LanguageDetector& detector, QWidget* parent)
    : Pane(parent)
    , m_detector(detector)
    , m_pages(new QStackedWidget)
    , m_full(new FullReplyInput)
    , m_compose(new ComposeInput)
{
    buildHead();
    m_byKind.insert(SkillKind::Text, new TextInput);
    m_byKind.insert(SkillKind::QuickReply, new QuickReplyInput);
    m_byKind.insert(SkillKind::FullReply, m_full);
    m_byKind.insert(SkillKind::Compose, m_compose);
    for (const SkillKind kind :
         {SkillKind::Text, SkillKind::QuickReply, SkillKind::FullReply, SkillKind::Compose}) {
        InputPage* p = m_byKind.value(kind);
        m_pages->addWidget(p);
        connect(p, &InputPage::edited, this, [this] {
            updateInfo();
            updateHint();
            updateRun();
            Q_EMIT edited();
        });
    }
    setBody(m_pages);
    m_hint = makeLabel(QString(), u"small"_s);
    m_hint->setWordWrap(true);
    m_run = new ChipButton(QString(), tr("Ctrl Enter"), ChipButton::ChipSide::After);
    m_run->setObjectName(u"run"_s);
    m_run->setProperty("btnSize", u"big"_s);
    footRow()->addWidget(m_hint, 1);
    footRow()->addWidget(m_run);
    connect(m_run, &QPushButton::clicked, this, &InputPane::runClicked);
    showKind(SkillKind::Text);
}

void InputPane::buildHead()
{
    m_title = makeLabel(QString(), u"caption"_s);
    m_info = makeLabel(QString(), u"small"_s);
    m_pill = makeLabel(QString(), u"pill"_s);
    m_pill->setObjectName(u"langPill"_s);
    m_replyKind = new SegmentedControl({
        {.name = u"quickReplies"_s, .text = tr("Quick replies")},
        {.name = u"fullReply"_s, .text = tr("Full reply")},
    });
    m_paste = makeButton(tr("Paste"), u"flat"_s);
    m_clear = makeButton(tr("Clear"), u"flat"_s);
    head()->addWidget(m_title);
    head()->addWidget(m_info);
    head()->addStretch();
    head()->addWidget(m_pill, 0, Qt::AlignVCenter);
    head()->addWidget(m_replyKind);
    head()->addWidget(m_paste);
    head()->addWidget(m_clear);
    connect(m_replyKind, &SegmentedControl::changed, this,
            [this](int index) { Q_EMIT fullReplyToggled(index == 1); });
    connect(m_paste, &QPushButton::clicked, this,
            [this] { page()->setSubject(QApplication::clipboard()->text()); });
    connect(m_clear, &QPushButton::clicked, this, [this] {
        for (InputPage* p : std::as_const(m_byKind)) {
            p->clear();
        }
    });
}

void InputPane::showKind(SkillKind kind)
{
    // The text moves with the user, so switching actions keeps what was typed.
    const QString carried = page() != nullptr ? page()->subject() : QString();
    m_kind = kind;
    InputPage* next = m_byKind.value(kind);
    if (next->subject().isEmpty() && !carried.isEmpty()) {
        next->setSubject(carried);
    }
    m_pages->setCurrentWidget(next);
    const bool text = kind == SkillKind::Text;
    const bool compose = kind == SkillKind::Compose;
    if (compose) {
        m_title->setText(tr("WHAT TO WRITE"));
    } else {
        m_title->setText(text ? tr("YOUR TEXT") : tr("MESSAGE YOU GOT"));
    }
    // main-window-reply.html: the language as a pill, no word count, no Clear.
    m_clear->setVisible(text);
    m_paste->setVisible(!page()->showsLanguage());
    updateHint();
    updateRun();
    m_replyKind->setVisible(!text && !compose && m_fullAvailable);
    m_replyKind->setCurrent(kind == SkillKind::FullReply ? 1 : 0);
    updateInfo();
}

InputPage* InputPane::page() const
{
    return qobject_cast<InputPage*>(m_pages->currentWidget());
}

bool InputPane::isEmpty() const
{
    return page()->subject().trimmed().isEmpty();
}

void InputPane::focusField() const
{
    const auto fields = page()->findChildren<QPlainTextEdit*>();
    const auto first = std::ranges::find_if(fields, [](const QPlainTextEdit* f) { return !f->isReadOnly(); });
    if (first != fields.end()) {
        (*first)->setFocus();
    }
}

void InputPane::setRunAction(const QString& label, const QIcon& onFill, const QIcon& plain)
{
    m_runLabel = label;
    m_runIcon = onFill;
    m_runPlainIcon = plain;
    updateRun();
}

void InputPane::setRunState(bool primary, bool busy)
{
    m_runPrimary = primary;
    m_busy = busy;
    updateRun();
}

void InputPane::updateHint()
{
    // main-window-run.html note 2: the hint says what is missing, never the keys.
    const bool empty = isEmpty();
    if (m_kind == SkillKind::Compose) {
        m_hint->setText(empty ? tr("Say what to write first. Names, dates and numbers come only from what "
                                   "you wrote.")
                              : tr("Names, dates and numbers come only from what you wrote."));
    } else if (m_kind == SkillKind::FullReply) {
        m_hint->setText(empty ? tr("Paste the message you got first.")
                              : tr("Leave \"What do you want to say?\" empty for a reply that simply "
                                   "answers."));
    } else if (m_kind == SkillKind::QuickReply) {
        m_hint->setText(empty ? tr("Paste the message you got first.") : QString());
    } else {
        m_hint->setText(empty ? tr("Type or paste some text first.")
                              : tr("Type or paste anything, then run the action."));
    }
}

void InputPane::updateRun()
{
    if (m_run == nullptr) {
        return;
    }
    m_run->setEnabled(!m_busy && !isEmpty());
    const bool fill = m_runPrimary && m_run->isEnabled();
    m_run->setProperty("kind", m_runPrimary ? u"primary"_s : QString());
    m_run->style()->unpolish(m_run);
    m_run->style()->polish(m_run);
    m_run->setLeadingIcon(fill ? m_runIcon : m_runPlainIcon);

    // One width for every label (main-window-run.html note 1): room for the longest of the
    // fixed ones, and a label that would not fit is cut with "..." and shown whole as a tip.
    const auto* label = m_run->findChild<QLabel*>(u"label"_s);
    m_run->setLabel(m_runLabel);
    label->ensurePolished(); // the theme's font, not the default one
    const QFontMetrics metrics(label->font());
    const int parts = m_run->sizeHint().width() - metrics.horizontalAdvance(m_runLabel);
    // Measured in the bolder weight, so the filled and the plain button share one width.
    QFont bold = label->font();
    bold.setWeight(QFont::DemiBold);
    const QFontMetrics boldMetrics(bold);
    int longest = 0;
    for (const QString& fixed : {tr("Write the reply"), tr("Write 3 replies"), tr("Summarize")}) {
        longest = std::max(longest, boldMetrics.horizontalAdvance(fixed));
    }
    const int width = std::max(kRunWidth, parts + longest);
    m_run->setFixedWidth(width);
    const QString shown = metrics.elidedText(m_runLabel, Qt::ElideRight, width - parts);
    m_run->setLabel(shown);
    m_run->setAccessibleName(m_runLabel);
    m_run->setToolTip(shown == m_runLabel ? QString() : m_runLabel);
}

QString InputPane::language() const
{
    return m_detector.detect(page()->subject());
}

void InputPane::setFullReplyAvailable(bool available)
{
    m_fullAvailable = available;
    m_replyKind->setVisible(m_kind != SkillKind::Text && available);
}

void InputPane::setFullReply(bool full)
{
    m_replyKind->setCurrent(full ? 1 : 0);
}

void InputPane::updateInfo()
{
    const QString text = page()->subject();
    const QString name = text.trimmed().isEmpty() ? QString() : languageName(m_detector, language());
    if (page()->showsLanguage()) {
        m_info->clear();
        m_pill->setVisible(false);
        QString shown;
        if (!text.trimmed().isEmpty()) {
            shown = name.isEmpty() ? tr("Best effort") : name;
        }
        page()->setLanguage(shown);
        return;
    }
    if (m_kind != SkillKind::Text) {
        m_info->clear();
        m_pill->setText(name.isEmpty() ? tr("Best effort") : name);
        m_pill->setVisible(!text.trimmed().isEmpty());
        return;
    }
    m_pill->setVisible(false);
    if (text.trimmed().isEmpty()) {
        m_info->clear();
        return;
    }
    const auto words = text.split(QRegularExpression(u"\\s+"_s), Qt::SkipEmptyParts).size();
    // Plurals written out: tr()'s %n needs a loaded translation to pluralise.
    const QString count = words == 1 ? tr("1 word") : tr("%1 words").arg(words);
    m_info->setText(name.isEmpty() ? u"· "_s + count : u"· %1 · %2"_s.arg(count, name));
}

} // namespace paraphrase::app

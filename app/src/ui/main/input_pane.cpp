#include "ui/main/input_pane.h"

#include "ui/core/languages.h"
#include "ui/parts/input_pages.h"
#include "util/language_detector.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QStackedWidget>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

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
            Q_EMIT edited();
        });
    }
    setBody(m_pages);
    m_hint = makeLabel(QString(), u"small"_s);
    m_hint->setWordWrap(true);
    footRow()->addWidget(m_hint);
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
        m_hint->setText(tr("Ctrl+Enter writes it. Names, dates and numbers come only from what you wrote."));
    } else if (kind == SkillKind::FullReply) {
        m_title->setText(tr("MESSAGE YOU GOT"));
        m_hint->setText(
            tr("Ctrl+Enter writes the reply. Leave \"What do you want to say?\" empty for a reply "
               "that simply answers the message."));
    } else {
        m_title->setText(text ? tr("YOUR TEXT") : tr("MESSAGE YOU GOT"));
        m_hint->setText(tr("Type or paste anything. Ctrl+Enter runs the selected action."));
    }
    // main-window-reply.html: the language as a pill, no word count, no Clear, no hint line;
    // main-window-reply-draft.html keeps the hint for a full reply.
    m_clear->setVisible(text);
    m_paste->setVisible(!page()->showsLanguage());
    foot()->setVisible(kind != SkillKind::QuickReply);
    m_replyKind->setVisible(!text && !compose && m_fullAvailable);
    m_replyKind->setCurrent(kind == SkillKind::FullReply ? 1 : 0);
    updateInfo();
}

InputPage* InputPane::page() const
{
    return qobject_cast<InputPage*>(m_pages->currentWidget());
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

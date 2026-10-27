#include "ui/parts/input_pages.h"

#include "ui/common/grip_splitter.h"
#include "ui/common/text_field.h"
#include "ui/common/widgets.h"
#include "ui/parts/tone_picker.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QSettings>
#include <QSignalBlocker>
#include <QVBoxLayout>

#include <algorithm>
#include <array>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kMessageMinHeight = 94; ///< three lines (main-window-reply.html, note 5)
constexpr int kDraftHeight = 70;      ///< two lines
/// The split between them: the pane's 10px gaps on both sides of an invisible handle.
constexpr int kSplitHandle = 20;
constexpr int kDraftLabelGap = 6;
const QString kReplySplitKey = u"reply/split"_s;
constexpr int kIntentHeight = 76;
const QString kKindKey = u"reply/kind"_s;
const QString kToneKey = u"reply/tone"_s;
const QString kComposeKindKey = u"compose/kind"_s;
const QString kComposeToneKey = u"compose/tone"_s;
/// The compose presets, in the order of the switch (main-window-compose.html).
constexpr std::array<QStringView, 4> kComposeKinds = {u"email", u"message", u"post", u"note"};

TextField* field(const QString& name, const QString& placeholder,
                 int lineHeight = TextField::kFieldLineHeight)
{
    auto* edit = new TextField(lineHeight);
    edit->setObjectName(name);
    edit->setPlaceholderText(placeholder);
    // A long link or word wraps instead of scrolling sideways.
    edit->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    return edit;
}

QVBoxLayout* pageLayout(QWidget* page)
{
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);
    return layout;
}

bool filled(const QString& s)
{
    return !s.trimmed().isEmpty();
}

} // namespace

TextInput::TextInput(QWidget* parent)
    : InputPage(parent)
    , m_text(field(u"text"_s, tr("Type or paste your text"), TextField::kTextLineHeight))
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 12, 14, 12);
    m_text->document()->setDocumentMargin(0); // the page's margins are the whole inset
    layout->addWidget(m_text);
    connect(m_text, &QTextEdit::textChanged, this, &InputPage::edited);
}

QString TextInput::subject() const
{
    return m_text->toPlainText();
}

void TextInput::setSubject(const QString& text)
{
    m_text->setPlainText(text);
}

QVariantMap TextInput::input(const SkillInfo& skill, const QString& targetLang) const
{
    if (!filled(subject())) {
        return {};
    }
    QVariantMap in{{u"text"_s, subject()}};
    // A skill that writes in a chosen language always names it; for the others it is optional.
    if (skill.takes(u"target_lang"_s) && !targetLang.isEmpty()) {
        in.insert(u"target_lang"_s, targetLang);
    }
    return in;
}

void TextInput::clear()
{
    m_text->clear();
}

QuickReplyInput::QuickReplyInput(QWidget* parent)
    : InputPage(parent)
    , m_message(field(u"message"_s, tr("Paste the message you got")))
    , m_draft(field(u"draft"_s, tr("Start typing and the replies will continue it")))
    , m_split(new GripSplitter(Qt::Vertical))
{
    QVBoxLayout* layout = pageLayout(this);
    // main-window-reply.html: the message takes the pane's height; a split between it and the
    // reply so far makes either taller, and keeps its place for the next start.
    m_message->setMinimumHeight(kMessageMinHeight);
    auto* draftBox = new QWidget;
    auto* draftColumn = new QVBoxLayout(draftBox);
    draftColumn->setContentsMargins(0, 0, 0, 0);
    draftColumn->setSpacing(kDraftLabelGap);
    draftColumn->addWidget(makeLabel(tr("Your reply so far (optional)"), u"small"_s));
    m_draft->setMinimumHeight(kDraftHeight);
    draftColumn->addWidget(m_draft, 1);
    m_split->setObjectName(u"replySplit"_s);
    m_split->setChildrenCollapsible(false);
    m_split->setHandleWidth(kSplitHandle);
    m_split->addWidget(m_message);
    m_split->addWidget(draftBox);
    m_split->setStretchFactor(0, 1);
    m_split->setStretchFactor(1, 0);
    if (!m_split->restoreState(QSettings().value(kReplySplitKey).toByteArray())) {
        m_split->setSizes({QWIDGETSIZE_MAX, 0}); // the reply so far at its smallest
    }
    layout->addWidget(m_split, 1);
    connect(m_split, &QSplitter::splitterMoved, this,
            [this] { QSettings().setValue(kReplySplitKey, m_split->saveState()); });
    for (const TextField* edit : {m_message, m_draft}) {
        connect(edit, &QTextEdit::textChanged, this, &InputPage::edited);
    }
}

QString QuickReplyInput::subject() const
{
    return m_message->toPlainText();
}

void QuickReplyInput::setSubject(const QString& text)
{
    m_message->setPlainText(text);
}

QVariantMap QuickReplyInput::input(const SkillInfo& /*skill*/, const QString& /*targetLang*/) const
{
    if (!filled(subject())) {
        return {};
    }
    QVariantMap in{{u"context"_s, subject()}};
    if (const QString draft = m_draft->toPlainText(); filled(draft)) {
        in.insert(u"text"_s, draft); // every reply continues it
    }
    return in;
}

void QuickReplyInput::clear()
{
    m_message->clear();
    m_draft->clear();
}

FullReplyInput::FullReplyInput(QWidget* parent)
    : InputPage(parent)
    , m_message(field(u"fullMessage"_s, tr("Paste the message you got")))
    , m_intent(field(u"intent"_s, tr("For example: yes, but Friday morning works better")))
    , m_kind(new SegmentedControl({
          {.name = u"asEmail"_s, .text = tr("Email")},
          {.name = u"asChat"_s, .text = tr("Chat message")},
      }))
    , m_tone(new TonePicker(kToneKey, u"professional"_s))
{
    QVBoxLayout* layout = pageLayout(this);
    // main-window-reply-draft.html: the language and Paste are in the pane's header, where
    // Quick replies has them, so switching between the two moves nothing.
    layout->addWidget(m_message, 1);
    layout->addWidget(makeLabel(tr("What do you want to say?"), u"small"_s));
    m_intent->setFixedHeight(kIntentHeight);
    layout->addWidget(m_intent);
    auto* options = new QHBoxLayout;
    options->setSpacing(8);
    options->addWidget(makeLabel(tr("Write it as"), u"small"_s));
    options->addWidget(m_kind);
    options->addSpacing(8);
    options->addWidget(makeLabel(tr("Tone"), u"small"_s));
    options->addWidget(m_tone);
    options->addStretch();
    layout->addLayout(options);

    m_kind->setCurrent(QSettings().value(kKindKey).toString() == u"chat"_s ? 1 : 0);
    connect(m_kind, &SegmentedControl::changed, this, [this] {
        QSettings().setValue(kKindKey, chat() ? u"chat"_s : u"email"_s);
        Q_EMIT kindChanged();
    });
    connect(m_tone, &TonePicker::changed, this, &FullReplyInput::toneChanged);
    for (const TextField* edit : {m_message, m_intent}) {
        connect(edit, &QTextEdit::textChanged, this, &InputPage::edited);
    }
}

void FullReplyInput::takeMessage(const QString& text)
{
    setSubject(text);
    m_intent->clear();
    if (!QSettings().contains(kKindKey)) {
        // An email has paragraphs or opens with a greeting; a chat message is a line or two.
        static const QRegularExpression greeting(
            u"^\\s*(hi|hello|dear|hey|good (morning|afternoon|evening))\\b"_s,
            QRegularExpression::CaseInsensitiveOption);
        const bool email = text.trimmed().contains(u"\n\n"_s) || greeting.match(text).hasMatch();
        const QSignalBlocker quiet(m_kind); // not the user's choice: nothing is remembered
        m_kind->setCurrent(email ? 0 : 1);
    }
}

void FullReplyInput::focusIntent() const
{
    m_intent->setFocus();
}

QString FullReplyInput::subject() const
{
    return m_message->toPlainText();
}

void FullReplyInput::setSubject(const QString& text)
{
    m_message->setPlainText(text);
}

QVariantMap FullReplyInput::input(const SkillInfo& skill, const QString& /*targetLang*/) const
{
    if (!filled(subject())) {
        return {};
    }
    QVariantMap in{{u"context"_s, subject()}};
    if (const QString intent = m_intent->toPlainText(); filled(intent) && skill.takes(u"intent"_s)) {
        in.insert(u"intent"_s, intent);
    }
    if (!m_tone->tone().isEmpty() && skill.takes(u"tone"_s)) {
        in.insert(u"tone"_s, m_tone->tone());
    }
    return in;
}

void FullReplyInput::clear()
{
    m_message->clear();
    m_intent->clear();
}

void FullReplyInput::setTones(const QList<std::pair<QString, QString>>& tones)
{
    m_tone->setTones(tones);
}

void FullReplyInput::applyTheme(const Tokens& t)
{
    m_tone->applyTheme(t);
}

bool FullReplyInput::chat() const
{
    return m_kind->current() == 1;
}

ComposeInput::ComposeInput(QWidget* parent)
    : InputPage(parent)
    , m_description(field(u"composeText"_s, tr("For example: tell the team the office is closed on Monday")))
    , m_kind(new SegmentedControl({
          {.name = u"composeEmail"_s, .text = tr("Email")},
          {.name = u"composeMessage"_s, .text = tr("Message")},
          {.name = u"composePost"_s, .text = tr("Social post")},
          {.name = u"composeNote"_s, .text = tr("Note")},
      }))
    , m_tone(new TonePicker(kComposeToneKey, u"friendly"_s))
{
    QVBoxLayout* layout = pageLayout(this);
    layout->addWidget(
        makeLabel(tr("Describe it in your own words, in the language you want it in"), u"small"_s));
    layout->addWidget(m_description, 1);
    auto* kinds = new QHBoxLayout;
    kinds->setSpacing(8);
    kinds->addWidget(makeLabel(tr("Write it as"), u"small"_s));
    kinds->addWidget(m_kind);
    kinds->addStretch();
    layout->addLayout(kinds);
    auto* tones = new QHBoxLayout;
    tones->setSpacing(8);
    tones->addWidget(makeLabel(tr("Tone"), u"small"_s));
    tones->addWidget(m_tone);
    tones->addStretch();
    layout->addLayout(tones);
    const QString saved = QSettings().value(kComposeKindKey).toString();
    const auto* const found = std::ranges::find(kComposeKinds, saved);
    m_kind->setCurrent(found == kComposeKinds.end() ? 0 : static_cast<int>(found - kComposeKinds.begin()));
    connect(m_kind, &SegmentedControl::changed, this, [this] {
        QSettings().setValue(kComposeKindKey, kind());
        Q_EMIT kindChanged();
    });
    connect(m_tone, &TonePicker::changed, this, &ComposeInput::toneChanged);
    connect(m_description, &QTextEdit::textChanged, this, &InputPage::edited);
}

QString ComposeInput::subject() const
{
    return m_description->toPlainText();
}

void ComposeInput::setSubject(const QString& text)
{
    m_description->setPlainText(text);
}

QVariantMap ComposeInput::input(const SkillInfo& skill, const QString& /*targetLang*/) const
{
    if (!filled(subject())) {
        return {};
    }
    QVariantMap in{{u"text"_s, subject()}};
    if (!m_tone->tone().isEmpty() && skill.takes(u"tone"_s)) {
        in.insert(u"tone"_s, m_tone->tone());
    }
    return in;
}

void ComposeInput::clear()
{
    m_description->clear();
}

void ComposeInput::setTones(const QList<std::pair<QString, QString>>& tones)
{
    m_tone->setTones(tones);
}

void ComposeInput::applyTheme(const Tokens& t)
{
    m_tone->applyTheme(t);
}

QString ComposeInput::kind() const
{
    const auto i = static_cast<std::size_t>(
        std::clamp(m_kind->current(), 0, static_cast<int>(kComposeKinds.size()) - 1));
    return kComposeKinds.at(i).toString();
}

} // namespace paraphrase::app

#include "input_pages.h"

#include "widgets.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kMessageHeight = 120; ///< four lines (main-window-reply.html)
constexpr int kDraftHeight = 70;    ///< two lines
constexpr int kIntentHeight = 76;
const QString kKindKey = u"reply/kind"_s;
const QString kToneKey = u"reply/tone"_s;

QPlainTextEdit* field(const QString& name, const QString& placeholder)
{
    auto* edit = new QPlainTextEdit;
    edit->setObjectName(name);
    edit->setPlaceholderText(placeholder);
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
    , m_text(field(u"text"_s, tr("Type or paste your text")))
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->addWidget(m_text);
    connect(m_text, &QPlainTextEdit::textChanged, this, &InputPage::edited);
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
{
    QVBoxLayout* layout = pageLayout(this);
    m_message->setFixedHeight(kMessageHeight);
    layout->addWidget(m_message);
    layout->addWidget(makeLabel(tr("Your reply so far (optional)"), u"small"_s));
    m_draft->setFixedHeight(kDraftHeight);
    layout->addWidget(m_draft);
    layout->addStretch();
    for (const QPlainTextEdit* edit : {m_message, m_draft}) {
        connect(edit, &QPlainTextEdit::textChanged, this, &InputPage::edited);
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
    , m_tone(makeButton(QString()))
{
    QVBoxLayout* layout = pageLayout(this);
    layout->addWidget(makeLabel(tr("The email or chat you are answering"), u"small"_s));
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
    m_tone->setObjectName(u"toneChoice"_s);
    m_tone->setMenu(new QMenu(m_tone));
    options->addWidget(m_tone);
    options->addStretch();
    layout->addLayout(options);

    m_kind->setCurrent(QSettings().value(kKindKey).toString() == u"chat"_s ? 1 : 0);
    connect(m_kind, &SegmentedControl::changed, this, [this] {
        QSettings().setValue(kKindKey, chat() ? u"chat"_s : u"email"_s);
        Q_EMIT kindChanged();
    });
    for (const QPlainTextEdit* edit : {m_message, m_intent}) {
        connect(edit, &QPlainTextEdit::textChanged, this, &InputPage::edited);
    }
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
    if (!m_toneValue.isEmpty() && skill.takes(u"tone"_s)) {
        in.insert(u"tone"_s, m_toneValue);
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
    QMenu* menu = m_tone->menu();
    menu->clear();
    const QString saved = QSettings().value(kToneKey, u"professional"_s).toString();
    for (const auto& [tone, title] : tones) {
        QAction* item = menu->addAction(title);
        item->setObjectName(u"tone-"_s + tone);
        connect(item, &QAction::triggered, this, [this, tone, title] { chooseTone(tone, title); });
        if (tone == saved || m_toneValue.isEmpty()) {
            m_toneValue = tone;
            m_tone->setText(title);
        }
    }
    m_tone->setVisible(!tones.isEmpty());
}

void FullReplyInput::chooseTone(const QString& tone, const QString& title)
{
    m_toneValue = tone;
    m_tone->setText(title);
    QSettings().setValue(kToneKey, tone);
}

void FullReplyInput::applyTheme(const Tokens& t)
{
    // The menu arrow after the tone's name, as on the actions.
    m_tone->setLayoutDirection(Qt::RightToLeft);
    m_tone->setIcon(Theme::icon(u"down"_s, t.muted, 14));
}

bool FullReplyInput::chat() const
{
    return m_kind->current() == 1;
}

} // namespace paraphrase::app

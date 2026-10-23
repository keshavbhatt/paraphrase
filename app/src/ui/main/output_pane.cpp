#include "ui/main/output_pane.h"

#include "ui/common/chip_button.h"
#include "ui/parts/result_views.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kIconSize = 14;

struct ButtonTexts
{
    QString copy;
    QString secondary;
    QString retry;
};

ButtonTexts textsFor(SkillKind kind, bool running)
{
    switch (kind) {
    case SkillKind::QuickReply:
        return {
            .copy = OutputPane::tr("Copy reply"),
            .secondary = OutputPane::tr("3 new ones"),
            .retry = OutputPane::tr("Make it polite"),
        };
    case SkillKind::Compose:
        return {
            .copy = OutputPane::tr("Copy"),
            .secondary = OutputPane::tr("Another version"),
            .retry = running ? OutputPane::tr("Stop") : OutputPane::tr("Shorter"),
        };
    case SkillKind::FullReply:
        return {
            .copy = OutputPane::tr("Copy reply"),
            .secondary = OutputPane::tr("Another version"),
            .retry = running ? OutputPane::tr("Stop") : OutputPane::tr("Shorter"),
        };
    case SkillKind::Text:
        break;
    }
    return {
        .copy = OutputPane::tr("Copy result"),
        .secondary = OutputPane::tr("Use as my text"),
        .retry = running ? OutputPane::tr("Stop") : OutputPane::tr("Retry"),
    };
}

} // namespace

OutputPane::OutputPane(QWidget* parent)
    : Pane(parent)
    , m_pages(new QStackedWidget)
    , m_text(new TextResultView)
    , m_replies(new QuickRepliesView)
    , m_full(new FullReplyView)
{
    buildHead();
    m_pages->addWidget(m_text);
    m_pages->addWidget(m_replies);
    m_pages->addWidget(m_full);
    setBody(m_pages);
    buildFoot();
    connect(m_full, &FullReplyView::versionsChanged, this, &OutputPane::updateVersions);
    showKind(SkillKind::Text, QString(), false);
}

void OutputPane::buildHead()
{
    m_title = makeLabel(QString(), u"caption"_s);
    m_info = makeLabel(QString(), u"small"_s);
    m_info->setObjectName(u"outInfo"_s);
    m_view = new SegmentedControl({
        {.name = u"showChanges"_s, .text = tr("Changes")},
        {.name = u"showResult"_s, .text = tr("Result")},
    });
    m_versionInfo = makeLabel(QString(), u"small"_s);
    m_versionInfo->setObjectName(u"versionInfo"_s);
    m_prev = makeButton(u"‹"_s, u"flat"_s);
    m_next = makeButton(u"›"_s, u"flat"_s);
    m_prev->setAccessibleName(tr("Previous version"));
    m_next->setAccessibleName(tr("Next version"));
    head()->addWidget(m_title);
    head()->addWidget(m_info, 1);
    head()->addWidget(m_view);
    head()->addWidget(m_versionInfo);
    for (QPushButton* b : {m_prev, m_next}) {
        b->setFixedSize(30, 28);
        head()->addWidget(b);
    }
    connect(m_view, &SegmentedControl::changed, this,
            [this](int index) { m_text->setShowResult(index == 1); });
    connect(m_prev, &QPushButton::clicked, this, [this] { m_full->showVersion(m_full->versionIndex() - 1); });
    connect(m_next, &QPushButton::clicked, this, [this] { m_full->showVersion(m_full->versionIndex() + 1); });
}

void OutputPane::buildFoot()
{
    m_copy = new ChipButton(QString(), tr("Ctrl Shift C"), ChipButton::ChipSide::After);
    m_copy->setProperty("kind", u"primary"_s);
    m_copy->setProperty("btnSize", u"big"_s);
    m_copy->setObjectName(u"copy"_s);
    m_secondary = makeButton(QString());
    m_retry = makeButton(QString());
    for (QPushButton* b : {m_secondary, m_retry}) {
        b->setProperty("btnSize", u"big"_s);
    }
    footRow()->addWidget(m_copy);
    footRow()->addWidget(m_secondary);
    footRow()->addWidget(m_retry);
    footRow()->addStretch();
    connect(m_copy, &QPushButton::clicked, this, &OutputPane::copyClicked);
    connect(m_secondary, &QPushButton::clicked, this, &OutputPane::secondaryClicked);
    connect(m_retry, &QPushButton::clicked, this, &OutputPane::retryClicked);
}

void OutputPane::showKind(SkillKind kind, const QString& title, bool proofread)
{
    m_kind = kind;
    if (kind == SkillKind::Text) {
        m_pages->setCurrentWidget(m_text);
    } else if (kind == SkillKind::QuickReply) {
        m_pages->setCurrentWidget(m_replies);
    } else {
        m_pages->setCurrentWidget(m_full); // a full reply and Write share the draft view
        m_full->setComposing(kind == SkillKind::Compose);
    }
    m_title->setText(title.toUpper());
    m_view->setVisible(proofread);
    if (proofread) {
        m_view->setCurrent(0);
        m_text->setShowResult(false);
    }
    updateVersions();
    updateButtons();
}

void OutputPane::setInfo(const QString& text, const QString& role)
{
    m_info->setText(text);
    m_info->setProperty("role", role);
    m_info->style()->unpolish(m_info);
    m_info->style()->polish(m_info);
}

QString OutputPane::info() const
{
    return m_info->text();
}

void OutputPane::setRunning(bool running)
{
    m_running = running;
    updateButtons();
}

QString OutputPane::result() const
{
    switch (m_kind) {
    case SkillKind::QuickReply:
        return m_replies->chosen();
    case SkillKind::FullReply:
    case SkillKind::Compose:
        return m_full->text();
    case SkillKind::Text:
        break;
    }
    return m_text->result();
}

void OutputPane::applyTheme(const Tokens& t)
{
    m_tokens = t;
    m_text->applyTheme(t);
    m_replies->applyTheme(t);
    m_copy->setLeadingIcon(Theme::icon(u"copy"_s, QColor(Qt::white), kIconSize));
    updateButtons();
}

void OutputPane::updateButtons()
{
    const ButtonTexts texts = textsFor(m_kind, m_running);
    m_copy->setLabel(texts.copy);
    m_secondary->setText(texts.secondary);
    m_retry->setText(texts.retry);
    m_copy->setEnabled(!m_running);
    m_secondary->setEnabled(!m_running);
    // Quick replies' retry is "Make it polite": nothing to stop there while replies are written.
    m_retry->setEnabled(!m_running || m_kind != SkillKind::QuickReply);
    const QIcon refresh = Theme::icon(u"refresh"_s, m_tokens.text, kIconSize);
    const QIcon stop = Theme::icon(u"stop"_s, m_tokens.text, kIconSize);
    m_secondary->setIcon(m_kind == SkillKind::Text ? QIcon() : refresh);
    if (m_kind == SkillKind::Text) {
        m_retry->setIcon(m_running ? stop : refresh);
    } else {
        m_retry->setIcon(m_running && m_kind != SkillKind::QuickReply ? stop : QIcon());
    }
    // Retry or Shorter turns into Stop while it runs: the same width either way.
    reserveWidth(m_retry, {textsFor(m_kind, false).retry, textsFor(m_kind, true).retry}, stop);
}

void OutputPane::updateVersions()
{
    const qsizetype count = m_full->versionCount();
    const bool shown = (m_kind == SkillKind::FullReply || m_kind == SkillKind::Compose) && count > 1;
    m_versionInfo->setVisible(shown);
    m_prev->setVisible(shown);
    m_next->setVisible(shown);
    if (shown) {
        const qsizetype index = m_full->versionIndex();
        m_versionInfo->setText(tr("Version %1 of %2").arg(index + 1).arg(count));
        m_prev->setEnabled(index > 0);
        m_next->setEnabled(index + 1 < count);
    }
}

} // namespace paraphrase::app

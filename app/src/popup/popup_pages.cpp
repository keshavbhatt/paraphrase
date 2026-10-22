#include "popup/popup_pages.h"

#include "ui/action_bar.h"
#include "ui/chip_button.h"
#include "ui/flow_layout.h"
#include "ui/languages.h"
#include "ui/result_views.h"
#include "ui/widgets.h"
#include "util/language_detector.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kIconSize = 14;
constexpr int kMarkSize = 18;

QFrame* band(const QString& name)
{
    auto* f = new QFrame;
    f->setObjectName(name);
    return f;
}

QLabel* sourceBox()
{
    QLabel* l = makeLabel(QString());
    l->setObjectName(u"source"_s);
    l->setWordWrap(true);
    l->setTextFormat(Qt::PlainText);
    l->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return l;
}

QPushButton* iconButton(const QString& name, const QString& accessible)
{
    QPushButton* b = makeButton(QString(), u"flat"_s);
    b->setObjectName(name);
    b->setFixedSize(28, 28);
    b->setToolTip(accessible);
    b->setAccessibleName(accessible);
    return b;
}

QHBoxLayout* privacyLine(QLabel*& lock)
{
    auto* row = new QHBoxLayout;
    row->setSpacing(5);
    lock = new QLabel;
    row->addWidget(lock);
    row->addWidget(makeLabel(QObject::tr("On this device"), u"private"_s));
    return row;
}

QString wordsAndLanguage(const util::LanguageDetector& detector, const QString& text)
{
    const auto words = text.split(QRegularExpression(u"\\s+"_s), Qt::SkipEmptyParts).size();
    const QString count = words == 1 ? QObject::tr("1 word") : QObject::tr("%1 words").arg(words);
    const QString name = languageName(detector, detector.detect(text));
    return name.isEmpty() ? u"· "_s + count : u"· %1 · %2"_s.arg(count, name);
}

} // namespace

PopupActionsPage::PopupActionsPage(const util::LanguageDetector& detector, QWidget* parent)
    : QWidget(parent)
    , m_detector(detector)
    , m_source(sourceBox())
    , m_actions(new ActionBar(detector, ActionBar::Keys::Digits))
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(buildHead());
    auto* body = new QWidget;
    auto* bodyLayout = new QVBoxLayout(body);
    bodyLayout->setContentsMargins(16, 14, 16, 14);
    bodyLayout->setSpacing(10);
    auto* captionRow = new QHBoxLayout;
    m_caption = makeLabel(QString(), u"caption"_s);
    m_info = makeLabel(QString(), u"small"_s);
    captionRow->addWidget(m_caption);
    captionRow->addWidget(m_info, 1);
    bodyLayout->addLayout(captionRow);
    bodyLayout->addWidget(m_source);
    bodyLayout->addWidget(makeLabel(tr("ACTIONS"), u"caption"_s));
    bodyLayout->addWidget(m_actions);
    bodyLayout->addStretch();
    layout->addWidget(body, 1);
    layout->addWidget(buildFoot());
}

QWidget* PopupActionsPage::buildHead()
{
    QFrame* head = band(u"popupHead"_s);
    auto* row = new QHBoxLayout(head);
    row->setContentsMargins(12, 10, 10, 10);
    row->setSpacing(10);
    m_mark = new QLabel;
    row->addWidget(m_mark);
    row->addWidget(makeLabel(u"<b>Paraphrase</b>"_s));
    m_from = makeLabel(QString(), u"small"_s);
    row->addWidget(m_from);
    row->addStretch();
    row->addLayout(privacyLine(m_lock));
    m_expand = iconButton(u"openInWindow"_s, tr("Open in window"));
    m_close = iconButton(u"closePopup"_s, tr("Close"));
    row->addWidget(m_expand);
    row->addWidget(m_close);
    connect(m_expand, &QPushButton::clicked, this, &PopupActionsPage::openInWindow);
    connect(m_close, &QPushButton::clicked, this, &PopupActionsPage::closeRequested);
    return head;
}

QWidget* PopupActionsPage::buildFoot()
{
    QFrame* foot = band(u"popupFoot"_s);
    auto* row = new QHBoxLayout(foot);
    row->setContentsMargins(16, 10, 16, 10);
    row->setSpacing(8);
    auto* dot = new QLabel;
    dot->setObjectName(u"dot"_s);
    dot->setFixedSize(8, 8);
    row->addWidget(dot);
    m_status = makeLabel(QString(), u"small"_s);
    m_status->setObjectName(u"popupStatus"_s);
    row->addWidget(m_status, 1);
    QLabel* esc = makeLabel(tr("Esc"), u"kbd"_s);
    row->addWidget(esc);
    row->addWidget(makeLabel(tr("close"), u"small"_s));
    return foot;
}

void PopupActionsPage::showText(const QString& text, const QString& appName, const QString& origin)
{
    m_from->setText(appName.isEmpty() ? QString() : tr("from %1").arg(appName));
    m_caption->setText(origin == u"selection"_s ? tr("SELECTED TEXT") : tr("COPIED TEXT"));
    m_info->setText(wordsAndLanguage(m_detector, text));
    m_source->setText(text);
}

void PopupActionsPage::setStatus(const QString& status)
{
    m_status->setText(status);
}

void PopupActionsPage::applyTheme(const Tokens& t)
{
    m_mark->setPixmap(Theme::icon(u"pen"_s, t.accent, kMarkSize).pixmap(kMarkSize, kMarkSize));
    m_lock->setPixmap(Theme::icon(u"lock"_s, t.privacy, kIconSize).pixmap(kIconSize, kIconSize));
    m_expand->setIcon(Theme::icon(u"expand"_s, t.muted, kIconSize));
    m_close->setIcon(Theme::icon(u"x"_s, t.muted, kIconSize));
    m_actions->applyTheme(t);
}

PopupResultPage::PopupResultPage(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(buildHead());
    layout->addWidget(buildBody(), 1);
    layout->addWidget(buildFoot());
}

QWidget* PopupResultPage::buildHead()
{
    QFrame* head = band(u"popupHead"_s);
    auto* row = new QHBoxLayout(head);
    row->setContentsMargins(12, 10, 10, 10);
    row->setSpacing(10);
    m_back = makeButton(tr("Actions"), u"flat"_s);
    m_back->setObjectName(u"backToActions"_s);
    m_title = makeLabel(QString());
    m_title->setObjectName(u"popupTitle"_s);
    row->addWidget(m_back);
    row->addWidget(m_title);
    row->addStretch();
    row->addLayout(privacyLine(m_lock));
    m_close = iconButton(u"closePopup"_s, tr("Close"));
    row->addWidget(m_close);
    connect(m_back, &QPushButton::clicked, this, &PopupResultPage::back);
    connect(m_close, &QPushButton::clicked, this, &PopupResultPage::closeRequested);
    return head;
}

QWidget* PopupResultPage::buildBody()
{
    auto* body = new QWidget;
    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(16, 14, 16, 14);
    layout->setSpacing(10);
    m_presets = new QWidget;
    new FlowLayout(m_presets, 6);
    layout->addWidget(m_presets);
    auto* captionRow = new QHBoxLayout;
    m_caption = makeLabel(QString(), u"caption"_s);
    m_pill = makeLabel(QString(), u"pill"_s);
    captionRow->addWidget(m_caption, 1);
    captionRow->addWidget(m_pill);
    layout->addLayout(captionRow);
    m_source = sourceBox();
    layout->addWidget(m_source);
    auto* infoRow = new QHBoxLayout;
    m_info = makeLabel(QString(), u"small"_s);
    m_info->setObjectName(u"popupInfo"_s);
    m_view = new SegmentedControl({
        {.name = u"showChanges"_s, .text = tr("Changes")},
        {.name = u"showResult"_s, .text = tr("Result")},
    });
    infoRow->addWidget(m_info, 1);
    infoRow->addWidget(m_view);
    layout->addLayout(infoRow);
    m_pick = makeLabel(tr("PICK A REPLY"), u"caption"_s);
    layout->addWidget(m_pick);
    m_results = new QStackedWidget;
    m_text = new TextResultView;
    m_text->setObjectName(u"resultBox"_s);
    m_replies = new QuickRepliesView;
    // The views bring their own margins for the main window's panes; here the body has them.
    for (const QWidget* view : {static_cast<QWidget*>(m_text), static_cast<QWidget*>(m_replies)}) {
        view->layout()->setContentsMargins(0, 0, 0, 0);
    }
    m_results->addWidget(m_text);
    m_results->addWidget(m_replies);
    layout->addWidget(m_results, 1);
    connect(m_view, &SegmentedControl::changed, this,
            [this](int index) { m_text->setShowResult(index == 1); });
    return body;
}

QWidget* PopupResultPage::buildFoot()
{
    QFrame* foot = band(u"popupFoot"_s);
    auto* row = new QHBoxLayout(foot);
    row->setContentsMargins(16, 12, 16, 12);
    row->setSpacing(8);
    m_insert = new ChipButton(QString(), tr("Enter"), ChipButton::ChipSide::After);
    m_insert->setObjectName(u"insert"_s);
    m_insert->setProperty("kind", u"primary"_s);
    m_insert->setProperty("size", u"big"_s);
    m_copy = makeButton(tr("Copy"));
    m_copy->setObjectName(u"popupCopy"_s);
    m_again = makeButton(QString());
    m_again->setObjectName(u"again"_s);
    for (QPushButton* b : {m_copy, m_again}) {
        b->setProperty("size", u"big"_s);
    }
    row->addWidget(m_insert);
    row->addWidget(m_copy);
    row->addWidget(m_again);
    row->addStretch();
    connect(m_insert, &QPushButton::clicked, this, &PopupResultPage::insertClicked);
    connect(m_copy, &QPushButton::clicked, this, &PopupResultPage::copyClicked);
    connect(m_again, &QPushButton::clicked, this, &PopupResultPage::againClicked);
    return foot;
}

void PopupResultPage::begin(SkillKind kind, const QString& title, const QString& source,
                            const QString& appName, bool showSource)
{
    m_kind = kind;
    m_appName = appName;
    m_title->setText(u"<b>%1</b>"_s.arg(title.toHtmlEscaped()));
    const bool reply = kind != SkillKind::Text;
    m_caption->setText(reply ? tr("MESSAGE YOU GOT") : tr("YOUR TEXT"));
    m_pill->setVisible(false);
    m_source->setText(source);
    m_source->setVisible(showSource);
    m_caption->setVisible(showSource);
    m_info->setVisible(!reply);
    m_pick->setVisible(reply);
    m_results->setCurrentWidget(reply ? static_cast<QWidget*>(m_replies) : m_text);
    m_view->setVisible(false);
    m_text->clear();
    m_replies->setReplies({});
    m_presets->setVisible(false);
    setInfo(tr("· writing..."));
    updateButtons();
}

void PopupResultPage::setPresets(const QList<const SkillInfo*>& presets, const QString& currentId)
{
    QLayout* flow = m_presets->layout();
    while (const QLayoutItem* item = flow->takeAt(0)) {
        delete item->widget();
        delete item;
    }
    for (const SkillInfo* p : presets) {
        QPushButton* b = makeButton(SkillCatalog::presetTitle(*p), u"tone"_s);
        b->setObjectName(u"preset-"_s + p->id);
        b->setCheckable(true);
        b->setChecked(p->id == currentId);
        const QString id = p->id;
        connect(b, &QPushButton::clicked, this, [this, id] { Q_EMIT presetChosen(id); });
        flow->addWidget(b);
    }
    m_presets->setVisible(presets.size() > 1);
}

void PopupResultPage::setInfo(const QString& text, const QString& role)
{
    m_info->setText(text);
    m_info->setProperty("role", role);
    m_info->style()->unpolish(m_info);
    m_info->style()->polish(m_info);
    // Changes or Result once a proofread diff is shown.
    m_view->setVisible(m_kind == SkillKind::Text && m_text->hasDiff());
}

void PopupResultPage::setRunning(bool running)
{
    m_running = running;
    updateButtons();
}

void PopupResultPage::setLanguagePill(const QString& language)
{
    m_pill->setText(language);
    m_pill->setVisible(!language.isEmpty());
}

QString PopupResultPage::result() const
{
    return m_kind == SkillKind::Text ? m_text->result() : m_replies->chosen();
}

void PopupResultPage::updateButtons()
{
    const bool reply = m_kind != SkillKind::Text;
    if (reply) {
        m_insert->setLabel(tr("Insert reply"));
    } else {
        m_insert->setLabel(m_appName.isEmpty() ? tr("Replace") : tr("Replace in %1").arg(m_appName));
    }
    if (reply) {
        m_again->setText(tr("3 new ones"));
    } else {
        m_again->setText(m_running ? tr("Stop") : tr("Retry"));
    }
    // Replace waits until the text is complete (popup-rewrite.html, note 2).
    m_insert->setEnabled(!m_running);
    m_copy->setEnabled(!m_running);
    m_again->setEnabled(!m_running || !reply);
    m_again->setIcon(Theme::icon(m_running && !reply ? u"stop"_s : u"refresh"_s, m_tokens.text, kIconSize));
}

void PopupResultPage::applyTheme(const Tokens& t)
{
    m_tokens = t;
    m_lock->setPixmap(Theme::icon(u"lock"_s, t.privacy, kIconSize).pixmap(kIconSize, kIconSize));
    m_back->setIcon(Theme::icon(u"left"_s, t.text, kIconSize));
    m_close->setIcon(Theme::icon(u"x"_s, t.muted, kIconSize));
    m_copy->setIcon(Theme::icon(u"copy"_s, t.text, kIconSize));
    m_insert->setLeadingIcon({});
    m_text->applyTheme(t);
    m_replies->applyTheme(t);
    updateButtons();
}

} // namespace paraphrase::app

#include "popup/popup_pages.h"

#include "ui/common/busy_pill.h"
#include "ui/common/chip_button.h"
#include "ui/common/flow_layout.h"
#include "ui/common/menu_icons.h"
#include "ui/common/widgets.h"
#include "ui/core/languages.h"
#include "ui/parts/action_bar.h"
#include "ui/parts/language_menu.h"
#include "ui/parts/result_views.h"
#include "ui/parts/skeleton_view.h"
#include "util/language_detector.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QStackedWidget>
#include <QStyle>
#include <QVBoxLayout>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kIconSize = 14;
constexpr int kMarkSize = 18;
constexpr int kFootGap = 6;

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

/// The grip at the start of a header: the header moves the popup (popup-working.html, note 1).
QLabel* grip()
{
    auto* l = new QLabel;
    l->setObjectName(u"grip"_s);
    l->setToolTip(QObject::tr("Drag to move"));
    return l;
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
    bodyLayout->setSpacing(12); // popup mocks: .pbody gap 12px
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
    row->setContentsMargins(8, 10, 10, 10);
    row->setSpacing(10);
    m_grip = grip();
    row->addWidget(m_grip);
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
    row->setContentsMargins(16, 12, 16, 12); // .pfoot padding 12px 16px
    row->setSpacing(8);
    m_dot = new QLabel;
    m_dot->setObjectName(u"dot"_s);
    m_dot->setFixedSize(8, 8);
    row->addWidget(m_dot);
    // While it works ahead: the working pill (main-window-busy.html), else a plain line.
    m_busy = new BusyPill;
    m_busy->setObjectName(u"popupBusy"_s);
    m_busy->setSteps({tr("Loading the model"), tr("Reading the text for %1").arg(tr("My actions"))});
    row->addWidget(m_busy);
    m_status = makeLabel(QString(), u"small"_s);
    m_status->setObjectName(u"popupStatus"_s);
    m_status->setMinimumHeight(m_busy->sizeHint().height()); // the pill or the line: one height
    m_status->setWordWrap(true); // a longer notice wraps instead of stopping mid-sentence
    row->addWidget(m_status, 1);
    row->addStretch(); // the space stays here while the working pill hides the line
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
    m_busy->stop();
    m_dot->show();
    m_status->show();
    m_status->setText(status);
}

void PopupActionsPage::showWorking(const QString& step)
{
    m_dot->hide();
    m_status->hide();
    if (m_busy->isHidden()) {
        m_busy->start(step);
    } else {
        m_busy->setStep(step);
    }
}

void PopupActionsPage::applyTheme(const Tokens& t)
{
    m_mark->setPixmap(Theme::icon(u"pen"_s, t.accent, kMarkSize).pixmap(kMarkSize, kMarkSize));
    m_grip->setPixmap(Theme::icon(u"grip"_s, t.muted, kIconSize).pixmap(kIconSize, kIconSize));
    m_busy->applyTheme(t);
    m_lock->setPixmap(Theme::icon(u"lock"_s, t.privacy, kIconSize).pixmap(kIconSize, kIconSize));
    m_expand->setIcon(Theme::icon(u"expand"_s, t.muted, kIconSize));
    m_close->setIcon(Theme::icon(u"x"_s, t.muted, kIconSize));
    m_actions->applyTheme(t);
}

PopupResultPage::PopupResultPage(const util::LanguageDetector& detector, QWidget* parent)
    : QWidget(parent)
    , m_detector(detector)
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
    row->setContentsMargins(8, 10, 10, 10);
    row->setSpacing(10);
    m_grip = grip();
    row->addWidget(m_grip);
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
    layout->setSpacing(12); // .pbody gap 12px
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
    m_busy = new BusyPill;
    m_busy->setSteps(
        {tr("Reading your text"), tr("Writing"), tr("Checking, writing again"), tr("Writing 3 replies")});
    infoRow->addWidget(m_busy);
    infoRow->addWidget(m_info, 1);
    infoRow->addStretch(0); // keeps the pill at the start while the line is hidden
    infoRow->addWidget(m_view);
    infoRow->addWidget(buildLanguagePill());
    layout->addLayout(infoRow);
    m_pick = makeLabel(tr("PICK A REPLY"), u"caption"_s);
    layout->addWidget(m_pick);
    // One result view shows at a time. Not a QStackedWidget: that is as tall as its tallest
    // page (three reply cards), which left an empty band under a short text result.
    m_results = new QWidget;
    auto* views = new QVBoxLayout(m_results);
    views->setContentsMargins(0, 0, 0, 0);
    views->setSpacing(0);
    m_text = new TextResultView;
    m_text->setObjectName(u"resultBox"_s);
    m_text->setFitToText(true); // popup-proofread.html: the box hugs the text
    m_replies = new QuickRepliesView;
    m_replies->setForPopup(true);
    // The views bring their own margins for the main window's panes; here the body has them.
    for (const QWidget* view : {static_cast<QWidget*>(m_text), static_cast<QWidget*>(m_replies)}) {
        view->layout()->setContentsMargins(0, 0, 0, 0);
    }
    m_skeleton = new SkeletonView;
    views->addWidget(m_text);
    views->addWidget(m_replies);
    views->addWidget(m_skeleton);
    m_replies->hide();
    m_skeleton->hide();
    layout->addWidget(m_results);
    m_otherLanguage = buildLanguageLink();
    layout->addWidget(m_otherLanguage, 0, Qt::AlignLeft);
    connect(m_view, &SegmentedControl::changed, this,
            [this](int index) { m_text->setShowResult(index == 1); });
    return body;
}

QWidget* PopupResultPage::buildLanguagePill()
{
    // The language the user chose, with × to go back to the text's own (popup-working.html, 3).
    m_chosen = new QFrame;
    m_chosen->setObjectName(u"chosenLanguage"_s);
    auto* row = new QHBoxLayout(m_chosen);
    row->setContentsMargins(10, 0, 4, 0);
    row->setSpacing(6);
    m_chosenName = new QLabel;
    m_chosenName->setObjectName(u"chosenLanguageName"_s);
    m_clearLanguage = makeButton(QString(), u"flat"_s);
    m_clearLanguage->setObjectName(u"clearLanguage"_s);
    m_clearLanguage->setFixedSize(18, 18);
    row->addWidget(m_chosenName);
    row->addWidget(m_clearLanguage);
    connect(m_clearLanguage, &QPushButton::clicked, this, &PopupResultPage::languageCleared);
    m_chosen->hide();
    return m_chosen;
}

void PopupResultPage::setChosenLanguage(const QString& native, const QString& name, const QString& own)
{
    m_chosenName->setText(native.isEmpty() || native == name ? name : u"%1 · %2"_s.arg(native, name));
    m_clearLanguage->setToolTip(own.isEmpty() ? tr("Back to the text's language")
                                              : tr("Back to %1").arg(own));
    m_clearLanguage->setAccessibleName(m_clearLanguage->toolTip());
    m_chosen->setVisible(!name.isEmpty());
}

QPushButton* PopupResultPage::buildLanguageLink()
{
    // "Write it in another language": the languages, filled when it opens; a list that
    // scrolls, as under Translate.
    QPushButton* link = makeButton(tr("Write it in another language"), u"link"_s);
    link->setObjectName(u"otherLanguage"_s);
    auto* languages = new LanguageMenu(link);
    link->setMenu(languages);
    connect(languages, &QMenu::aboutToShow, this, [this, languages] {
        languages->clearLanguages();
        languages->setTokens(m_tokens);
        for (const MenuLanguage& lang : menuLanguages()) {
            const QString name = languageName(m_detector, lang.code);
            if (name.isEmpty() || lang.code == m_ownLanguage) {
                continue;
            }
            const QAction* item = languages->addLanguage(lang.code, name, nativeLanguageName(lang.code),
                                                         scriptBadge(lang.code, m_tokens));
            const QString code = lang.code;
            connect(item, &QAction::triggered, this, [this, code] { Q_EMIT languageChosen(code); });
        }
        languages->finish();
    });
    link->hide();
    return link;
}

QWidget* PopupResultPage::buildFoot()
{
    QFrame* foot = band(u"popupFoot"_s);
    auto* row = new QHBoxLayout(foot);
    row->setContentsMargins(16, 12, 16, 12);
    row->setSpacing(kFootGap); // 2 px under the mocks' 8: "Replace in Telegram" fits the 440 px popup
    m_insert = new ChipButton(QString(), tr("Enter"), ChipButton::ChipSide::After);
    m_insert->setObjectName(u"insert"_s);
    m_insert->setProperty("kind", u"primary"_s);
    m_insert->setProperty("btnSize", u"big"_s);
    m_copy = makeButton(tr("Copy"));
    m_copy->setObjectName(u"popupCopy"_s);
    m_again = makeButton(QString());
    m_again->setObjectName(u"again"_s);
    for (QPushButton* b : {m_copy, m_again}) {
        b->setProperty("btnSize", u"big"_s);
    }
    m_done = makeButton(tr("Close"), u"primary"_s);
    m_done->setObjectName(u"closeResult"_s);
    m_done->setProperty("btnSize", u"big"_s);
    row->addWidget(m_insert);
    row->addWidget(m_done);
    row->addWidget(m_copy);
    row->addWidget(m_again);
    row->addStretch();
    connect(m_done, &QPushButton::clicked, this, &PopupResultPage::closeRequested);
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
    m_running = false; // a new run: setRunning(true) starts the pill again
    m_busy->stop();
    m_title->setText(u"<b>%1</b>"_s.arg(title.toHtmlEscaped()));
    const bool reply = kind != SkillKind::Text;
    m_caption->setText(reply ? tr("MESSAGE YOU GOT") : tr("YOUR TEXT"));
    m_pill->setVisible(false);
    m_source->setText(source);
    m_source->setVisible(showSource);
    m_caption->setVisible(showSource);
    m_info->setVisible(!reply);
    m_pick->setVisible(reply);
    showResultView();
    m_view->setVisible(false);
    m_text->clear();
    m_replies->setReplies({});
    m_presets->setVisible(false);
    m_nothingToFix = false;
    setInfo(QString());
    updateButtons();
}

void PopupResultPage::showView(QWidget* view)
{
    for (QWidget* w : {static_cast<QWidget*>(m_text), static_cast<QWidget*>(m_replies),
                       static_cast<QWidget*>(m_skeleton)}) {
        w->setVisible(w == view);
    }
}

void PopupResultPage::showResultView()
{
    showView(m_kind != SkillKind::Text ? static_cast<QWidget*>(m_replies) : m_text);
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
    const bool started = running && !m_running;
    const bool ended = !running && m_running;
    m_running = running;
    const bool reply = m_kind != SkillKind::Text;
    if (started) {
        m_busy->start(reply ? tr("Writing 3 replies") : tr("Reading your text"));
        m_info->hide();
        // As many lines as the text takes in the result box: the answer is about as long.
        QFont f = font();
        f.setPixelSize(15);
        const QFontMetrics fm(f);
        const int width = std::max(200, m_results->width() > 0 ? m_results->width() : 408);
        const int lines =
            fm.boundingRect(QRect(0, 0, width, 100'000), Qt::TextWordWrap, m_source->text()).height() /
            std::max(1, fm.lineSpacing());
        m_skeleton->setShape(reply ? SkeletonView::Shape::Replies : SkeletonView::Shape::Text);
        m_skeleton->setCompact(std::clamp(lines, 1, 12));
        showView(m_skeleton);
    } else if (ended) {
        m_busy->stop();
        m_info->setVisible(!reply);
        m_text->endStream();
        showResultView();
    }
    updateButtons();
}

void PopupResultPage::firstWord()
{
    if (m_running && m_kind == SkillKind::Text && m_busy->step() != tr("Writing")) {
        m_busy->setStep(tr("Writing"));
        showResultView();
    }
}

void PopupResultPage::retrying()
{
    if (m_running) {
        m_busy->setStep(tr("Checking, writing again"));
        showView(m_skeleton);
    }
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

void PopupResultPage::offerOtherLanguages(bool offer, const QString& own)
{
    m_ownLanguage = own;
    m_otherLanguage->setVisible(offer);
}

void PopupResultPage::showNothingToFix()
{
    m_nothingToFix = true;
    setInfo(tr("Looks good, no changes"));
    m_view->setVisible(false);
    updateButtons();
}

void PopupResultPage::updateButtons()
{
    for (QWidget* b :
         {static_cast<QWidget*>(m_insert), static_cast<QWidget*>(m_copy), static_cast<QWidget*>(m_again)}) {
        b->setVisible(!m_nothingToFix);
    }
    m_done->setVisible(m_nothingToFix);
    const bool reply = m_kind != SkillKind::Text;
    if (reply) {
        m_insert->setLabel(tr("Insert reply"));
    } else {
        m_insert->setLabel(m_appName.isEmpty() ? tr("Replace") : tr("Replace in %1").arg(m_appName));
        fitInsertLabel();
    }
    if (reply) {
        m_again->setText(tr("3 new ones"));
    } else {
        m_again->setText(m_running ? tr("Stop") : tr("Retry"));
    }
    setButtonIcon(m_again, m_running && !reply ? u"stop"_s : u"refresh"_s, m_tokens.text, kIconSize);
    // Only this page's own pair: "3 new ones" never shares the row with Replace.
    reserveWidth(m_again, reply ? QStringList{tr("3 new ones")} : QStringList{tr("Retry"), tr("Stop")});
    // Replace waits until the text is complete (popup-rewrite.html, note 2).
    m_insert->setEnabled(!m_running);
    m_copy->setEnabled(!m_running);
    m_again->setEnabled(!m_running || !reply);
}

void PopupResultPage::fitInsertLabel()
{
    // The footer holds Replace, Copy and Retry in the popup's width: a long app name is cut
    // with "…" rather than the label being clipped mid-letter ("Replace in Telegran").
    constexpr int kFooterWidth = 440 - (2 * 16) - (2 * kFootGap); // the popup, .pfoot padding, two gaps
    const int others = std::max(m_copy->sizeHint().width(), m_copy->minimumWidth()) +
                       std::max(m_again->sizeHint().width(), m_again->minimumWidth());
    const int over = m_insert->sizeHint().width() - (kFooterWidth - others);
    if (over <= 0) {
        return;
    }
    QFont f = m_insert->font();
    f.setWeight(QFont::Medium); // QLabel#label in the style sheet
    const QFontMetrics fm(f);
    const QString name = fm.elidedText(m_appName, Qt::ElideRight, fm.horizontalAdvance(m_appName) - over - 2);
    m_insert->setLabel(name.size() <= 1 ? tr("Replace") : tr("Replace in %1").arg(name));
}

void PopupResultPage::applyTheme(const Tokens& t)
{
    m_tokens = t;
    m_lock->setPixmap(Theme::icon(u"lock"_s, t.privacy, kIconSize).pixmap(kIconSize, kIconSize));
    m_grip->setPixmap(Theme::icon(u"grip"_s, t.muted, kIconSize).pixmap(kIconSize, kIconSize));
    m_clearLanguage->setIcon(Theme::icon(u"x"_s, t.accent, 12));
    m_back->setIcon(Theme::icon(u"left"_s, t.text, kIconSize));
    m_close->setIcon(Theme::icon(u"x"_s, t.muted, kIconSize));
    setButtonIcon(m_copy, u"copy"_s, t.text, kIconSize);
    m_insert->setLeadingIcon({});
    m_text->applyTheme(t);
    m_replies->applyTheme(t);
    m_busy->applyTheme(t);
    m_skeleton->applyTheme(t);
    updateButtons();
}

} // namespace paraphrase::app

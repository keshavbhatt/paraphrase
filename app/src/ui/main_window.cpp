#include "main_window.h"

#include <QApplication>
#include <QButtonGroup>
#include <QClipboard>
#include <QDBusMessage>
#include <QEvent>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QRegularExpression>
#include <QResizeEvent>
#include <QScrollArea>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QStyle>
#include <QTextBrowser>
#include <QToolButton>
#include <QVBoxLayout>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kWarmDelayMs = 800; ///< after the last keystroke (ADR-005)
constexpr int kNarrowWidth = 820; ///< below this the panes stack
constexpr int kMaxShortcuts = 9;

QLabel* label(const QString& text, const QString& role = {})
{
    auto* l = new QLabel(text);
    if (!role.isEmpty()) {
        l->setProperty("role", role);
    }
    return l;
}

QPushButton* button(const QString& text, const QString& kind = {})
{
    auto* b = new QPushButton(text);
    if (!kind.isEmpty()) {
        b->setProperty("kind", kind);
    }
    b->setCursor(Qt::PointingHandCursor);
    return b;
}

QJsonValue parseJson(const QString& text)
{
    const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
    return doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object());
}

QString html(const QString& text)
{
    return text.toHtmlEscaped().replace(u'\n', u"<br>"_s);
}

/// The order the actions appear in: the skills the mocks show first, then the rest.
int rank(const QString& base)
{
    static const QStringList kOrder = {
        u"proofread"_s, u"rewrite"_s, u"reply"_s, u"summarize"_s, u"format"_s, u"translate"_s,
    };
    const qsizetype i = kOrder.indexOf(base);
    return i < 0 ? static_cast<int>(kOrder.size()) : static_cast<int>(i);
}

} // namespace

MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
    , m_detector(util::LanguageData::builtIn())
    , m_tokens(Theme::current())
{
    setObjectName(u"mainWindow"_s);
    setWindowTitle(u"Paraphrase"_s);
    resize(1180, 760);
    buildUi();
    applyTheme();
    m_warmTimer.setSingleShot(true);
    m_warmTimer.setInterval(kWarmDelayMs);
    connect(&m_warmTimer, &QTimer::timeout, this, [this] {
        if (!m_skill.isEmpty() && !busy() && !input().isEmpty()) {
            (void)m_client.call(u"Warm"_s, {m_skill, input()});
        }
    });
    connect(&m_client, &client::DaemonClient::token, this, &MainWindow::onToken);
    connect(&m_client, &client::DaemonClient::retrying, this, &MainWindow::onRetrying);
    connect(&m_client, &client::DaemonClient::done, this, &MainWindow::onDone);
    connect(&m_client, &client::DaemonClient::error, this, &MainWindow::onError);
}

MainWindow::~MainWindow()
{
    if (busy()) {
        cancel();
    }
}

void MainWindow::buildUi()
{
    // The layout of main-window.html: a header band, the notice band, then the actions and the
    // two panes on the window background, and the status band.
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    buildHeader(root);

    auto* body = new QWidget;
    auto* main = new QVBoxLayout(body);
    main->setContentsMargins(16, 14, 16, 16);
    main->setSpacing(14);
    root->addWidget(body, 1);

    // Actions, filled from the daemon's skills.
    m_actionsRow = new QHBoxLayout;
    m_actionsRow->setSpacing(8);
    m_actionGroup = new QButtonGroup(this);
    m_actionGroup->setExclusive(true);
    main->addLayout(m_actionsRow);

    m_panes = new QSplitter(Qt::Horizontal);
    m_panes->setChildrenCollapsible(false);
    m_panes->setHandleWidth(14);
    m_panes->addWidget(buildInputPane());
    m_panes->addWidget(buildOutputPane());
    main->addWidget(m_panes, 1);
    root->addWidget(buildStatusBar());

    // Keyboard first (DESIGN.md 4).
    connect(new QShortcut(QKeySequence(u"Ctrl+Return"_s), this), &QShortcut::activated, this,
            &MainWindow::run);
    connect(new QShortcut(QKeySequence(u"Ctrl+Shift+C"_s), this), &QShortcut::activated, this,
            &MainWindow::copyResult);
    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this,
            &MainWindow::cancel);

    setRunning(false);
}

void MainWindow::buildHeader(QVBoxLayout* root)
{
    // Header: the mark, and the privacy line on every surface (DESIGN.md 4).
    auto* header = new QFrame;
    header->setObjectName(u"header"_s);
    auto* row = new QHBoxLayout(header);
    row->setContentsMargins(14, 10, 14, 10);
    row->setSpacing(10);
    m_mark = new QLabel;
    row->addWidget(m_mark);
    row->addWidget(label(u"<b>Paraphrase</b>"_s));
    row->addStretch();
    m_lock = new QLabel;
    auto* privacy = new QHBoxLayout;
    privacy->setSpacing(5);
    privacy->addWidget(m_lock);
    privacy->addWidget(label(tr("On this device"), u"private"_s));
    row->addLayout(privacy);
    m_settings = button(tr("Settings"), u"flat"_s);
    m_settings->setObjectName(u"settings"_s);
    row->addWidget(m_settings);
    connect(m_settings, &QPushButton::clicked, this, &MainWindow::settingsRequested);
    root->addWidget(header);

    // The notice for what the desktop does not offer (ADR-013): never an error, can be hidden.
    m_notice = new QFrame;
    m_notice->setObjectName(u"notice"_s);
    auto* notice = new QHBoxLayout(m_notice);
    notice->setContentsMargins(16, 10, 10, 10);
    notice->setSpacing(10);
    m_noticeIcon = new QLabel;
    notice->addWidget(m_noticeIcon, 0, Qt::AlignTop);
    m_noticeText = label(QString());
    m_noticeText->setObjectName(u"noticeText"_s);
    m_noticeText->setWordWrap(true);
    m_noticeText->setTextFormat(Qt::RichText);
    connect(m_noticeText, &QLabel::linkActivated, this, &MainWindow::settingsRequested);
    notice->addWidget(m_noticeText, 1);
    auto* hide = button(QString(), u"icon"_s);
    hide->setProperty("kind", u"flat"_s);
    hide->setFixedSize(28, 28);
    hide->setObjectName(u"hideNotice"_s);
    hide->setToolTip(tr("Hide this notice"));
    hide->setAccessibleName(tr("Hide this notice"));
    connect(hide, &QPushButton::clicked, m_notice, &QWidget::hide);
    notice->addWidget(hide, 0, Qt::AlignTop);
    root->addWidget(m_notice);
    setNotice(tr("This desktop gives Paraphrase no global shortcut, so it cannot open over other apps. "
                 "Everything works here: paste your text, pick an action, copy the result."));
}

void MainWindow::setNotice(const QString& text, const QString& link)
{
    m_noticeText->setText(link.isEmpty()
                              ? text.toHtmlEscaped()
                              : u"%1 <a href=\"settings\">%2</a>"_s.arg(text.toHtmlEscaped(), link));
    m_notice->setVisible(!text.isEmpty());
}

QFrame* MainWindow::paneHead(QHBoxLayout*& row)
{
    auto* head = new QFrame;
    head->setObjectName(u"paneHead"_s);
    row = new QHBoxLayout(head);
    row->setContentsMargins(14, 8, 10, 8);
    row->setSpacing(8);
    return head;
}

QWidget* MainWindow::buildInputPane()
{
    // Left pane: your text, or the message and draft for Reply.
    auto* left = new QFrame;
    left->setObjectName(u"pane"_s);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    QHBoxLayout* head = nullptr;
    leftLayout->addWidget(paneHead(head));
    m_inTitle = label(tr("YOUR TEXT"), u"caption"_s);
    head->addWidget(m_inTitle);
    m_inInfo = label(QString(), u"small"_s);
    head->addWidget(m_inInfo);
    head->addStretch();
    m_langPill = label(QString(), u"pill"_s);
    m_langPill->setObjectName(u"langPill"_s);
    head->addWidget(m_langPill);
    auto* paste = button(tr("Paste"), u"flat"_s);
    m_clear = button(tr("Clear"), u"flat"_s);
    head->addWidget(paste);
    head->addWidget(m_clear);

    m_inStack = new QStackedWidget;
    auto* textPage = new QWidget;
    auto* textLayout = new QVBoxLayout(textPage);
    textLayout->setContentsMargins(14, 12, 14, 12);
    m_text = new QPlainTextEdit;
    m_text->setObjectName(u"text"_s);
    m_text->setPlaceholderText(tr("Type or paste your text"));
    textLayout->addWidget(m_text);
    m_inStack->addWidget(textPage);
    m_inStack->addWidget(buildReplyInput());
    leftLayout->addWidget(m_inStack, 1);

    auto* foot = new QFrame;
    foot->setObjectName(u"paneFoot"_s);
    auto* footRow = new QHBoxLayout(foot);
    footRow->setContentsMargins(14, 10, 14, 10);
    m_inHint = label(tr("Type or paste anything. Ctrl+Enter runs the selected action."), u"small"_s);
    m_inHint->setWordWrap(true);
    footRow->addWidget(m_inHint);
    m_inFoot = foot;
    leftLayout->addWidget(foot);

    connect(paste, &QPushButton::clicked, this, [this] {
        QPlainTextEdit* target = m_inStack->currentIndex() == 0 ? m_text : m_message;
        target->setPlainText(QApplication::clipboard()->text());
    });
    connect(m_clear, &QPushButton::clicked, this, [this] {
        m_text->clear();
        m_message->clear();
        m_draft->clear();
    });
    for (const QPlainTextEdit* edit : {m_text, m_message, m_draft}) {
        connect(edit, &QPlainTextEdit::textChanged, this, [this] {
            updateInputInfo();
            warmLater();
        });
    }
    return left;
}

QWidget* MainWindow::buildReplyInput()
{
    // main-window-reply.html: the message (four lines), then the optional started reply.
    auto* page = new QWidget;
    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(14, 14, 14, 14);
    layout->setSpacing(10);
    m_message = new QPlainTextEdit;
    m_message->setObjectName(u"message"_s);
    m_message->setPlaceholderText(tr("Paste the message you got"));
    m_message->setFixedHeight(120);
    layout->addWidget(m_message);
    layout->addWidget(label(tr("Your reply so far (optional)"), u"small"_s));
    m_draft = new QPlainTextEdit;
    m_draft->setObjectName(u"draft"_s);
    m_draft->setPlaceholderText(tr("Start typing and the replies will continue it"));
    m_draft->setFixedHeight(70);
    layout->addWidget(m_draft);
    layout->addStretch();
    return page;
}

QWidget* MainWindow::buildOutputPane()
{
    // Right pane: the result.
    auto* right = new QFrame;
    right->setObjectName(u"pane"_s);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(0);
    QHBoxLayout* head = nullptr;
    rightLayout->addWidget(paneHead(head));
    m_outTitle = label(QString(), u"caption"_s);
    head->addWidget(m_outTitle);
    m_outInfo = label(QString(), u"small"_s);
    m_outInfo->setObjectName(u"outInfo"_s);
    head->addWidget(m_outInfo, 1);
    m_segment = new QFrame;
    m_segment->setObjectName(u"seg"_s);
    auto* seg = new QHBoxLayout(m_segment);
    seg->setContentsMargins(2, 2, 2, 2);
    seg->setSpacing(0);
    m_showChanges = button(tr("Changes"), u"segment"_s);
    m_showResult = button(tr("Result"), u"segment"_s);
    for (QPushButton* b : {m_showChanges, m_showResult}) {
        b->setCheckable(true);
        seg->addWidget(b);
    }
    m_showChanges->setChecked(true);
    auto* segGroup = new QButtonGroup(this);
    segGroup->addButton(m_showChanges);
    segGroup->addButton(m_showResult);
    connect(segGroup, &QButtonGroup::buttonClicked, this, [this] { renderDiff(); });
    head->addWidget(m_segment);

    m_outStack = new QStackedWidget;
    auto* textOut = new QWidget;
    auto* textOutLayout = new QVBoxLayout(textOut);
    textOutLayout->setContentsMargins(14, 12, 14, 8);
    textOutLayout->setSpacing(8);
    m_output = new QTextBrowser;
    m_output->setObjectName(u"output"_s);
    m_output->setOpenLinks(false);
    textOutLayout->addWidget(m_output, 3);
    m_changes = new QWidget;
    m_changesLayout = new QVBoxLayout(m_changes);
    m_changesLayout->setContentsMargins(0, 0, 0, 0);
    m_changesLayout->setSpacing(2);
    m_changesScroll = new QScrollArea;
    m_changesScroll->setWidgetResizable(true);
    m_changesScroll->setWidget(m_changes);
    m_changesScroll->setObjectName(u"changes"_s);
    m_changesScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    textOutLayout->addWidget(m_changesScroll, 2);
    m_outStack->addWidget(textOut);

    m_outStack->addWidget(buildReplyOutput());
    rightLayout->addWidget(m_outStack, 1);
    rightLayout->addWidget(buildFooter());
    return right;
}

QWidget* MainWindow::buildReplyOutput()
{
    auto* replyOut = new QWidget;
    auto* replyOutLayout = new QVBoxLayout(replyOut);
    replyOutLayout->setContentsMargins(14, 14, 14, 14);
    replyOutLayout->setSpacing(10);
    for (int i = 0; i < 3; ++i) {
        auto* row = new QHBoxLayout;
        row->setSpacing(10);
        auto* choice = new ChipButton(QString(), QString::number(i + 1));
        choice->setProperty("kind", u"choice"_s);
        choice->setObjectName(u"reply%1"_s.arg(i + 1));
        choice->setCheckable(true);
        choice->setAutoExclusive(true);
        choice->setWordWrap(true);
        connect(choice, &QPushButton::clicked, this,
                [this, choice] { m_replyEdit->setText(choice->property("reply").toString()); });
        auto* copy = button(QString(), u"icon"_s);
        copy->setToolTip(tr("Copy reply %1").arg(i + 1));
        copy->setAccessibleName(tr("Copy reply %1").arg(i + 1));
        connect(copy, &QPushButton::clicked, this,
                [choice] { QApplication::clipboard()->setText(choice->property("reply").toString()); });
        row->addWidget(choice, 1);
        row->addWidget(copy);
        replyOutLayout->addLayout(row);
        m_replyButtons.append(choice);
        m_replyCopy.append(copy);
        // Number keys 1 to 3 pick a reply (main-window-reply.html, note 3).
        const auto* key = new QShortcut(QKeySequence(Qt::Key_1 + i), replyOut, nullptr, nullptr,
                                        Qt::WidgetWithChildrenShortcut);
        connect(key, &QShortcut::activated, choice, &QPushButton::click);
    }
    replyOutLayout->addSpacing(4);
    replyOutLayout->addWidget(label(tr("Edit before copying"), u"small"_s));
    m_replyEdit = new QLineEdit;
    m_replyEdit->setObjectName(u"replyEdit"_s);
    replyOutLayout->addWidget(m_replyEdit);
    replyOutLayout->addStretch();

    return replyOut;
}

QWidget* MainWindow::buildFooter()
{
    auto* foot = new QFrame;
    foot->setObjectName(u"paneFoot"_s);
    auto* footer = new QHBoxLayout(foot);
    footer->setContentsMargins(14, 10, 14, 10);
    footer->setSpacing(8);
    m_primary = new ChipButton(QString(), tr("Ctrl Shift C"), ChipButton::ChipSide::After);
    m_primary->setProperty("kind", u"primary"_s);
    m_primary->setProperty("size", u"big"_s);
    m_primary->setObjectName(u"copy"_s);
    m_secondary = button(QString());
    m_secondary->setProperty("size", u"big"_s);
    m_retry = button(QString());
    m_retry->setProperty("size", u"big"_s);
    footer->addWidget(m_primary);
    footer->addWidget(m_secondary);
    footer->addWidget(m_retry);
    footer->addStretch();

    connect(m_primary, &QPushButton::clicked, this, &MainWindow::copyResult);
    connect(m_secondary, &QPushButton::clicked, this, [this] {
        if (modeOf(m_skill) == Mode::Reply) {
            run(); // three new ones
            return;
        }
        // "Use as my text": run another action on the result, e.g. Proofread then Tone.
        m_text->setPlainText(resultText());
        m_output->clear();
        m_diff.reset();
        renderDiff();
    });
    connect(m_retry, &QPushButton::clicked, this, [this] {
        if (busy()) {
            cancel();
        } else if (modeOf(m_skill) == Mode::Reply) {
            makeReplyPolite();
        } else {
            run();
        }
    });
    return foot;
}

void MainWindow::makeReplyPolite()
{
    // "Make it polite": the Tone action on the chosen reply.
    const QString text = m_replyEdit->text().trimmed();
    if (text.isEmpty() || skill(u"rewrite.polite"_s) == nullptr) {
        return;
    }
    const QDBusMessage reply =
        m_client.call(u"RunSkill"_s, {u"rewrite.polite"_s, QVariantMap{{u"text"_s, text}}, QVariantMap()});
    if (reply.type() == QDBusMessage::ReplyMessage) {
        m_politeForReply = true;
        m_requestId = reply.arguments().value(0).toString();
        m_streamed.clear();
        setRunning(true);
    } else {
        showMessage(reply.errorMessage(), u"danger"_s);
    }
}

QWidget* MainWindow::buildStatusBar()
{
    // Status line: which model answers; the privacy promise.
    auto* status = new QFrame;
    status->setObjectName(u"statusBar"_s);
    auto* statusLayout = new QHBoxLayout(status);
    statusLayout->setContentsMargins(16, 7, 16, 7);
    statusLayout->setSpacing(10);
    m_statusDot = new QLabel;
    m_statusDot->setObjectName(u"dot"_s);
    m_statusDot->setFixedSize(8, 8);
    statusLayout->addWidget(m_statusDot);
    m_statusText = label(tr("Starting..."));
    m_statusText->setObjectName(u"statusText"_s);
    statusLayout->addWidget(m_statusText);
    statusLayout->addStretch();
    statusLayout->addWidget(label(tr("Nothing leaves this computer")));

    return status;
}

void MainWindow::applyTheme()
{
    m_tokens = Theme::current();
    setStyleSheet(Theme::styleSheet(m_tokens));
    m_mark->setPixmap(Theme::icon(u"pen"_s, m_tokens.accent, 18).pixmap(18, 18));
    m_lock->setPixmap(Theme::icon(u"lock"_s, m_tokens.privacy, 14).pixmap(14, 14));
    m_noticeIcon->setPixmap(Theme::icon(u"info"_s, m_tokens.warning, 14).pixmap(14, 14));
    if (auto* hide = m_notice->findChild<QPushButton*>(u"hideNotice"_s)) {
        hide->setIcon(Theme::icon(u"x"_s, m_tokens.warning, 14));
    }
    m_primary->setLeadingIcon(Theme::icon(u"copy"_s, QColor(Qt::white), 14));
    for (QPushButton* copy : std::as_const(m_replyCopy)) {
        copy->setIcon(Theme::icon(u"copy"_s, m_tokens.text, 14));
    }
    for (QAbstractButton* b : m_actionGroup->buttons()) {
        if (auto* chip = qobject_cast<ChipButton*>(b); chip != nullptr && chip->menu() != nullptr) {
            chip->setTrailingIcon(
                Theme::icon(u"down"_s, chip->isChecked() ? QColor(Qt::white) : m_tokens.muted, 14));
        }
    }
    selectSkill(m_skill); // icons that depend on the action
    renderDiff();
}

void MainWindow::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::ThemeChange || event->type() == QEvent::PaletteChange) {
        const bool dark = Theme::current().dark;
        if (dark != m_tokens.dark) {
            applyTheme();
        }
    }
    QWidget::changeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    // Narrow windows stack the two panes (main-window.html, note 6).
    m_panes->setOrientation(event->size().width() < kNarrowWidth ? Qt::Vertical : Qt::Horizontal);
    QWidget::resizeEvent(event);
}

void MainWindow::start()
{
    showMessage(tr("Starting Paraphrase..."));
    if (const QString problem = m_client.connectToDaemon(); !problem.isEmpty()) {
        m_statusText->setText(problem);
        m_statusDot->setStyleSheet(u"color: %1;"_s.arg(m_tokens.danger.name()));
        showMessage(problem, u"danger"_s);
        return;
    }
    const QDBusMessage reply = m_client.call(u"ListSkills"_s, {});
    QList<SkillInfo> skills;
    for (const auto& v : parseJson(reply.arguments().value(0).toString()).toArray()) {
        const QJsonObject o = v.toObject();
        skills.append({
            .id = o.value(u"id"_s).toString(),
            .base = o.value(u"base"_s).toString(),
            .title = o.value(u"title"_s).toString(),
            .output = o.value(u"output"_s).toString(),
        });
    }
    buildActions(skills);
    loadStatus();
    showMessage(tr("Paste or type your text on the left, then pick an action."));
    Q_EMIT actionsLoaded();
}

void MainWindow::buildActions(const QList<SkillInfo>& skills)
{
    m_skills = skills;
    std::ranges::stable_sort(
        m_skills, [](const SkillInfo& a, const SkillInfo& b) { return rank(a.base) < rank(b.base); });
    QStringList bases;
    for (const SkillInfo& s : std::as_const(m_skills)) {
        if (!bases.contains(s.base)) {
            bases.append(s.base);
        }
    }
    int number = 0;
    for (const QString& base : std::as_const(bases)) {
        ++number;
        QList<const SkillInfo*> variants;
        for (const SkillInfo& s : std::as_const(m_skills)) {
            if (s.base == base) {
                variants.append(&s);
            }
        }
        ChipButton* action = actionButton(number, base, variants);
        m_actionGroup->addButton(action);
        m_actionsRow->addWidget(action);
        if (number <= kMaxShortcuts) {
            const auto* shortcut = new QShortcut(QKeySequence(u"Ctrl+%1"_s.arg(number)), this);
            connect(shortcut, &QShortcut::activated, action, [action] {
                if (action->menu() != nullptr) {
                    action->showMenu();
                } else {
                    action->click();
                }
            });
        }
    }
    m_actionsRow->addStretch();
    if (!m_skills.isEmpty()) {
        selectSkill(m_skills.front().id);
    }
    applyTheme();
}

ChipButton* MainWindow::actionButton(int number, const QString& base, const QList<const SkillInfo*>& variants)
{
    // One button per skill; a skill with presets (rewrite's tones, format's layouts) opens a
    // short menu of them (main-window.html, note 2).
    const QString chip = number <= kMaxShortcuts ? tr("Ctrl %1").arg(number) : QString();
    auto* b = new ChipButton(actionTitle(base, variants), chip);
    b->setProperty("kind", u"action"_s);
    b->setObjectName(u"action-"_s + base);
    b->setCheckable(true);
    if (variants.size() == 1) {
        const QString id = variants.front()->id;
        connect(b, &QPushButton::clicked, this, [this, id] {
            selectSkill(id);
            run();
        });
        return b;
    }
    auto* menu = new QMenu(b);
    for (const SkillInfo* v : variants) {
        const QString id = v->id;
        QAction* item = menu->addAction(presetTitle(*v));
        item->setObjectName(id);
        connect(item, &QAction::triggered, this, [this, id] {
            selectSkill(id);
            run();
        });
    }
    b->setMenu(menu);
    // The menu opens on click; the button shows the action as selected only once one is picked.
    connect(b, &QPushButton::clicked, b, [this, b] {
        b->setChecked(skill(m_skill) != nullptr && u"action-"_s + skill(m_skill)->base == b->objectName());
    });
    return b;
}

void MainWindow::updateActionButtons(const SkillInfo& s)
{
    if (auto* action = findChild<ChipButton*>(u"action-"_s + s.base); action != nullptr) {
        action->setChecked(true);
        if (action->menu() != nullptr) {
            // The button names the preset in use: "Tone: Polite".
            QList<const SkillInfo*> variants;
            for (const SkillInfo& v : std::as_const(m_skills)) {
                if (v.base == s.base) {
                    variants.append(&v);
                }
            }
            action->setLabel(tr("%1: %2").arg(actionTitle(s.base, variants), presetTitle(s)));
        }
    }
    for (QAbstractButton* b : m_actionGroup->buttons()) {
        if (auto* chip = qobject_cast<ChipButton*>(b); chip != nullptr && chip->menu() != nullptr) {
            const QColor colour = chip->isChecked() ? QColor(Qt::white) : m_tokens.muted;
            chip->setTrailingIcon(Theme::icon(u"down"_s, colour, 14));
        }
    }
}

QString MainWindow::actionTitle(const QString& base, const QList<const SkillInfo*>& variants)
{
    if (base == u"rewrite"_s) {
        return tr("Tone");
    }
    if (base == u"reply"_s) {
        return tr("Reply");
    }
    // A preset's title is "<Skill>: <Preset>"; the button shows the skill's part.
    return variants.front()->title.section(u':', 0, 0).trimmed();
}

QString MainWindow::presetTitle(const SkillInfo& s)
{
    const QString preset = s.title.section(u':', 1).trimmed();
    return preset.isEmpty() ? s.title : preset;
}

const SkillInfo* MainWindow::skill(const QString& id) const
{
    for (const SkillInfo& s : m_skills) {
        if (s.id == id) {
            return &s;
        }
    }
    return nullptr;
}

MainWindow::Mode MainWindow::modeOf(const QString& skillId) const
{
    const SkillInfo* s = skill(skillId);
    return s != nullptr && s->base == u"reply"_s ? Mode::Reply : Mode::Text;
}

void MainWindow::selectSkill(const QString& skillId)
{
    const SkillInfo* s = skill(skillId);
    if (s == nullptr) {
        return;
    }
    const bool switchedMode = m_skill.isEmpty() || modeOf(m_skill) != modeOf(skillId);
    m_skill = skillId;
    updateActionButtons(*s);
    const bool reply = modeOf(skillId) == Mode::Reply;
    if (switchedMode) {
        // The text moves between the two input pages, so switching keeps what was typed.
        if (reply && m_message->toPlainText().isEmpty()) {
            m_message->setPlainText(m_text->toPlainText());
        } else if (!reply && m_text->toPlainText().isEmpty()) {
            m_text->setPlainText(m_message->toPlainText());
        }
    }
    m_inStack->setCurrentIndex(reply ? 1 : 0);
    m_inTitle->setText(reply ? tr("MESSAGE YOU GOT") : tr("YOUR TEXT"));
    // main-window-reply.html: the language as a pill, no word count, no Clear, no hint line.
    m_clear->setVisible(!reply);
    m_inFoot->setVisible(!reply);
    m_outStack->setCurrentIndex(reply ? 1 : 0);
    m_outTitle->setText((reply ? tr("Replies") : s->title).toUpper());
    m_segment->setVisible(s->base == u"proofread"_s);
    m_primary->setLabel(reply ? tr("Copy reply") : tr("Copy result"));
    m_secondary->setText(reply ? tr("3 new ones") : tr("Use as my text"));
    m_secondary->setIcon(reply ? Theme::icon(u"refresh"_s, m_tokens.text) : QIcon());
    m_retry->setText(reply ? tr("Make it polite") : tr("Retry"));
    m_retry->setIcon(reply ? QIcon() : Theme::icon(u"refresh"_s, m_tokens.text));
    updateInputInfo();
    setRunning(busy());
}

QVariantMap MainWindow::input() const
{
    if (modeOf(m_skill) == Mode::Reply) {
        QVariantMap in;
        if (const QString message = m_message->toPlainText(); !message.trimmed().isEmpty()) {
            in.insert(u"context"_s, message);
        }
        if (const QString draft = m_draft->toPlainText(); !draft.trimmed().isEmpty()) {
            in.insert(u"text"_s, draft);
        }
        return in;
    }
    const QString text = m_text->toPlainText();
    return text.trimmed().isEmpty() ? QVariantMap() : QVariantMap{{u"text"_s, text}};
}

QString MainWindow::languageName(const QString& text) const
{
    const QString code = m_detector.detect(text);
    const QString name = m_detector.plainName(code);
    return name.isEmpty() ? QString() : name.section(u" ("_s, 0, 0);
}

void MainWindow::updateInputInfo()
{
    const bool reply = modeOf(m_skill) == Mode::Reply;
    const QString text = (reply ? m_message : m_text)->toPlainText();
    m_langPill->setVisible(false);
    if (text.trimmed().isEmpty()) {
        m_inInfo->clear();
        return;
    }
    const auto words = text.split(QRegularExpression(u"\\s+"_s), Qt::SkipEmptyParts).size();
    const QString language = languageName(text);
    if (reply) {
        m_inInfo->clear();
        m_langPill->setText(language.isEmpty() ? tr("Best effort") : language);
        m_langPill->setVisible(true);
        return;
    }
    // Plurals written out: tr()'s %n needs a loaded translation to pluralise.
    const QString count = words == 1 ? tr("1 word") : tr("%1 words").arg(words);
    m_inInfo->setText(language.isEmpty() ? u"· "_s + count : u"· %1 · %2"_s.arg(count, language));
}

void MainWindow::warmLater()
{
    m_warmTimer.start();
}

void MainWindow::loadStatus()
{
    const QDBusMessage status = m_client.call(u"Status"_s, {});
    const QDBusMessage models = m_client.call(u"ListModels"_s, {});
    const QJsonObject s = parseJson(status.arguments().value(0).toString()).toObject();
    QString line;
    for (const auto& v : parseJson(models.arguments().value(0).toString()).toArray()) {
        const QJsonObject m = v.toObject();
        if (m.value(u"role"_s).toString() != u"model"_s || !m.value(u"installed"_s).toBool()) {
            continue;
        }
        const bool loaded = m.value(u"id"_s).toString() == s.value(u"model"_s).toString();
        const QString tier = m.value(u"tiers"_s).toArray().first().toString();
        const QString entry =
            tr("%1 (%2)").arg(m.value(u"name"_s).toString(), tier == u"fast"_s ? tr("fast") : tr("quality"));
        if (loaded) {
            line = tr("%1 · about %2 GB memory in use").arg(entry).arg(m.value(u"memory_gb"_s).toDouble());
            break;
        }
        if (line.isEmpty()) {
            line = tr("%1 ready").arg(entry);
        }
    }
    if (line.isEmpty()) {
        line = tr("No model is installed: run \"paraphrase models download fast\"");
        m_statusDot->setStyleSheet(u"color: %1;"_s.arg(m_tokens.warning.name()));
    }
    m_statusText->setText(line);
}

void MainWindow::run()
{
    if (m_skill.isEmpty() || busy()) {
        return;
    }
    const QVariantMap in = input();
    if (in.isEmpty()) {
        showMessage(modeOf(m_skill) == Mode::Reply ? tr("Paste the message you got first.")
                                                   : tr("Type or paste some text first."));
        return;
    }
    m_warmTimer.stop();
    const QDBusMessage reply = m_client.call(u"RunSkill"_s, {m_skill, in, QVariantMap()});
    if (reply.type() != QDBusMessage::ReplyMessage) {
        showMessage(reply.errorMessage(), u"danger"_s);
        Q_EMIT finished();
        return;
    }
    m_requestId = reply.arguments().value(0).toString();
    m_requestInput = in.value(u"text"_s).toString();
    m_politeForReply = false;
    m_streamed.clear();
    m_result.clear();
    m_diff.reset();
    m_undone.clear();
    m_output->clear();
    renderDiff();
    if (modeOf(m_skill) == Mode::Reply) {
        showReplies({});
    }
    m_outInfo->setProperty("role", u"muted"_s);
    m_outInfo->setText(tr("· writing..."));
    setRunning(true);
}

void MainWindow::cancel()
{
    if (busy()) {
        (void)m_client.call(u"Cancel"_s, {m_requestId});
    }
}

void MainWindow::setRunning(bool running)
{
    m_primary->setEnabled(!running);
    m_secondary->setEnabled(!running);
    if (modeOf(m_skill) == Mode::Text) {
        m_retry->setText(running ? tr("Stop") : tr("Retry"));
        m_retry->setIcon(Theme::icon(running ? u"stop"_s : u"refresh"_s, m_tokens.text));
    }
}

void MainWindow::onToken(const QString& id, int variant, const QString& text)
{
    if (id != m_requestId || variant != 0) {
        return;
    }
    m_streamed += text;
    // The answer streams in as the model writes; JSON replies wait for the whole set.
    if (modeOf(m_skill) == Mode::Text && !m_politeForReply) {
        m_output->setPlainText(m_streamed);
        m_output->moveCursor(QTextCursor::End);
    }
}

void MainWindow::onRetrying(const QString& id, const QString& reason)
{
    if (id != m_requestId) {
        return;
    }
    Q_UNUSED(reason)
    m_streamed.clear();
    m_output->clear();
    m_outInfo->setText(tr("· the answer was in another language; writing it again..."));
}

void MainWindow::onDone(const QString& id, const QString& result)
{
    if (id != m_requestId) {
        return;
    }
    m_requestId.clear();
    setRunning(false);
    const QJsonObject r = parseJson(result).toObject();
    const QString output = r.value(u"output"_s).toString();
    if (m_politeForReply) {
        m_politeForReply = false;
        m_replyEdit->setText(output.trimmed());
        m_outInfo->setText(tr("· made polite; copy it or pick another"));
        Q_EMIT finished();
        return;
    }
    const SkillInfo* s = skill(m_skill);
    const QString language =
        languageName(m_requestInput.isEmpty() ? m_message->toPlainText() : m_requestInput);
    if (modeOf(m_skill) == Mode::Reply) {
        QStringList replies;
        for (const auto& v : r.value(u"replies"_s).toArray()) {
            replies.append(v.toString());
        }
        showReplies(replies);
        m_outInfo->setText(language.isEmpty() ? QString() : tr("· in %1").arg(language));
    } else if (s != nullptr && s->base == u"proofread"_s) {
        showProofread(m_requestInput, output);
    } else {
        m_result = output.trimmed();
        m_output->setPlainText(m_result);
        m_outInfo->setText(language.isEmpty() ? QString() : tr("· kept in %1").arg(language));
    }
    // A failed language check is shown in words, the result never hidden (ADR-006).
    if (const QString warning = r.value(u"warning"_s).toString(); !warning.isEmpty()) {
        m_outInfo->setProperty("role", u"warning"_s);
        m_outInfo->setText(tr("· may not be in %1: check before using it")
                               .arg(language.isEmpty() ? tr("the right language") : language));
        m_outInfo->style()->polish(m_outInfo);
    }
    Q_EMIT finished();
}

void MainWindow::onError(const QString& id, const QString& code, const QString& message)
{
    if (id != m_requestId) {
        return;
    }
    m_requestId.clear();
    m_politeForReply = false;
    setRunning(false);
    if (code == u"cancelled"_s) {
        m_outInfo->setText(tr("· stopped"));
    } else {
        showMessage(message, u"danger"_s);
    }
    Q_EMIT finished();
}

void MainWindow::showMessage(const QString& message, const QString& role)
{
    m_outInfo->clear();
    const QColor color = role == u"danger"_s ? m_tokens.danger : m_tokens.muted;
    m_output->setHtml(u"<p style=\"color:%1\">%2</p>"_s.arg(color.name(), html(message)));
    if (modeOf(m_skill) == Mode::Reply) {
        m_outStack->setCurrentIndex(0);
    }
}

void MainWindow::showProofread(const QString& before, const QString& after)
{
    // The model returns the whole corrected text; the diff shows what changed (PLAN D6).
    QString corrected = after;
    if (!before.endsWith(u'\n') && corrected.endsWith(u'\n')) {
        corrected.chop(1);
    }
    m_diff.emplace(before, corrected);
    m_undone.clear();
    m_showChanges->setChecked(true);
    renderDiff();
}

void MainWindow::renderDiff()
{
    // Clear the change rows.
    while (const QLayoutItem* item = m_changesLayout->takeAt(0)) {
        if (QWidget* w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }
    if (!m_diff) {
        m_changesScroll->setVisible(false);
        return;
    }
    const client::WordDiff& diff = *m_diff;
    const auto changes = diff.changes();
    m_result = diff.result(m_undone);
    const QString language = languageName(m_requestInput);
    m_outInfo->setProperty("role", u"muted"_s);
    const QString count = changes.size() == 1 ? tr("1 change") : tr("%1 changes").arg(changes.size());
    m_outInfo->setText(changes.isEmpty()
                           ? tr("· nothing to fix")
                           : u"· "_s + count +
                                 (language.isEmpty() ? QString() : tr(" · kept in %1").arg(language)));
    if (m_showResult->isChecked()) {
        m_output->setPlainText(m_result);
    } else {
        m_output->setHtml(
            u"<div style=\"white-space: pre-wrap; font-size: 15px\">%1</div>"_s.arg(diffHtml(diff)));
    }
    // One row per change, each with its own Undo.
    m_changesScroll->setVisible(!changes.isEmpty());
    // Rows are 30 px: the list takes what it needs, up to a third of the pane.
    m_changesScroll->setMaximumHeight(std::min<int>((static_cast<int>(changes.size()) * 32) + 4, 220));
    for (const qsizetype k : changes) {
        m_changesLayout->addWidget(changeRow(diff, k));
    }
    m_changesLayout->addStretch();
}

QString MainWindow::diffHtml(const client::WordDiff& diff) const
{
    // Colour is never the only signal: deletions are struck out, insertions underlined.
    const auto& t = m_tokens;
    QString body;
    const auto& segments = diff.segments();
    for (qsizetype k = 0; k < segments.size(); ++k) {
        const auto& s = segments.at(k);
        if (!s.changed || m_undone.contains(k)) {
            body += html(s.changed ? s.before : s.after);
            continue;
        }
        if (!s.before.isEmpty()) {
            body += u"<span style=\"color:%1; background:%2; text-decoration:line-through\">%3</span>"_s.arg(
                t.del.name(), t.delBg.name(), html(s.before));
        }
        if (!s.before.isEmpty() && !s.after.isEmpty()) {
            body += u" "_s; // the old and the new word side by side, as in the mock
        }
        if (!s.after.isEmpty()) {
            body += u"<span style=\"color:%1; background:%2; text-decoration:underline\">%3</span>"_s.arg(
                t.ins.name(), t.insBg.name(), html(s.after));
        }
    }
    return body;
}

QWidget* MainWindow::changeRow(const client::WordDiff& diff, qsizetype k)
{
    // main-window.html, note 5: old word, arrow, new word, Undo. An addition shows where it
    // went ("end" for the end of a line) and a removal says so: colour is never the only signal.
    const auto& segments = diff.segments();
    const auto& s = segments.at(k);
    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);
    const QString before = s.before.trimmed();
    const QString after = s.after.trimmed();
    const auto chip = [](const QString& text, const QString& kind) {
        auto* l = new QLabel(text.isEmpty() ? u" "_s : text);
        l->setProperty("chip", kind);
        l->setTextFormat(Qt::PlainText);
        return l;
    };
    if (before.isEmpty()) {
        const bool atEnd = k + 1 >= segments.size() || segments.at(k + 1).after.startsWith(u'\n');
        layout->addWidget(chip(atEnd ? tr("end") : tr("new"), u"pill"_s));
    } else {
        layout->addWidget(chip(before, u"del"_s));
    }
    auto* arrow = new QLabel;
    arrow->setPixmap(Theme::icon(u"forward"_s, m_tokens.muted, 14).pixmap(14, 14));
    layout->addWidget(arrow);
    layout->addWidget(after.isEmpty() ? chip(tr("removed"), u"pill"_s) : chip(after, u"ins"_s));
    layout->addStretch(1);
    auto* undo = button(m_undone.contains(k) ? tr("Redo") : tr("Undo"), u"link"_s);
    undo->setObjectName(u"undo%1"_s.arg(k));
    connect(undo, &QPushButton::clicked, this, [this, k] {
        if (!m_undone.remove(k)) {
            m_undone.insert(k);
        }
        renderDiff();
    });
    layout->addWidget(undo);
    return row;
}

void MainWindow::showReplies(const QStringList& replies)
{
    m_outStack->setCurrentIndex(1);
    for (qsizetype i = 0; i < m_replyButtons.size(); ++i) {
        ChipButton* b = m_replyButtons.at(i);
        const QString text = replies.value(i);
        b->setProperty("reply", text);
        b->setLabel(text);
        b->setEnabled(!text.isEmpty());
        m_replyCopy.at(i)->setEnabled(!text.isEmpty());
    }
    if (!replies.isEmpty()) {
        m_replyButtons.front()->setChecked(true);
        m_replyEdit->setText(replies.front());
    } else {
        m_replyEdit->clear();
    }
}

QString MainWindow::resultText() const
{
    return modeOf(m_skill) == Mode::Reply ? m_replyEdit->text() : m_result;
}

void MainWindow::copyResult()
{
    // Copy is the main action here; nothing is ever typed into another app from this window.
    const QString text = resultText();
    if (!text.isEmpty()) {
        QApplication::clipboard()->setText(text);
        m_outInfo->setText(m_outInfo->text() + tr(" · copied"));
    }
}

} // namespace paraphrase::app

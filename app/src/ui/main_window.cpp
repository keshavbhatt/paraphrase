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
    static const QStringList kOrder = {u"proofread"_s, u"rewrite"_s, u"reply"_s};
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
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    auto* body = new QWidget;
    auto* main = new QVBoxLayout(body);
    main->setContentsMargins(20, 16, 20, 16);
    main->setSpacing(14);
    root->addWidget(body, 1);

    buildHeader(main);
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

void MainWindow::buildHeader(QVBoxLayout* main)
{
    // Header: the mark, and the privacy line on every surface (DESIGN.md 4).
    auto* header = new QHBoxLayout;
    m_mark = new QLabel;
    header->addWidget(m_mark);
    auto* name = label(u"<b>Paraphrase</b>"_s);
    name->setStyleSheet(u"font-size: 16px;"_s);
    header->addWidget(name);
    header->addStretch();
    m_lock = new QLabel;
    header->addWidget(m_lock);
    header->addWidget(label(tr("On this device"), u"private"_s));
    main->addLayout(header);

    // The notice for what the desktop does not offer (ADR-013): never an error, can be hidden.
    m_notice = new QFrame;
    m_notice->setObjectName(u"notice"_s);
    auto* notice = new QHBoxLayout(m_notice);
    notice->setContentsMargins(12, 10, 8, 10);
    m_noticeIcon = new QLabel;
    notice->addWidget(m_noticeIcon, 0, Qt::AlignTop);
    auto* noticeText =
        label(tr("Paraphrase cannot open over other apps yet, because no global shortcut is set "
                 "up. Everything works here: paste your text, pick an action, copy the result."));
    noticeText->setWordWrap(true);
    notice->addWidget(noticeText, 1);
    auto* hide = button(QString(), u"flat"_s);
    hide->setObjectName(u"hideNotice"_s);
    hide->setToolTip(tr("Hide this notice"));
    hide->setAccessibleName(tr("Hide this notice"));
    connect(hide, &QPushButton::clicked, m_notice, &QWidget::hide);
    notice->addWidget(hide, 0, Qt::AlignTop);
    main->addWidget(m_notice);
}

QWidget* MainWindow::buildInputPane()
{
    // Left pane: your text, or the message and draft for Reply.
    auto* left = new QFrame;
    left->setObjectName(u"pane"_s);
    auto* leftLayout = new QVBoxLayout(left);
    leftLayout->setContentsMargins(16, 14, 16, 14);
    auto* leftHead = new QHBoxLayout;
    m_inTitle = label(tr("YOUR TEXT"), u"caption"_s);
    leftHead->addWidget(m_inTitle);
    m_inInfo = label(QString(), u"muted"_s);
    leftHead->addWidget(m_inInfo);
    leftHead->addStretch();
    auto* paste = button(tr("Paste"), u"flat"_s);
    auto* clear = button(tr("Clear"), u"flat"_s);
    leftHead->addWidget(paste);
    leftHead->addWidget(clear);
    leftLayout->addLayout(leftHead);
    m_inStack = new QStackedWidget;
    m_text = new QPlainTextEdit;
    m_text->setObjectName(u"text"_s);
    m_text->setPlaceholderText(tr("Type or paste your text"));
    m_inStack->addWidget(m_text);
    auto* replyIn = new QWidget;
    auto* replyInLayout = new QVBoxLayout(replyIn);
    replyInLayout->setContentsMargins(0, 0, 0, 0);
    m_message = new QPlainTextEdit;
    m_message->setObjectName(u"message"_s);
    m_message->setPlaceholderText(tr("Paste the message you got"));
    replyInLayout->addWidget(m_message, 2);
    replyInLayout->addWidget(label(tr("Your reply so far (optional)"), u"muted"_s));
    m_draft = new QPlainTextEdit;
    m_draft->setObjectName(u"draft"_s);
    m_draft->setPlaceholderText(tr("Start typing and the replies will continue it"));
    replyInLayout->addWidget(m_draft, 1);
    m_inStack->addWidget(replyIn);
    leftLayout->addWidget(m_inStack, 1);
    m_inHint = label(tr("Type or paste anything. Ctrl+Enter runs the selected action."), u"muted"_s);
    leftLayout->addWidget(m_inHint);

    connect(paste, &QPushButton::clicked, this, [this] {
        QPlainTextEdit* target = m_inStack->currentIndex() == 0 ? m_text : m_message;
        target->setPlainText(QApplication::clipboard()->text());
    });
    connect(clear, &QPushButton::clicked, this, [this] {
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

QWidget* MainWindow::buildOutputPane()
{
    // Right pane: the result.
    auto* right = new QFrame;
    right->setObjectName(u"pane"_s);
    auto* rightLayout = new QVBoxLayout(right);
    rightLayout->setContentsMargins(16, 14, 16, 14);
    auto* rightHead = new QHBoxLayout;
    m_outTitle = label(QString(), u"caption"_s);
    rightHead->addWidget(m_outTitle);
    m_outInfo = label(QString(), u"muted"_s);
    m_outInfo->setObjectName(u"outInfo"_s);
    m_outInfo->setWordWrap(true);
    rightHead->addWidget(m_outInfo, 1);
    m_segment = new QWidget;
    auto* seg = new QHBoxLayout(m_segment);
    seg->setContentsMargins(0, 0, 0, 0);
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
    rightHead->addWidget(m_segment);
    rightLayout->addLayout(rightHead);

    m_outStack = new QStackedWidget;
    auto* textOut = new QWidget;
    auto* textOutLayout = new QVBoxLayout(textOut);
    textOutLayout->setContentsMargins(0, 0, 0, 0);
    m_output = new QTextBrowser;
    m_output->setObjectName(u"output"_s);
    m_output->setOpenLinks(false);
    textOutLayout->addWidget(m_output, 3);
    m_changes = new QWidget;
    m_changesLayout = new QVBoxLayout(m_changes);
    m_changesLayout->setContentsMargins(0, 0, 0, 0);
    m_changesLayout->setSpacing(2);
    auto* changesScroll = new QScrollArea;
    changesScroll->setWidgetResizable(true);
    changesScroll->setWidget(m_changes);
    changesScroll->setObjectName(u"changes"_s);
    textOutLayout->addWidget(changesScroll, 2);
    m_outStack->addWidget(textOut);

    m_outStack->addWidget(buildReplyOutput());
    rightLayout->addWidget(m_outStack, 1);
    rightLayout->addLayout(buildFooter());
    return right;
}

QWidget* MainWindow::buildReplyOutput()
{
    auto* replyOut = new QWidget;
    auto* replyOutLayout = new QVBoxLayout(replyOut);
    replyOutLayout->setContentsMargins(0, 0, 0, 0);
    for (int i = 0; i < 3; ++i) {
        auto* row = new QHBoxLayout;
        auto* choice = button(QString(), u"choice"_s);
        choice->setObjectName(u"reply%1"_s.arg(i + 1));
        choice->setCheckable(true);
        choice->setAutoExclusive(true);
        choice->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        connect(choice, &QPushButton::clicked, this,
                [this, choice] { m_replyEdit->setText(choice->property("reply").toString()); });
        auto* copy = button(QString());
        copy->setToolTip(tr("Copy reply %1").arg(i + 1));
        copy->setAccessibleName(tr("Copy reply %1").arg(i + 1));
        connect(copy, &QPushButton::clicked, this,
                [choice] { QApplication::clipboard()->setText(choice->property("reply").toString()); });
        row->addWidget(choice, 1);
        row->addWidget(copy);
        replyOutLayout->addLayout(row);
        m_replyButtons.append(choice);
        m_replyCopy.append(copy);
    }
    replyOutLayout->addWidget(label(tr("Edit before copying"), u"muted"_s));
    m_replyEdit = new QLineEdit;
    m_replyEdit->setObjectName(u"replyEdit"_s);
    replyOutLayout->addWidget(m_replyEdit);
    replyOutLayout->addStretch();

    return replyOut;
}

QHBoxLayout* MainWindow::buildFooter()
{
    auto* footer = new QHBoxLayout;
    m_primary = button(QString(), u"primary"_s);
    m_primary->setObjectName(u"copy"_s);
    m_secondary = button(QString());
    m_retry = button(QString());
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
            // "Make it polite": the Tone action on the chosen reply.
            const QString text = m_replyEdit->text().trimmed();
            if (text.isEmpty() || skill(u"rewrite.polite"_s) == nullptr) {
                return;
            }
            const QDBusMessage reply = m_client.call(
                u"RunSkill"_s, {u"rewrite.polite"_s, QVariantMap{{u"text"_s, text}}, QVariantMap()});
            if (reply.type() == QDBusMessage::ReplyMessage) {
                m_politeForReply = true;
                m_requestId = reply.arguments().value(0).toString();
                m_streamed.clear();
                setRunning(true);
            } else {
                showMessage(reply.errorMessage(), u"danger"_s);
            }
        } else {
            run();
        }
    });

    return footer;
}

QWidget* MainWindow::buildStatusBar()
{
    // Status line: which model answers; the privacy promise.
    auto* status = new QFrame;
    status->setObjectName(u"statusBar"_s);
    auto* statusLayout = new QHBoxLayout(status);
    statusLayout->setContentsMargins(20, 8, 20, 8);
    m_statusDot = label(u"●"_s);
    statusLayout->addWidget(m_statusDot);
    m_statusText = label(tr("Starting..."), u"muted"_s);
    m_statusText->setObjectName(u"statusText"_s);
    statusLayout->addWidget(m_statusText);
    statusLayout->addStretch();
    statusLayout->addWidget(label(tr("Nothing leaves this computer"), u"muted"_s));

    return status;
}

void MainWindow::applyTheme()
{
    m_tokens = Theme::current();
    setStyleSheet(Theme::styleSheet(m_tokens));
    m_mark->setPixmap(Theme::icon(u"pen"_s, m_tokens.accent, 20).pixmap(20, 20));
    m_lock->setPixmap(Theme::icon(u"lock"_s, m_tokens.privacy, 14).pixmap(14, 14));
    m_noticeIcon->setPixmap(Theme::icon(u"info"_s, m_tokens.accent, 16).pixmap(16, 16));
    if (auto* hide = m_notice->findChild<QPushButton*>(u"hideNotice"_s)) {
        hide->setIcon(Theme::icon(u"x"_s, m_tokens.muted));
    }
    m_primary->setIcon(Theme::icon(u"copy"_s, QColor(Qt::white)));
    m_retry->setIcon(Theme::icon(u"refresh"_s, m_tokens.text));
    for (QPushButton* copy : std::as_const(m_replyCopy)) {
        copy->setIcon(Theme::icon(u"copy"_s, m_tokens.text));
    }
    if (m_toneButton != nullptr) {
        m_toneButton->setIcon(Theme::icon(u"down"_s, m_tokens.muted));
    }
    m_statusDot->setStyleSheet(u"color: %1;"_s.arg(m_tokens.success.name()));
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
        const QString hint = number <= kMaxShortcuts ? u"Ctrl %1  "_s.arg(number) : QString();
        QAbstractButton* action =
            variants.size() == 1 ? actionButton(hint, *variants.front()) : presetButton(hint, base, variants);
        action->setObjectName(u"action-"_s + base);
        action->setCheckable(true);
        m_actionGroup->addButton(action);
        m_actionsRow->addWidget(action);
        if (number <= kMaxShortcuts) {
            const auto* shortcut = new QShortcut(QKeySequence(u"Ctrl+%1"_s.arg(number)), this);
            connect(shortcut, &QShortcut::activated, action, [action] {
                if (auto* tool = qobject_cast<QToolButton*>(action)) {
                    tool->showMenu();
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

QAbstractButton* MainWindow::actionButton(const QString& hint, const SkillInfo& s)
{
    auto* b = button(hint + s.title, u"action"_s);
    const QString id = s.id;
    connect(b, &QPushButton::clicked, this, [this, id] {
        selectSkill(id);
        run();
    });
    return b;
}

QAbstractButton* MainWindow::presetButton(const QString& hint, const QString& base,
                                          const QList<const SkillInfo*>& variants)
{
    // A skill with presets (rewrite's tones) opens a short menu (main-window.html, note 2).
    const QString title = base == u"rewrite"_s ? tr("Tone") : variants.front()->title.section(u':', 0, 0);
    auto* b = new QToolButton;
    b->setProperty("kind", u"action"_s);
    b->setText(hint + title);
    b->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    b->setLayoutDirection(Qt::RightToLeft); // the arrow after the label
    b->setPopupMode(QToolButton::InstantPopup);
    b->setCursor(Qt::PointingHandCursor);
    auto* menu = new QMenu(b);
    for (const SkillInfo* v : variants) {
        const QString id = v->id;
        QAction* item = menu->addAction(v->title);
        item->setObjectName(id);
        connect(item, &QAction::triggered, this, [this, id] {
            selectSkill(id);
            run();
        });
    }
    b->setMenu(menu);
    if (base == u"rewrite"_s) {
        m_toneButton = b;
    }
    return b;
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
    if (auto* action = findChild<QAbstractButton*>(u"action-"_s + s->base)) {
        action->setChecked(true);
        if (s->base == u"rewrite"_s) {
            action->setText(action->text().section(u"Tone"_s, 0, 0) + tr("Tone: %1").arg(s->title));
        }
    }
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
    m_outStack->setCurrentIndex(reply ? 1 : 0);
    m_outTitle->setText((reply ? tr("Replies") : s->title).toUpper());
    m_segment->setVisible(s->base == u"proofread"_s);
    m_primary->setText(reply ? tr("Copy reply") : tr("Copy result"));
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
    if (text.trimmed().isEmpty()) {
        m_inInfo->clear();
        return;
    }
    const auto words = text.split(QRegularExpression(u"\\s+"_s), Qt::SkipEmptyParts).size();
    const QString language = languageName(text);
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
        m_changes->parentWidget()->parentWidget()->setVisible(false);
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
    m_changes->parentWidget()->parentWidget()->setVisible(!changes.isEmpty());
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
    const auto& s = diff.segments().at(k);
    auto* row = new QWidget;
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(4, 2, 4, 2);
    // An addition or a removal says so in words (colour is never the only signal).
    const QString before = s.before.trimmed();
    const QString after = s.after.trimmed();
    const QString del = u"<span style=\"color:%1; text-decoration:line-through\">%2</span>"_s.arg(
        m_tokens.del.name(), before.toHtmlEscaped());
    const QString ins = u"<span style=\"color:%1; text-decoration:underline\">%2</span>"_s.arg(
        m_tokens.ins.name(), after.toHtmlEscaped());
    const QString muted = u"<span style=\"color:%1\">%2</span>"_s;
    QString line;
    if (before.isEmpty()) {
        line = muted.arg(m_tokens.muted.name(), tr("added")) + u" "_s + ins;
    } else if (after.isEmpty()) {
        line = muted.arg(m_tokens.muted.name(), tr("removed")) + u" "_s + del;
    } else {
        line = del + u" &rarr; "_s + ins;
    }
    layout->addWidget(new QLabel(line), 1);
    auto* undo = button(m_undone.contains(k) ? tr("Redo") : tr("Undo"), u"flat"_s);
    undo->setObjectName(u"undo%1"_s.arg(k));
    undo->setStyleSheet(u"color: %1;"_s.arg(m_tokens.accent.name()));
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
        QPushButton* b = m_replyButtons.at(i);
        const QString text = replies.value(i);
        b->setProperty("reply", text);
        b->setText(text.isEmpty() ? QString() : u"%1   %2"_s.arg(i + 1).arg(text));
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

#include "ui/main/main_window.h"

#include "history/history_keeper.h"
#include "ui/common/menu_icons.h"
#include "ui/core/facts_check.h"
#include "ui/core/languages.h"
#include "ui/core/request_runner.h"
#include "ui/core/result_summary.h"
#include "ui/core/user_actions_client.h"
#include "ui/main/bars.h"
#include "ui/main/history_view.h"
#include "ui/main/input_pane.h"
#include "ui/main/output_pane.h"
#include "ui/parts/action_bar.h"
#include "ui/parts/input_pages.h"
#include "ui/parts/result_views.h"

#include <QApplication>
#include <QClipboard>
#include <QDBusMessage>
#include <QEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QResizeEvent>
#include <QSettings>
#include <QShortcut>
#include <QSplitter>
#include <QStackedWidget>
#include <QVBoxLayout>

#include <utility>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kRunIconSize = 14;
constexpr int kNarrowWidth = 820; ///< below this the panes stack (main-window.html, note 6)
const QString kFullReplyKey = u"reply/full"_s;

QJsonValue parseJson(const QString& text)
{
    const QJsonDocument doc = QJsonDocument::fromJson(text.toUtf8());
    return doc.isArray() ? QJsonValue(doc.array()) : QJsonValue(doc.object());
}

} // namespace

MainWindow::MainWindow(client::DaemonClient* client, QWidget* parent)
    : QWidget(parent)
    , m_ownClient(client == nullptr ? std::make_unique<client::DaemonClient>() : nullptr)
    , m_client(client == nullptr ? *m_ownClient : *client)
    , m_detector(util::LanguageData::builtIn())
    , m_tokens(Theme::current())
    , m_runner(new RequestRunner(m_client, this))
{
    setObjectName(u"mainWindow"_s);
    setWindowTitle(u"Paraphrase"_s);
    resize(1180, 760);
    buildUi();
    connectParts();
    applyTheme();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi()
{
    // main-window.html: the header and notice bands, the actions and the two panes on the
    // window background, the status band.
    m_header = new HeaderBar;
    m_notice = new NoticeBar;
    m_notice->setNotice(
        tr("This desktop gives Paraphrase no global shortcut, so it cannot open over other apps. "
           "Everything works here: paste your text, pick an action, copy the result."));
    m_actions = new ActionBar(m_detector, ActionBar::Keys::Ctrl);
    m_input = new InputPane(m_detector);
    m_output = new OutputPane;
    m_status = new StatusBar;
    m_panes = new QSplitter(Qt::Horizontal);
    m_panes->setChildrenCollapsible(false);
    m_panes->setHandleWidth(14);
    m_panes->addWidget(m_input);
    m_panes->addWidget(m_output);

    m_body = new QWidget;
    auto* main = new QVBoxLayout(m_body);
    main->setContentsMargins(16, 14, 16, 16);
    main->setSpacing(14);
    main->addWidget(m_actions);
    main->addWidget(m_panes, 1);

    // Writing and history share one place. A stack swaps them in one step and is as tall
    // as the taller of the two, so opening or closing history never resizes the window.
    auto* writing = new QWidget;
    auto* writingColumn = new QVBoxLayout(writing);
    writingColumn->setContentsMargins(0, 0, 0, 0);
    writingColumn->setSpacing(0);
    writingColumn->addWidget(m_body, 1);
    writingColumn->addWidget(m_status);
    m_area = new QStackedWidget;
    m_area->addWidget(writing);

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(m_header);
    root->addWidget(m_notice);
    root->addWidget(m_area, 1);
}

void MainWindow::connectParts()
{
    connect(m_header, &HeaderBar::settingsClicked, this, &MainWindow::settingsRequested);
    connect(m_header, &HeaderBar::historyClicked, this, [this] { showHistory(!historyOpen()); });
    connect(m_notice, &NoticeBar::linkClicked, this, &MainWindow::shortcutHelpRequested);
    connect(m_actions, &ActionBar::chosen, this, [this](const QString& id, const QString& lang) {
        m_targetLang = lang;
        selectSkill(id);
        run();
    });
    connect(m_actions, &ActionBar::newActionRequested, this, &MainWindow::newActionRequested);
    // Reply and Write open their form and wait for it (main-window-run.html note 4).
    connect(m_actions, &ActionBar::replyChosen, this, [this] {
        chooseReply(m_fullReply);
        runOrWait();
    });
    connectInput();
    connect(m_output, &OutputPane::copyClicked, this, &MainWindow::copyResult);
    connect(m_output, &OutputPane::secondaryClicked, this, &MainWindow::onSecondary);
    connect(m_output, &OutputPane::retryClicked, this, &MainWindow::onRetry);
    connect(m_output->textView(), &TextResultView::diffChanged, this, [this] { showResult({}); });
    connect(m_runner, &RequestRunner::token, this, &MainWindow::onToken);
    connect(m_runner, &RequestRunner::retrying, this, [this] {
        m_streamed.clear();
        m_output->setStep(OutputPane::Step::Retrying);
    });
    connect(m_runner, &RequestRunner::finished, this, &MainWindow::onFinished);
    connect(m_runner, &RequestRunner::failed, this, &MainWindow::onFailed);

    // Keyboard first (DESIGN.md 4).
    connect(new QShortcut(QKeySequence(u"Ctrl+Return"_s), this), &QShortcut::activated, this,
            &MainWindow::run);
    connect(new QShortcut(QKeySequence(u"Ctrl+Shift+C"_s), this), &QShortcut::activated, this,
            &MainWindow::copyResult);
    connect(new QShortcut(QKeySequence(Qt::Key_Escape), this), &QShortcut::activated, this,
            &MainWindow::cancel);
}

void MainWindow::connectInput()
{
    m_actions->setTextLanguage([this] { return m_input->language(); });
    connect(m_input, &InputPane::fullReplyToggled, this, [this](bool full) {
        chooseReply(full);
        rerun();
    });
    connect(m_input->fullReplyInput(), &FullReplyInput::kindChanged, this, [this] {
        chooseReply(true);
        rerun();
    });
    connect(m_input->fullReplyInput(), &FullReplyInput::toneChanged, this, &MainWindow::rerun);
    connect(m_actions, &ActionBar::composeChosen, this, [this] {
        chooseCompose();
        runOrWait();
    });
    connect(m_input->composeInput(), &ComposeInput::kindChanged, this, [this] {
        chooseCompose();
        rerun();
    });
    connect(m_input->composeInput(), &ComposeInput::toneChanged, this, &MainWindow::rerun);
    connect(m_input, &InputPane::runClicked, this, &MainWindow::run);
    connect(m_input, &InputPane::edited, this, [this] {
        if (m_input->page()->subject().trimmed().isEmpty()) {
            m_ran = false; // cleared: options wait for the next run
        }
        updateRun();
        if (const SkillInfo* s = m_catalog.find(m_skill)) {
            m_runner->warmLater(m_skill, m_input->page()->input(*s, m_targetLang));
        }
    });
}

void MainWindow::setHistory(history::Keeper* keeper)
{
    if (m_history != nullptr || keeper == nullptr) {
        return;
    }
    m_history = keeper;
    m_historyView = new HistoryView(*keeper);
    m_area->addWidget(m_historyView);
    m_historyView->applyTheme(m_tokens);
    m_header->setHistoryShown(true);
    connect(m_historyView, &HistoryView::backRequested, this, [this] { showHistory(false); });
    connect(m_historyView, &HistoryView::privacyRequested, this, &MainWindow::privacyRequested);
    connect(m_historyView, &HistoryView::useAgain, this, &MainWindow::useAgain);
    connect(new QShortcut(QKeySequence(u"Ctrl+H"_s), this), &QShortcut::activated, this,
            [this] { showHistory(!historyOpen()); });
}

void MainWindow::showHistory(bool open)
{
    if (m_historyView == nullptr) {
        return;
    }
    // In place of the panes; the text on the left stays as it was (note 1).
    if (open) {
        m_historyView->reload();
    }
    m_area->setCurrentIndex(open ? 1 : 0);
    m_header->setHistoryOpen(open);
    if (open) {
        m_historyView->setFocus();
    } else {
        m_input->setFocus();
    }
}

bool MainWindow::historyOpen() const
{
    return m_historyView != nullptr && m_area->currentWidget() == m_historyView;
}

void MainWindow::useAgain(const history::Entry& entry)
{
    // The text back on the left with the same action and options, ready to run (note 4).
    showHistory(false);
    m_targetLang = QJsonDocument::fromJson(entry.options).object().value(u"target_lang"_s).toString();
    if (m_catalog.find(entry.skillId) != nullptr) {
        selectSkill(entry.skillId);
    }
    m_input->page()->setSubject(entry.text);
    m_ran = false;
    m_output->textView()->showMessage(tr("Your text is back. Run it again when you are ready."), false);
}

void MainWindow::applyTheme()
{
    m_tokens = Theme::current();
    setStyleSheet(Theme::styleSheet(m_tokens));
    m_header->applyTheme(m_tokens);
    m_notice->applyTheme(m_tokens);
    m_actions->applyTheme(m_tokens);
    m_input->fullReplyInput()->applyTheme(m_tokens);
    m_input->composeInput()->applyTheme(m_tokens);
    m_output->applyTheme(m_tokens);
    m_status->applyTheme(m_tokens);
    updateRun();
    if (m_historyView != nullptr) {
        m_historyView->applyTheme(m_tokens);
    }
}

void MainWindow::changeEvent(QEvent* event)
{
    if ((event->type() == QEvent::ThemeChange || event->type() == QEvent::PaletteChange) &&
        Theme::current().dark != m_tokens.dark) {
        applyTheme();
    }
    QWidget::changeEvent(event);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    m_panes->setOrientation(event->size().width() < kNarrowWidth ? Qt::Vertical : Qt::Horizontal);
    QWidget::resizeEvent(event);
}

void MainWindow::takeText(const QString& text)
{
    if (m_catalog.kindOf(m_skill) != SkillKind::Text && !m_catalog.isEmpty()) {
        selectSkill(m_catalog.skills().front().id);
    }
    m_input->page()->setSubject(text);
}

void MainWindow::setGlobalShortcut(bool available)
{
    if (available) {
        m_notice->setNotice({});
        return;
    }
    m_notice->setNotice(
        tr("This desktop gives Paraphrase no global shortcut, so it cannot open over other apps. "
           "Everything works here: paste your text, pick an action, copy the result."),
        tr("Add a shortcut by hand"));
}

void MainWindow::start()
{
    m_output->textView()->showMessage(tr("Starting Paraphrase..."), false);
    if (const QString problem = m_client.connectToDaemon(); !problem.isEmpty()) {
        m_status->showProblem(problem);
        m_output->textView()->showMessage(problem, true);
        return;
    }
    loadSkills();
    connect(&m_client, &client::DaemonClient::skillsChanged, this, &MainWindow::loadSkills,
            Qt::UniqueConnection);
    m_fullReply = QSettings().value(kFullReplyKey).toBool();
    if (!m_catalog.isEmpty()) {
        selectSkill(m_catalog.skills().front().id);
    }
    loadStatus();
    m_output->textView()->showMessage(tr("Paste or type your text on the left, then pick an action."), false);
    applyTheme();
    Q_EMIT actionsLoaded();
    if (const auto pending = std::exchange(m_pending, std::nullopt)) {
        takeOver(*pending);
    }
}

void MainWindow::loadSkills()
{
    const QDBusMessage reply = m_client.call(u"ListSkills"_s, {});
    m_catalog = SkillCatalog::fromJson(reply.arguments().value(0).toString());
    UserActionsClient actions(m_client);
    if (actions.load()) {
        m_catalog.setUserActionsBase(actions.templateSkill());
    }
    m_actions->setCatalog(&m_catalog);
    m_actions->applyTheme(m_tokens);
    m_input->fullReplyInput()->setTones(m_catalog.tones());
    m_input->composeInput()->setTones(m_catalog.tones());
    m_input->setFullReplyAvailable(!m_catalog.fullReplyId(false).isEmpty());
    if (const SkillInfo* current = m_catalog.find(m_skill)) {
        m_actions->setCurrent(*current, m_targetLang);
    }
}

void MainWindow::loadStatus()
{
    const QDBusMessage status = m_client.call(u"Status"_s, {});
    const QDBusMessage models = m_client.call(u"ListModels"_s, {});
    m_status->showModels(parseJson(status.arguments().value(0).toString()).toObject(),
                         parseJson(models.arguments().value(0).toString()).toArray());
}

void MainWindow::chooseCompose()
{
    selectSkill(m_catalog.composeId(m_input->composeInput()->kind()));
}

void MainWindow::chooseReply(bool full)
{
    const QString fullId = m_catalog.fullReplyId(m_input->fullReplyInput()->chat());
    m_fullReply = full && !fullId.isEmpty();
    QSettings().setValue(kFullReplyKey, m_fullReply);
    selectSkill(m_fullReply ? fullId : u"reply"_s);
}

void MainWindow::selectSkill(const QString& skillId)
{
    const SkillInfo* s = m_catalog.find(skillId);
    if (s == nullptr) {
        return;
    }
    m_skill = skillId;
    const SkillKind kind = m_catalog.kindOf(skillId);
    m_actions->setCurrent(*s, m_targetLang);
    m_input->showKind(kind);
    m_input->setFullReply(kind == SkillKind::FullReply);
    QString title = s->title.section(u':', 0, 0);
    if (kind == SkillKind::QuickReply) {
        title = tr("Replies");
    } else if (kind == SkillKind::FullReply) {
        title = tr("Your reply");
    } else if (kind == SkillKind::Compose) {
        title = tr("Your text");
    }
    m_output->showKind(kind, title, s->base == u"proofread"_s);
    if (const QString chosen = chosenPreset(*s); !chosen.isEmpty()) {
        m_output->setInfo(u"· "_s + chosen);
    }
    m_output->setRunning(busy());
    updateRun();
}

QString MainWindow::runLabel(const SkillInfo& s) const
{
    switch (m_catalog.kindOf(s.id)) {
    case SkillKind::QuickReply:
        return tr("Write 3 replies");
    case SkillKind::FullReply:
        return tr("Write the reply");
    case SkillKind::Compose:
        return tr("Write it");
    case SkillKind::Text:
        break;
    }
    if (m_catalog.picksLanguage(s.base)) {
        return m_targetLang.isEmpty() ? s.title.section(u':', 0, 0)
                                      : tr("Into %1").arg(languageName(m_detector, m_targetLang));
    }
    return s.title.contains(u':') ? SkillCatalog::presetTitle(s) : s.title;
}

QString MainWindow::runKey() const
{
    const SkillInfo* s = m_catalog.find(m_skill);
    if (s == nullptr) {
        return {};
    }
    const QJsonObject in = QJsonObject::fromVariantMap(m_input->page()->input(*s, m_targetLang));
    return m_skill + u'\n' + m_targetLang + u'\n' + QString::fromUtf8(QJsonDocument(in).toJson());
}

void MainWindow::updateRun()
{
    const SkillInfo* s = m_catalog.find(m_skill);
    if (s == nullptr) {
        return;
    }
    const QString icon = actionIcon(m_skill);
    m_input->setRunAction(runLabel(*s), Theme::icon(icon, QColor(Qt::white), kRunIconSize),
                          Theme::icon(icon, m_tokens.text, kRunIconSize));
    // Filled while no result for this text and these options is showing (note 3).
    m_input->setRunState(m_resultKey.isEmpty() || m_resultKey != runKey(), busy());
}

void MainWindow::runOrWait()
{
    if (m_input->isEmpty()) {
        m_input->focusField();
        return;
    }
    run();
}

QString MainWindow::chosenPreset(const SkillInfo& s) const
{
    // Which tone, format, language or action of the user's: the chips keep their own labels.
    if (m_catalog.kindOf(s.id) != SkillKind::Text) {
        return {};
    }
    if (m_catalog.picksLanguage(s.base)) {
        return m_targetLang.isEmpty() ? QString() : tr("into %1").arg(languageName(m_detector, m_targetLang));
    }
    if (!s.title.contains(u':')) {
        return {};
    }
    const QString preset = SkillCatalog::presetTitle(s);
    return s.base == u"custom"_s ? preset : preset.toLower(); // the user's own names keep their case
}

bool MainWindow::busy() const
{
    return m_runner->busy();
}

void MainWindow::run()
{
    const SkillInfo* s = m_catalog.find(m_skill);
    if (s == nullptr || busy()) {
        return;
    }
    const SkillKind kind = m_catalog.kindOf(m_skill);
    const QVariantMap in = m_input->page()->input(*s, m_targetLang);
    if (in.isEmpty()) {
        QString hint = tr("Paste the message you got first.");
        if (kind == SkillKind::Text) {
            hint = tr("Type or paste some text first.");
        } else if (kind == SkillKind::Compose) {
            hint = tr("Say what to write first.");
        }
        m_output->textView()->showMessage(hint, false);
        m_output->showKind(SkillKind::Text, s->title.section(u':', 0, 0), false);
        return;
    }
    // "Another version", "3 new ones" and Retry ask for one that differs; a first run does not.
    const bool variety = std::exchange(m_variety, false);
    m_lastVariety = variety;
    if (const QString problem = m_runner->start(m_skill, in, variety); !problem.isEmpty()) {
        m_output->textView()->showMessage(problem, true);
        Q_EMIT finished();
        return;
    }
    m_requestInput = in.value(s->mainInputIsContext() ? u"context"_s : u"text"_s).toString();
    m_runKey = runKey();
    m_resultKey.clear();
    m_rewriting = false;
    m_ran = true;
    m_streamed.clear();
    selectSkill(m_skill); // the right view, also after a message was shown
    if (kind == SkillKind::Text) {
        m_output->textView()->clear();
    } else if (kind == SkillKind::QuickReply) {
        m_output->repliesView()->setReplies({});
    } else if (const QString key = m_skill + m_requestInput + in.value(u"intent"_s).toString();
               key != m_fullReplyKey) {
        m_fullReplyKey = key; // a new message or kind: start the versions again
        m_output->fullView()->reset();
    }
    m_output->startWork();
}

void MainWindow::cancel()
{
    m_rerunAfterStop = false;
    m_runner->cancel();
}

void MainWindow::rerun()
{
    if (!m_ran) {
        return; // nothing on screen yet: Ctrl+Enter or an action runs it
    }
    if (busy()) {
        m_runner->cancel();
        m_rerunAfterStop = true;
        return;
    }
    run();
}

void MainWindow::onSecondary()
{
    if (m_catalog.kindOf(m_skill) != SkillKind::Text) {
        m_variety = true;
        run(); // three new ones, another version
        return;
    }
    // "Use as my text": run another action on the result, e.g. Proofread then Tone.
    m_input->page()->setSubject(resultText());
    m_output->textView()->clear();
    m_output->setInfo(QString());
}

void MainWindow::onRetry()
{
    const SkillKind kind = m_catalog.kindOf(m_skill);
    if (busy()) {
        cancel();
    } else if (kind == SkillKind::QuickReply) {
        rewriteResult(u"rewrite.polite"_s);
    } else if (writesDraft(kind)) {
        rewriteResult(u"rewrite.concise"_s);
    } else {
        m_variety = true; // Retry: another take on the same text
        run();
    }
}

void MainWindow::rewriteResult(const QString& skillId)
{
    const QString text = resultText().trimmed();
    if (text.isEmpty() || m_catalog.find(skillId) == nullptr) {
        return;
    }
    if (const QString problem = m_runner->start(skillId, {{u"text"_s, text}}); !problem.isEmpty()) {
        m_output->setInfo(problem, u"danger"_s);
        return;
    }
    m_rewriting = true;
    m_streamed.clear();
    m_output->startWork(false); // the result stays on screen until its new version comes
    updateRun();
}

void MainWindow::onToken(const QString& piece)
{
    m_streamed += piece;
    m_output->setStep(OutputPane::Step::Writing);
    // The answer streams in as the model writes; quick replies wait for the whole set.
    const SkillKind kind = m_catalog.kindOf(m_skill);
    if (m_rewriting || kind == SkillKind::QuickReply) {
        return;
    }
    if (writesDraft(kind)) {
        m_output->fullView()->stream(m_streamed);
    } else {
        m_output->textView()->stream(m_streamed);
    }
}

void MainWindow::onFinished(const QJsonObject& result)
{
    m_output->setRunning(false);
    updateRun();
    if (m_rerunAfterStop) {
        // An option changed while this ran and the answer came before the stop: it is stale.
        m_rerunAfterStop = false;
        m_rewriting = false;
        Q_EMIT finished();
        run();
        return;
    }
    const QString output = result.value(u"output"_s).toString().trimmed();
    const SkillKind kind = m_catalog.kindOf(m_skill);
    if (m_rewriting) {
        m_rewriting = false;
        if (kind == SkillKind::QuickReply) {
            m_output->repliesView()->setChosen(output);
            m_output->setInfo(tr("· made polite; copy it or pick another"));
        } else {
            m_output->fullView()->addVersion(output);
            m_output->setInfo(tr("· shorter version"));
        }
        keepInHistory(true, {});
        Q_EMIT finished();
        return;
    }
    const QStringList replies = display(result);
    m_resultKey = m_runKey;
    updateRun();
    keepInHistory(m_lastVariety, replies.join(u"\n\n"_s));
    loadStatus(); // the model that answered, and its memory
    Q_EMIT finished();
}

void MainWindow::keepInHistory(bool sameRun, const QString& replies)
{
    const SkillInfo* s = m_catalog.find(m_skill);
    if (m_history == nullptr || s == nullptr) {
        return;
    }
    // Quick replies keep all three; a chosen or polite one replaces them when copied.
    const QString result = replies.isEmpty() ? resultText() : replies;
    if (sameRun && m_historyId != 0 && m_history->update(m_historyId, result)) {
        return; // another version of the same run: the entry keeps the newest
    }
    const bool chosen = !m_targetLang.isEmpty() && m_catalog.picksLanguage(s->base);
    const bool proofread = s->base == u"proofread"_s && m_output->textView()->hasDiff();
    history::Entry entry;
    entry.skillId = m_skill;
    entry.label = historyLabel(*s, chosen ? languageName(m_detector, m_targetLang) : QString(),
                               proofread ? static_cast<int>(m_output->textView()->changeCount()) : -1);
    entry.text = m_requestInput;
    entry.result = result;
    QJsonObject options;
    if (!m_targetLang.isEmpty()) {
        options.insert(u"target_lang"_s, m_targetLang);
    }
    entry.options = QJsonDocument(options).toJson(QJsonDocument::Compact);
    m_historyId = m_history->record(entry);
}

QStringList MainWindow::display(const QJsonObject& result)
{
    const QString output = result.value(u"output"_s).toString().trimmed();
    const SkillKind kind = m_catalog.kindOf(m_skill);
    QStringList replies;
    if (kind == SkillKind::QuickReply) {
        for (const auto& v : result.value(u"replies"_s).toArray()) {
            replies.append(v.toString());
        }
        m_output->repliesView()->setReplies(replies);
    } else if (writesDraft(kind)) {
        m_output->fullView()->addVersion(output);
    } else if (m_catalog.find(m_skill)->base == u"proofread"_s) {
        m_output->textView()->showProofread(m_requestInput, result.value(u"output"_s).toString());
    } else {
        m_output->textView()->showText(output);
    }
    showResult(result);
    return replies;
}

void MainWindow::takeOver(const Handover& handover)
{
    if (m_catalog.isEmpty()) {
        m_pending = handover; // the actions are not loaded yet: start() applies it
        return;
    }
    showHistory(false);
    if (busy()) {
        cancel();
    }
    if (handover.skillId.isEmpty()) {
        takeText(handover.text);
        return;
    }
    // popup-open-in-window.html, note 4: Reply opens on Full reply with the message, waiting
    // for what the user wants to say. Nothing runs until they press Write the reply.
    const SkillKind kind = m_catalog.kindOf(handover.skillId);
    if (kind == SkillKind::QuickReply || kind == SkillKind::FullReply) {
        m_input->fullReplyInput()->takeMessage(handover.text);
        chooseReply(true);
        m_ran = false;
        m_input->fullReplyInput()->focusIntent();
        return;
    }
    // Note 5: the same action and language, and the popup's result without running again.
    m_targetLang = handover.targetLang;
    selectSkill(handover.skillId);
    m_input->page()->setSubject(handover.text);
    m_ran = false;
    if (handover.result.isEmpty()) {
        m_output->textView()->showMessage(tr("Your text is here. Run it when you are ready."), false);
        return;
    }
    m_requestInput = handover.text;
    m_runKey = runKey();
    display(handover.result);
    m_resultKey = m_runKey;
    m_ran = true;
    updateRun();
}

void MainWindow::showResult(const QJsonObject& result)
{
    // The line after the title: what happened, in which language.
    const SkillInfo* s = m_catalog.find(m_skill);
    const SkillKind kind = m_catalog.kindOf(m_skill);
    const bool chosen = s != nullptr && !m_targetLang.isEmpty() && m_catalog.picksLanguage(s->base);
    QString replyKind;
    if (kind == SkillKind::FullReply) {
        replyKind = m_input->fullReplyInput()->chat() ? tr("chat message") : tr("email");
    } else if (kind == SkillKind::Compose && s != nullptr) {
        replyKind = SkillCatalog::presetTitle(*s).toLower();
    } else if (s != nullptr && !m_catalog.picksLanguage(s->base)) {
        replyKind = chosenPreset(*s);
    }
    const ResultFacts facts{
        .kind = kind,
        .proofread = kind == SkillKind::Text && m_output->textView()->hasDiff(),
        .changes = m_output->textView()->changeCount(),
        .language = chosen ? languageName(m_detector, m_targetLang) : inputLanguageName(),
        .chosenLanguage = chosen,
        .replyKind = replyKind,
        .facts = {},
    };
    ResultFacts shown = facts;
    FactsCheck check;
    if (s != nullptr && keepsFacts(s->base)) {
        const bool sameLanguage = !chosen || m_targetLang == m_detector.detect(m_requestInput);
        check = checkFacts(m_requestInput, m_output->result(), sameLanguage);
        shown.facts = factsLine(check);
    }
    m_output->setInfo(resultSummary(shown), check.missing.isEmpty() ? u"small"_s : u"warning"_s);
    if (!result.value(u"warning"_s).toString().isEmpty()) {
        m_output->setInfo(languageWarning(facts.language), u"warning"_s);
    }
}

void MainWindow::onFailed(const QString& code, const QString& message)
{
    m_rewriting = false;
    m_output->setRunning(false);
    updateRun();
    if (code == u"cancelled"_s && m_rerunAfterStop) {
        m_rerunAfterStop = false;
        Q_EMIT finished();
        run(); // stopped for an option change: run with the new one
        return;
    }
    if (code == u"cancelled"_s) {
        m_output->setInfo(tr("· stopped"));
    } else {
        m_output->showKind(SkillKind::Text, m_catalog.find(m_skill)->title.section(u':', 0, 0), false);
        m_output->textView()->showMessage(message, true);
        m_output->setInfo(QString());
    }
    Q_EMIT finished();
}

QString MainWindow::inputLanguageName() const
{
    return m_requestInput.trimmed().isEmpty() ? QString()
                                              : languageName(m_detector, m_detector.detect(m_requestInput));
}

QString MainWindow::resultText() const
{
    return m_output->result();
}

void MainWindow::copyResult()
{
    // Copy is the main action here; nothing is ever typed into another app from this window.
    const QString text = resultText();
    if (!text.isEmpty()) {
        QApplication::clipboard()->setText(text);
        if (m_history != nullptr && m_historyId != 0) {
            m_history->update(m_historyId, text); // the version copied, edited or not
        }
        m_output->setInfo(m_output->info() + tr(" · copied"));
    }
}

} // namespace paraphrase::app

#include "popup/popup_controller.h"

#include "history/history_keeper.h"
#include "integration/integrations.h"
#include "popup/popup_pages.h"
#include "popup/popup_window.h"
#include "ui/common/menu_icons.h"
#include "ui/core/app_names.h"
#include "ui/core/facts_check.h"
#include "ui/core/languages.h"
#include "ui/core/request_runner.h"
#include "ui/core/result_summary.h"
#include "ui/core/user_actions_client.h"
#include "ui/parts/action_bar.h"
#include "ui/parts/result_views.h"

#include <QClipboard>
#include <QCursor>
#include <QDBusMessage>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSettings>

#include <utility>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

const QString kLastKey = u"popup/lastAction"_s;
constexpr int kPollMs = 400;
constexpr int kWarmStartMs = 1200; ///< RequestRunner's 800 ms pause and a margin
constexpr int kPollGiveUpMs = 120'000;

QVariantMap inputFor(SkillKind kind, const QString& text, const QString& targetLang)
{
    if (kind != SkillKind::Text) {
        return {{u"context"_s, text}};
    }
    QVariantMap in{{u"text"_s, text}};
    if (!targetLang.isEmpty()) {
        in.insert(u"target_lang"_s, targetLang);
    }
    return in;
}

} // namespace

PopupController::PopupController(const Integrations& integrations, client::DaemonClient* client,
                                 QObject* parent)
    : QObject(parent)
    , m_integrations(integrations)
    , m_ownClient(client == nullptr ? std::make_unique<client::DaemonClient>() : nullptr)
    , m_client(client == nullptr ? *m_ownClient : *client)
    , m_detector(util::LanguageData::builtIn())
    , m_runner(new RequestRunner(m_client, this))
    , m_window(std::make_unique<PopupWindow>(m_detector))
{
    connectWindow();
    m_poll.setInterval(kPollMs);
    connect(&m_poll, &QTimer::timeout, this, &PopupController::pollWork);
    connect(m_runner, &RequestRunner::token, this, [this](const QString& piece) {
        m_streamed += piece;
        if (m_catalog.kindOf(m_skill) == SkillKind::Text) {
            m_window->resultPage()->firstWord();
            m_window->resultPage()->textView()->stream(m_streamed);
        }
    });
    connect(m_runner, &RequestRunner::retrying, this, [this] {
        m_streamed.clear();
        m_window->resultPage()->retrying();
    });
    connect(m_runner, &RequestRunner::finished, this, &PopupController::onFinished);
    // An action saved or deleted: read the skills again on the next open.
    connect(&m_client, &client::DaemonClient::skillsChanged, this, [this] { m_catalog = {}; });
    connect(m_runner, &RequestRunner::failed, this, &PopupController::onFailed);
}

PopupController::~PopupController() = default;

void PopupController::connectWindow()
{
    const PopupActionsPage* actions = m_window->actionsPage();
    const PopupResultPage* result = m_window->resultPage();
    connect(actions->actionBar(), &ActionBar::chosen, this, &PopupController::run);
    connect(actions->actionBar(), &ActionBar::replyChosen, this, [this] { run(u"reply"_s, {}); });
    connect(actions->actionBar(), &ActionBar::newActionRequested, this, [this] {
        m_window->close();
        Q_EMIT newActionRequested();
    });
    actions->actionBar()->setTextLanguage([this] { return m_detector.detect(m_text.text); });
    connect(actions, &PopupActionsPage::openInWindow, this, [this] {
        m_window->close();
        Q_EMIT openInWindow(m_text.text);
    });
    connect(result, &PopupResultPage::insertClicked, this, &PopupController::insert);
    connect(result, &PopupResultPage::copyClicked, this, &PopupController::copy);
    connect(result, &PopupResultPage::presetChosen, this,
            [this](const QString& id) { run(id, m_targetLang); });
    connect(result, &PopupResultPage::languageChosen, this,
            [this](const QString& code) { run(m_skill, code); });
    connect(result, &PopupResultPage::languageCleared, this, [this] { run(m_skill, {}); });
    connect(result, &PopupResultPage::againClicked, this, [this] {
        if (m_runner->busy()) {
            m_runner->cancel(); // Stop keeps what is there
        } else {
            m_variety = true; // Retry or "3 new ones": another take that differs
            run(m_skill, m_targetLang);
        }
    });
    connect(m_window.get(), &PopupWindow::closed, m_runner, &RequestRunner::cancel);
    connect(m_window.get(), &PopupWindow::closed, &m_poll, &QTimer::stop);
}

bool PopupController::ensureCatalog()
{
    if (!m_catalog.isEmpty()) {
        return true;
    }
    if (!m_client.connectToDaemon().isEmpty()) {
        return false;
    }
    const QDBusMessage reply = m_client.call(u"ListSkills"_s, {});
    m_catalog = SkillCatalog::fromJson(reply.arguments().value(0).toString());
    UserActionsClient actions(m_client);
    if (actions.load()) {
        m_catalog.setUserActionsBase(actions.templateSkill());
    }
    m_window->actionsPage()->actionBar()->setHiddenBases({u"compose"_s}); // Write starts from nothing
    m_window->actionsPage()->actionBar()->setCatalog(&m_catalog);
    m_window->applyTheme();
    return !m_catalog.isEmpty();
}

void PopupController::show()
{
    const auto captured = m_integrations.capture().capture();
    showText(captured.value_or(CapturedText{}));
    m_window->openNear(QCursor::pos());
}

void PopupController::showText(const CapturedText& text)
{
    m_text = text;
    PopupActionsPage* page = m_window->actionsPage();
    page->showText(text.text, appDisplayName(text.appName), text.origin);
    m_window->showActions();
    if (!ensureCatalog()) {
        page->setStatus(tr("Paraphrase is not running. Open it from the menu and try again."));
        return;
    }
    if (text.text.trimmed().isEmpty()) {
        page->setStatus(tr("Select or copy some text first, then open Paraphrase again."));
        return;
    }
    readAhead();
}

QString PopupController::likelyBase() const
{
    // The action used last, so the popup can work ahead (owner, 2026-10-23: rather than the
    // one used most, which kept Reply on top for text of the user's own).
    const QString last = QSettings().value(kLastKey).toString();
    return m_catalog.actionBases().contains(last) ? last : u"proofread"_s;
}

void PopupController::readAhead()
{
    const QString base = likelyBase();
    const auto variants = m_catalog.variants(base);
    m_window->actionsPage()->actionBar()->highlight(base);
    if (variants.isEmpty() || m_catalog.picksLanguage(base)) {
        m_window->actionsPage()->setStatus(tr("Pick an action."));
        return;
    }
    const SkillInfo* skill = variants.front();
    m_runner->warmLater(skill->id, inputFor(m_catalog.kindOf(skill->id), m_text.text, {}));
    m_likelyTitle = m_catalog.actionTitle(base);
    m_window->actionsPage()->showWorking(tr("Reading the text for %1").arg(m_likelyTitle));
    m_pollClock.start();
    m_poll.start();
}

void PopupController::pollWork()
{
    // The read-ahead starts after a short pause (RequestRunner::kWarmDelayMs) and says nothing
    // when it ends, so the daemon's status tells: no model yet, busy reading, or ready.
    const QJsonObject status =
        QJsonDocument::fromJson(m_client.call(u"Status"_s, {}).arguments().value(0).toString().toUtf8())
            .object();
    PopupActionsPage* page = m_window->actionsPage();
    const bool loading = status.value(u"model"_s).toString().isEmpty();
    const bool busy = status.value(u"busy"_s).toBool();
    if (m_pollClock.elapsed() > kPollGiveUpMs ||
        (!loading && !busy && m_pollClock.elapsed() > kWarmStartMs)) {
        m_poll.stop();
        page->setStatus(tr("Ready for %1").arg(m_likelyTitle));
        return;
    }
    page->showWorking(loading ? tr("Loading the model") : tr("Reading the text for %1").arg(m_likelyTitle));
}

void PopupController::run(const QString& skillId, const QString& targetLang)
{
    const SkillInfo* skill = m_catalog.find(skillId);
    if (skill == nullptr || m_text.text.trimmed().isEmpty()) {
        return;
    }
    m_runner->cancel();
    m_poll.stop();
    m_skill = skillId;
    m_targetLang = targetLang;
    m_streamed.clear();
    const SkillKind kind = m_catalog.kindOf(skillId);
    QSettings().setValue(kLastKey, skill->base);
    PopupResultPage* page = m_window->resultPage();
    const QString title = kind == SkillKind::Text ? m_catalog.actionTitle(skill->base) : tr("Reply");
    // Proofread shows the diff of the text itself (popup-proofread.html); the others show it too.
    page->begin(kind, title, m_text.text, appDisplayName(m_text.appName), skill->base != u"proofread"_s);
    page->setPresets(m_catalog.variants(skill->base), skillId);
    // A tone keeps the text's language unless the user picks another (owner, 2026-10-22).
    page->offerOtherLanguages(skill->takes(u"target_lang"_s) && !m_catalog.picksLanguage(skill->base),
                              m_detector.detect(m_text.text));
    page->setLanguagePill(kind == SkillKind::Text ? QString()
                                                  : languageName(m_detector, m_detector.detect(m_text.text)));
    // A language the user chose stays for the next tone; the pill says so and undoes it.
    const QString own = m_detector.detect(m_text.text);
    const bool other = !targetLang.isEmpty() && targetLang != own;
    page->setChosenLanguage(other ? nativeLanguageName(targetLang) : QString(),
                            other ? languageName(m_detector, targetLang) : QString(),
                            languageName(m_detector, own));
    m_window->showResult();
    m_lastVariety = std::exchange(m_variety, false);
    if (const QString problem =
            m_runner->start(skillId, inputFor(kind, m_text.text, targetLang), m_lastVariety);
        !problem.isEmpty()) {
        page->setInfo(problem, u"danger"_s);
        return;
    }
    page->setRunning(true);
    m_window->fitContent(); // as tall as the shape of the answer, not the last result
}

void PopupController::onFinished(const QJsonObject& result)
{
    PopupResultPage* page = m_window->resultPage();
    page->setRunning(false);
    const SkillInfo* skill = m_catalog.find(m_skill);
    const SkillKind kind = m_catalog.kindOf(m_skill);
    const QString output = result.value(u"output"_s).toString();
    QStringList replies;
    if (kind == SkillKind::QuickReply) {
        for (const auto& v : result.value(u"replies"_s).toArray()) {
            replies.append(v.toString());
        }
        page->repliesView()->setReplies(replies);
    } else if (skill->base == u"proofread"_s) {
        page->textView()->showProofread(m_text.text, output);
    } else {
        page->textView()->showText(output);
    }
    const bool chosen = !m_targetLang.isEmpty();
    const bool sameLanguage = !chosen || m_targetLang == m_detector.detect(m_text.text);
    const FactsCheck check =
        keepsFacts(skill->base) ? checkFacts(m_text.text, page->result(), sameLanguage) : FactsCheck{};
    const QString language = languageName(m_detector, chosen ? m_targetLang : m_detector.detect(m_text.text));
    const ResultFacts facts{
        .kind = kind,
        .proofread = page->textView()->hasDiff() && kind == SkillKind::Text,
        .changes = page->textView()->changeCount(),
        .language = language,
        .chosenLanguage = chosen,
        .replyKind = {},
        .facts = factsLine(check),
    };
    // popup-proofread.html: the line starts the row ("2 changes · kept in English"), no title before it.
    QString summary = resultSummary(facts);
    if (summary.startsWith(u"· "_s)) {
        summary.remove(0, 2);
    }
    if (skill->base == u"rewrite"_s) {
        summary = summaryLine(*skill, output, !sameLanguage);
    }
    page->setInfo(summary, check.missing.isEmpty() ? u"small"_s : u"warning"_s);
    if (facts.proofread && facts.changes == 0) {
        page->showNothingToFix();
    }
    if (!result.value(u"warning"_s).toString().isEmpty()) {
        page->setInfo(languageWarning(language), u"warning"_s);
    }
    m_window->fitContent();
    keepInHistory(m_lastVariety, replies.isEmpty() ? page->result() : replies.join(u"\n\n"_s));
    Q_EMIT finished();
}

void PopupController::keepInHistory(bool sameRun, const QString& result)
{
    const SkillInfo* skill = m_catalog.find(m_skill);
    // Text from an app the user left out is never kept (settings-privacy.html note 4).
    if (m_history == nullptr || skill == nullptr || history::Keeper::isExcluded(m_text.appName) ||
        history::Keeper::isExcluded(appDisplayName(m_text.appName))) {
        return;
    }
    if (sameRun && m_historyId != 0 && m_history->update(m_historyId, result)) {
        return;
    }
    const bool chosen = !m_targetLang.isEmpty() && m_targetLang != m_detector.detect(m_text.text);
    const PopupResultPage* page = m_window->resultPage();
    const bool proofread = skill->base == u"proofread"_s && page->textView()->hasDiff();
    history::Entry entry;
    entry.skillId = m_skill;
    entry.label = historyLabel(*skill, chosen ? languageName(m_detector, m_targetLang) : QString(),
                               proofread ? static_cast<int>(page->textView()->changeCount()) : -1);
    entry.app = appDisplayName(m_text.appName);
    entry.text = m_text.text;
    entry.result = result;
    QJsonObject options;
    if (!m_targetLang.isEmpty()) {
        options.insert(u"target_lang"_s, m_targetLang);
    }
    entry.options = QJsonDocument(options).toJson(QJsonDocument::Compact);
    m_historyId = m_history->record(entry);
}

QString PopupController::summaryLine(const SkillInfo& skill, const QString& output, bool otherLanguage) const
{
    // The tone, the facts kept, and for Concise and Elaborate how the length moved; a language
    // the user chose is the pill's, and a slip of language shows as a warning instead.
    QStringList parts{SkillCatalog::presetTitle(skill)};
    if (const QString facts = factsLine(checkFacts(m_text.text, output, !otherLanguage)); !facts.isEmpty()) {
        parts.append(facts);
    }
    const QString preset = skill.id.section(u'.', 1);
    if (preset == u"concise"_s || preset == u"elaborate"_s) {
        static const QRegularExpression kSpace(u"\\s+"_s);
        parts.append(tr("%1 to %2 words")
                         .arg(m_text.text.split(kSpace, Qt::SkipEmptyParts).size())
                         .arg(output.split(kSpace, Qt::SkipEmptyParts).size()));
    }
    return parts.join(u" · "_s);
}

void PopupController::onFailed(const QString& code, const QString& message)
{
    PopupResultPage* page = m_window->resultPage();
    page->setRunning(false);
    page->setInfo(code == u"cancelled"_s ? tr("· stopped") : message,
                  code == u"cancelled"_s ? u"small"_s : u"danger"_s);
    Q_EMIT finished();
}

void PopupController::insert()
{
    // Only after the user's click (Replace, Insert reply); never sends anything.
    const QString text = m_window->resultPage()->result();
    if (text.isEmpty()) {
        return;
    }
    // Out of the way first: the text goes back only once its own window has the focus again
    // (PasteInsert waits for it), and the popup on top would keep it.
    const QPoint at = m_window->pos();
    m_window->hide();
    if (m_history != nullptr && m_historyId != 0) {
        m_history->update(m_historyId, text); // the version put back, edited or not
    }
    const InsertOutcome outcome = m_integrations.insert().insert(text, m_text);
    if (outcome == InsertOutcome::Typed) {
        m_window->close();
        return;
    }
    m_window->openNear(at - QPoint(PopupWindow::kOffset, PopupWindow::kOffset)); // to say where the text is
    const QString where = m_text.appName.isEmpty() ? tr("the app") : appDisplayName(m_text.appName);
    m_window->resultPage()->setInfo(outcome == InsertOutcome::OnClipboard
                                        ? tr("· on the clipboard: press Ctrl+V in %1").arg(where)
                                        : tr("· could not put it back; use Copy"),
                                    u"warning"_s);
}

void PopupController::copy()
{
    const QString text = m_window->resultPage()->result();
    if (!text.isEmpty()) {
        QGuiApplication::clipboard()->setText(text);
        if (m_history != nullptr && m_historyId != 0) {
            m_history->update(m_historyId, text);
        }
        m_window->resultPage()->setInfo(tr("· copied"));
    }
}

} // namespace paraphrase::app

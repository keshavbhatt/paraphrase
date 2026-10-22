#include "popup/popup_controller.h"

#include "integration/integrations.h"
#include "popup/popup_pages.h"
#include "popup/popup_window.h"
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
#include <QSettings>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

const QString kUsesKey = u"popup/uses/"_s;

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
    connect(m_runner, &RequestRunner::token, this, [this](const QString& piece) {
        m_streamed += piece;
        if (m_catalog.kindOf(m_skill) == SkillKind::Text) {
            m_window->resultPage()->textView()->stream(m_streamed);
        }
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
    connect(result, &PopupResultPage::againClicked, this, [this] {
        if (m_runner->busy()) {
            m_runner->cancel(); // Stop keeps what is there
        } else {
            run(m_skill, m_targetLang);
        }
    });
    connect(m_window.get(), &PopupWindow::closed, m_runner, &RequestRunner::cancel);
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
    page->showText(text.text, text.appName, text.origin);
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
    // The action the user picks most often, so the popup can work ahead (popup-actions.html).
    const QSettings settings;
    QString best = u"proofread"_s;
    int most = -1;
    for (const QString& base : m_catalog.actionBases()) {
        const int uses = settings.value(kUsesKey + base, 0).toInt();
        if (uses > most) {
            most = uses;
            best = base;
        }
    }
    return best;
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
    m_window->actionsPage()->setStatus(tr("Reading the text for %1...").arg(m_catalog.actionTitle(base)));
}

void PopupController::run(const QString& skillId, const QString& targetLang)
{
    const SkillInfo* skill = m_catalog.find(skillId);
    if (skill == nullptr || m_text.text.trimmed().isEmpty()) {
        return;
    }
    m_runner->cancel();
    m_skill = skillId;
    m_targetLang = targetLang;
    m_streamed.clear();
    const SkillKind kind = m_catalog.kindOf(skillId);
    QSettings settings;
    settings.setValue(kUsesKey + skill->base, settings.value(kUsesKey + skill->base, 0).toInt() + 1);
    PopupResultPage* page = m_window->resultPage();
    const QString title = kind == SkillKind::Text ? m_catalog.actionTitle(skill->base) : tr("Reply");
    // Proofread shows the diff of the text itself (popup-proofread.html); the others show it too.
    page->begin(kind, title, m_text.text, m_text.appName, skill->base != u"proofread"_s);
    page->setPresets(m_catalog.variants(skill->base), skillId);
    page->setLanguagePill(kind == SkillKind::Text ? QString()
                                                  : languageName(m_detector, m_detector.detect(m_text.text)));
    m_window->showResult();
    if (const QString problem = m_runner->start(skillId, inputFor(kind, m_text.text, targetLang));
        !problem.isEmpty()) {
        page->setInfo(problem, u"danger"_s);
        return;
    }
    page->setRunning(true);
}

void PopupController::onFinished(const QJsonObject& result)
{
    PopupResultPage* page = m_window->resultPage();
    page->setRunning(false);
    const SkillInfo* skill = m_catalog.find(m_skill);
    const SkillKind kind = m_catalog.kindOf(m_skill);
    const QString output = result.value(u"output"_s).toString();
    if (kind == SkillKind::QuickReply) {
        QStringList replies;
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
    const FactsCheck check = keepsFacts(skill->base) ? checkFacts(m_text.text, page->result()) : FactsCheck{};
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
    page->setInfo(resultSummary(facts), check.missing.isEmpty() ? u"small"_s : u"warning"_s);
    if (facts.proofread && facts.changes == 0) {
        page->showNothingToFix();
    }
    if (!result.value(u"warning"_s).toString().isEmpty()) {
        page->setInfo(languageWarning(language), u"warning"_s);
    }
    m_window->adjustSize();
    Q_EMIT finished();
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
    const InsertOutcome outcome = m_integrations.insert().insert(text, m_text);
    if (outcome == InsertOutcome::Typed) {
        m_window->close();
        return;
    }
    const QString where = m_text.appName.isEmpty() ? tr("the app") : m_text.appName;
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
        m_window->resultPage()->setInfo(tr("· copied"));
    }
}

} // namespace paraphrase::app

#pragma once

#include "daemon_client.h"
#include "integration/backends.h"
#include "ui/core/skill_catalog.h"
#include "util/language_detector.h"

#include <QElapsedTimer>
#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QTimer>

#include <memory>

namespace paraphrase::app {

class Integrations;
class PopupWindow;
class RequestRunner;

/// The popup flow (PLAN 3.2): capture the text the user means, open the popup with the
/// actions, read ahead for the likely one, run the chosen one, and put the result back only
/// when the user clicks Replace or Insert reply. Nothing is typed or sent otherwise
/// (CLAUDE.md), and a result goes to the clipboard when typing is not possible.
class PopupController : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(PopupController)

public:
    /// Uses `client` when given (one connection for the whole app), else its own.
    explicit PopupController(const Integrations& integrations, client::DaemonClient* client = nullptr,
                             QObject* parent = nullptr);
    ~PopupController() override;

    /// Captures the text now and opens the popup near the cursor.
    void show();
    /// Opens the popup on this text (tests, and the main window's hand-over in the other way).
    void showText(const CapturedText& text);
    [[nodiscard]] PopupWindow* window() const { return m_window.get(); }

Q_SIGNALS:
    /// "Open in window": the main window takes the text (ADR-013).
    void openInWindow(const QString& text);
    void finished();           ///< a run ended, with a result or a message (tests)
    void newActionRequested(); ///< "New action..." under My actions

private:
    [[nodiscard]] bool ensureCatalog();
    void connectWindow();
    void readAhead();
    void run(const QString& skillId, const QString& targetLang);
    void onFinished(const QJsonObject& result);
    void onFailed(const QString& code, const QString& message);
    void insert();
    void copy();
    [[nodiscard]] QString likelyBase() const;
    /// While it reads ahead: the footer's working pill follows the daemon's status.
    void pollWork();
    /// "Elaborate · kept: Friday · 21 to 34 words": what the run changed (popup-working.html, 4).
    [[nodiscard]] QString summaryLine(const SkillInfo& skill, const QString& output,
                                      bool otherLanguage) const;

    const Integrations& m_integrations;
    std::unique_ptr<client::DaemonClient> m_ownClient;
    client::DaemonClient& m_client;
    util::LanguageDetector m_detector;
    SkillCatalog m_catalog;
    RequestRunner* m_runner = nullptr;
    std::unique_ptr<PopupWindow> m_window;
    CapturedText m_text;
    QString m_skill;
    QString m_targetLang;
    QString m_streamed;
    QTimer m_poll;
    QElapsedTimer m_pollClock;
    QString m_likelyTitle;
    bool m_variety = false; ///< the next run is another version (RunSkill's variety option)
};

} // namespace paraphrase::app

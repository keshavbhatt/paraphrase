#pragma once

#include "daemon_client.h"
#include "ui/common/theme.h"
#include "ui/core/skill_catalog.h"
#include "util/language_detector.h"

#include <QJsonObject>
#include <QString>
#include <QWidget>

#include <memory>

class QSplitter;

namespace paraphrase::app {

class ActionBar;
class HeaderBar;
class InputPane;
class NoticeBar;
class OutputPane;
class RequestRunner;
class StatusBar;

/// The standalone window (ADR-013, mocks main-window*.html): paste or type text, pick an
/// action, see the result stream in, copy it. It needs nothing from the desktop but a window
/// and the clipboard, and never types into another app.
///
/// It only composes the parts and routes between them: the action bar reports what the user
/// picked, the input pane gives the request's inputs, the runner talks to the daemon, the
/// output pane shows the result. The actions come from the daemon's skills, so a new skill
/// appears without a code change.
class MainWindow : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(MainWindow)

public:
    /// Uses `client` when given (one connection for the whole app), else its own.
    explicit MainWindow(client::DaemonClient* client = nullptr, QWidget* parent = nullptr);
    ~MainWindow() override;

    /// Connects to the daemon (starting it when needed) and loads the actions.
    void start();
    /// Text handed over from the popup ("Open in window"): it becomes your text.
    void takeText(const QString& text);
    /// With a global shortcut there is nothing to say; without one the notice explains how
    /// to add one by hand (ADR-013).
    void setGlobalShortcut(bool available);

    // What the window shows, for tests and screenshots.
    [[nodiscard]] QString resultText() const;
    [[nodiscard]] QString currentSkill() const { return m_skill; }
    [[nodiscard]] bool busy() const;

public Q_SLOTS:
    void selectSkill(const QString& skillId);
    void run();
    void cancel();
    void copyResult();

Q_SIGNALS:
    void actionsLoaded();
    void finished();              ///< a request ended, with a result or a message
    void settingsRequested();     ///< the header's Settings, or the notice's link
    void newActionRequested();    ///< "New action..." under My actions
    void shortcutHelpRequested(); ///< the notice's "Add a shortcut by hand"

protected:
    void changeEvent(QEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void buildUi();
    void connectParts();
    void applyTheme();
    void loadSkills();
    void loadStatus();
    void chooseReply(bool full);
    void onSecondary();
    void onRetry();
    /// Runs a Tone preset on the result: "Make it polite", "Shorter".
    void rewriteResult(const QString& skillId);
    void onToken(const QString& piece);
    void onFinished(const QJsonObject& result);
    void onFailed(const QString& code, const QString& message);
    void showResult(const QJsonObject& result);
    [[nodiscard]] QString inputLanguageName() const;

    std::unique_ptr<client::DaemonClient> m_ownClient;
    client::DaemonClient& m_client;
    util::LanguageDetector m_detector;
    Tokens m_tokens;
    SkillCatalog m_catalog;
    RequestRunner* m_runner = nullptr;

    HeaderBar* m_header = nullptr;
    NoticeBar* m_notice = nullptr;
    ActionBar* m_actions = nullptr;
    InputPane* m_input = nullptr;
    OutputPane* m_output = nullptr;
    StatusBar* m_status = nullptr;
    QSplitter* m_panes = nullptr;

    QString m_skill;        ///< the selected action
    QString m_targetLang;   ///< the language picked in a language menu
    QString m_requestInput; ///< the text the running or last request worked on
    QString m_streamed;     ///< text streamed so far
    QString m_fullReplyKey; ///< message and intent of the last full reply, to keep its versions
    bool m_fullReply = false;
    bool m_rewriting = false; ///< a Tone preset runs on the result
};

} // namespace paraphrase::app

#pragma once

#include "daemon_client.h"
#include "theme.h"
#include "util/language_detector.h"
#include "word_diff.h"

#include <QJsonObject>
#include <QList>
#include <QSet>
#include <QString>
#include <QTimer>
#include <QWidget>

#include <optional>

class QAbstractButton;
class QButtonGroup;
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QSplitter;
class QStackedWidget;
class QTextBrowser;
class QToolButton;
class QVBoxLayout;

namespace paraphrase::app {

/// A skill as the daemon lists it (ListSkills, DOCS/api.md).
struct SkillInfo
{
    QString id;
    QString base;
    QString title;
    QString output; ///< text | json
};

/// The standalone window (ADR-013, mocks main-window.html and main-window-reply.html): paste
/// or type text, pick an action, see the result stream in, copy it. It needs nothing from the
/// desktop but a window and the clipboard, and never types into another app.
///
/// The actions come from the daemon's skills, so a new skill appears without a code change:
/// Proofread shows the word diff with Undo per change (PLAN D6); the rewrite presets sit in a
/// Tone menu; Reply takes the message and an optional draft and shows three replies.
class MainWindow : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(MainWindow)

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    /// Connects to the daemon (starting it when needed) and loads the actions.
    void start();

    // What the window shows, for tests and screenshots.
    [[nodiscard]] QString resultText() const;
    [[nodiscard]] QString currentSkill() const { return m_skill; }
    [[nodiscard]] bool busy() const { return !m_requestId.isEmpty(); }

public Q_SLOTS:
    void selectSkill(const QString& skillId);
    void run();
    void cancel();
    void copyResult();

Q_SIGNALS:
    void actionsLoaded();
    void finished(); ///< a request ended, with a result or a message

protected:
    void resizeEvent(QResizeEvent* event) override;
    void changeEvent(QEvent* event) override;

private:
    enum class Mode : std::uint8_t
    {
        Text,  ///< proofread, rewrite and other text skills
        Reply, ///< message and draft in, three replies out
    };

    void buildUi();
    void buildHeader(QVBoxLayout* main);
    [[nodiscard]] QWidget* buildInputPane();
    [[nodiscard]] QWidget* buildOutputPane();
    [[nodiscard]] QWidget* buildReplyOutput();
    [[nodiscard]] QHBoxLayout* buildFooter();
    [[nodiscard]] QWidget* buildStatusBar();
    void buildActions(const QList<SkillInfo>& skills);
    [[nodiscard]] QAbstractButton* actionButton(const QString& hint, const SkillInfo& s);
    [[nodiscard]] QAbstractButton* presetButton(const QString& hint, const QString& base,
                                                const QList<const SkillInfo*>& variants);
    void applyTheme();
    void loadStatus();
    void showMessage(const QString& message, const QString& role = QStringLiteral("muted"));
    void updateInputInfo();
    void warmLater();

    void onToken(const QString& id, int variant, const QString& text);
    void onRetrying(const QString& id, const QString& reason);
    void onDone(const QString& id, const QString& result);
    void onError(const QString& id, const QString& code, const QString& message);

    void showProofread(const QString& before, const QString& after);
    void renderDiff();
    [[nodiscard]] QString diffHtml(const client::WordDiff& diff) const;
    [[nodiscard]] QWidget* changeRow(const client::WordDiff& diff, qsizetype k);
    void showReplies(const QStringList& replies);
    void setRunning(bool running);
    [[nodiscard]] Mode modeOf(const QString& skillId) const;
    [[nodiscard]] const SkillInfo* skill(const QString& id) const;
    [[nodiscard]] QString languageName(const QString& text) const;
    [[nodiscard]] QVariantMap input() const;

    client::DaemonClient m_client;
    util::LanguageDetector m_detector;
    Tokens m_tokens;
    QList<SkillInfo> m_skills;
    QString m_skill;        ///< the selected action
    QString m_requestId;    ///< the running request, empty when idle
    QString m_requestInput; ///< the text the running or last request was given
    QString m_streamed;     ///< text streamed so far
    QString m_result;       ///< the last finished result (text skills)
    std::optional<client::WordDiff> m_diff;
    QSet<qsizetype> m_undone;      ///< proofread changes the user undid
    bool m_politeForReply = false; ///< "Make it polite" on the chosen reply is running
    QTimer m_warmTimer;

    // Header, notice, actions.
    QLabel* m_mark = nullptr;
    QLabel* m_lock = nullptr;
    QWidget* m_notice = nullptr;
    QLabel* m_noticeIcon = nullptr;
    QHBoxLayout* m_actionsRow = nullptr;
    QButtonGroup* m_actionGroup = nullptr;
    QToolButton* m_toneButton = nullptr;

    // Left pane.
    QLabel* m_inTitle = nullptr;
    QLabel* m_inInfo = nullptr;
    QStackedWidget* m_inStack = nullptr;
    QPlainTextEdit* m_text = nullptr;
    QPlainTextEdit* m_message = nullptr;
    QPlainTextEdit* m_draft = nullptr;
    QLabel* m_inHint = nullptr;

    // Right pane.
    QLabel* m_outTitle = nullptr;
    QLabel* m_outInfo = nullptr;
    QWidget* m_segment = nullptr;
    QPushButton* m_showChanges = nullptr;
    QPushButton* m_showResult = nullptr;
    QStackedWidget* m_outStack = nullptr;
    QTextBrowser* m_output = nullptr;
    QWidget* m_changes = nullptr;
    QVBoxLayout* m_changesLayout = nullptr;
    QList<QPushButton*> m_replyButtons;
    QList<QPushButton*> m_replyCopy;
    QLineEdit* m_replyEdit = nullptr;
    QPushButton* m_primary = nullptr;
    QPushButton* m_secondary = nullptr;
    QPushButton* m_retry = nullptr;

    QSplitter* m_panes = nullptr;
    QLabel* m_statusDot = nullptr;
    QLabel* m_statusText = nullptr;
};

} // namespace paraphrase::app

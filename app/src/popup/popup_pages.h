#pragma once

#include "ui/common/theme.h"
#include "ui/core/skill_catalog.h"

#include <QWidget>

class QHBoxLayout;
class QLabel;
class QPushButton;
class QStackedWidget;

namespace paraphrase::util {
class LanguageDetector;
}

namespace paraphrase::app {

class ActionBar;
class ChipButton;
class QuickRepliesView;
class SegmentedControl;
class TextResultView;

/// The popup's first page (popup-actions.html): the captured text, the actions numbered 1 to
/// N, and a line saying what Paraphrase is doing. Esc closes from any page.
class PopupActionsPage : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(PopupActionsPage)

public:
    explicit PopupActionsPage(const util::LanguageDetector& detector, QWidget* parent = nullptr);
    ~PopupActionsPage() override = default;

    [[nodiscard]] ActionBar* actionBar() const { return m_actions; }
    /// The text and where it came from: "from Telegram", "· 16 words · English".
    void showText(const QString& text, const QString& appName, const QString& origin);
    void setStatus(const QString& status);
    void applyTheme(const Tokens& t);

Q_SIGNALS:
    void openInWindow();
    void closeRequested();

private:
    [[nodiscard]] QWidget* buildHead();
    [[nodiscard]] QWidget* buildFoot();

    const util::LanguageDetector& m_detector;
    QLabel* m_mark = nullptr;
    QLabel* m_lock = nullptr;
    QLabel* m_from = nullptr;
    QLabel* m_caption = nullptr;
    QLabel* m_info = nullptr;
    QLabel* m_source = nullptr;
    ActionBar* m_actions = nullptr;
    QLabel* m_status = nullptr;
    QPushButton* m_expand = nullptr;
    QPushButton* m_close = nullptr;
};

/// The popup's result page (popup-proofread.html, popup-reply.html, popup-rewrite.html):
/// the result of one action with Replace or Insert reply, Copy and Retry or "3 new ones".
/// Replace and Insert only report the user's click; the popup's controller puts the text back.
class PopupResultPage : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(PopupResultPage)

public:
    explicit PopupResultPage(const util::LanguageDetector& detector, QWidget* parent = nullptr);
    ~PopupResultPage() override = default;

    /// Starts showing a run: its kind, its title, the text it works on, the app to put it back in.
    void begin(SkillKind kind, const QString& title, const QString& source, const QString& appName,
               bool showSource);
    /// The other presets of the running skill ("Professional · Polite ..."), the current one marked.
    void setPresets(const QList<const SkillInfo*>& presets, const QString& currentId);
    void setInfo(const QString& text, const QString& role = QStringLiteral("small"));
    void setRunning(bool running);
    void setLanguagePill(const QString& language);
    /// Proofread found nothing: "Looks good, no changes" and only Close (popup-proofread.html,
    /// note 5), so the user is never asked to replace text with itself.
    void showNothingToFix();
    /// Offers "Write it in another language" (popup-rewrite.html, note 5) for a skill that
    /// takes a target language; `own` is the text's language, left out of the menu.
    void offerOtherLanguages(bool offer, const QString& own);
    [[nodiscard]] TextResultView* textView() const { return m_text; }
    [[nodiscard]] QuickRepliesView* repliesView() const { return m_replies; }
    /// What Replace or Copy uses.
    [[nodiscard]] QString result() const;
    void applyTheme(const Tokens& t);

Q_SIGNALS:
    void back();
    void closeRequested();
    void insertClicked();
    void copyClicked();
    void againClicked();
    void presetChosen(const QString& skillId);
    void languageChosen(const QString& code);

private:
    [[nodiscard]] QWidget* buildHead();
    [[nodiscard]] QWidget* buildBody();
    [[nodiscard]] QWidget* buildFoot();
    [[nodiscard]] QPushButton* buildLanguageLink();
    void updateButtons();

    QLabel* m_title = nullptr;
    QLabel* m_lock = nullptr;
    QPushButton* m_back = nullptr;
    QPushButton* m_close = nullptr;
    QWidget* m_presets = nullptr;
    QLabel* m_caption = nullptr;
    QLabel* m_pill = nullptr;
    QLabel* m_source = nullptr;
    QLabel* m_info = nullptr;
    QLabel* m_pick = nullptr;
    SegmentedControl* m_view = nullptr;
    QStackedWidget* m_results = nullptr;
    TextResultView* m_text = nullptr;
    QuickRepliesView* m_replies = nullptr;
    ChipButton* m_insert = nullptr;
    QPushButton* m_copy = nullptr;
    QPushButton* m_again = nullptr;
    QPushButton* m_done = nullptr;
    QPushButton* m_otherLanguage = nullptr;
    const util::LanguageDetector& m_detector;
    QString m_ownLanguage;
    bool m_nothingToFix = false;
    SkillKind m_kind = SkillKind::Text;
    QString m_appName;
    bool m_running = false;
    Tokens m_tokens;
};

} // namespace paraphrase::app

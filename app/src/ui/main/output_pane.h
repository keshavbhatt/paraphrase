#pragma once

#include "ui/common/theme.h"
#include "ui/common/widgets.h"
#include "ui/core/skill_catalog.h"

class QLabel;
class QPushButton;
class QStackedWidget;

namespace paraphrase::app {

class BusyPill;
class ChipButton;
class FullReplyView;
class SkeletonView;
class QuickRepliesView;
class TextResultView;

/// The right pane: the result of an action. It shows the view for the action's kind, a line
/// about the result, Changes or Result for proofread, the versions of a full reply, and the
/// buttons: Copy, then two that depend on the kind ("Use as my text" and Retry for text;
/// "3 new ones" and "Make it polite" for quick replies; "Another version" and "Shorter").
class OutputPane : public Pane
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(OutputPane)

public:
    explicit OutputPane(QWidget* parent = nullptr);
    ~OutputPane() override = default;

    void showKind(SkillKind kind, const QString& title, bool proofread);
    /// The line after the title: "· 4 changes · kept in English"; role muted, warning or danger.
    void setInfo(const QString& text, const QString& role = QStringLiteral("small"));
    [[nodiscard]] QString info() const;
    /// Copy and the other buttons wait while a run goes; Retry or Shorter turns into Stop.
    void setRunning(bool running);

    /// What the run is doing, as far as the app knows (main-window-busy.html).
    enum class Step : std::uint8_t
    {
        Reading,  ///< before the first word: the model reads the text (and loads, the first time)
        Writing,  ///< the words come in
        Retrying, ///< the engine's check failed: written again
    };
    /// A run starts: the working pill counts from zero and, with `placeholder`, the result
    /// gives way to the shape of the answer until its first word. A rewrite of the result on
    /// screen (Shorter, Make it polite) keeps the result instead.
    void startWork(bool placeholder = true);
    void setStep(Step step);
    /// What Copy copies for the current kind.
    [[nodiscard]] QString result() const;

    [[nodiscard]] TextResultView* textView() const { return m_text; }
    [[nodiscard]] QuickRepliesView* repliesView() const { return m_replies; }
    [[nodiscard]] FullReplyView* fullView() const { return m_full; }
    void applyTheme(const Tokens& t);

Q_SIGNALS:
    void copyClicked();
    void secondaryClicked(); ///< Use as my text, 3 new ones, Another version
    void retryClicked();     ///< Retry or Stop, Make it polite, Shorter

private:
    void buildHead();
    void buildFoot();
    void updateButtons();
    void updateVersions();
    void showPage();
    [[nodiscard]] QString stepText(Step step) const;

    QLabel* m_title = nullptr;
    QLabel* m_info = nullptr;
    SegmentedControl* m_view = nullptr;
    QLabel* m_versionInfo = nullptr;
    QPushButton* m_prev = nullptr;
    QPushButton* m_next = nullptr;
    QStackedWidget* m_pages = nullptr;
    TextResultView* m_text = nullptr;
    QuickRepliesView* m_replies = nullptr;
    FullReplyView* m_full = nullptr;
    ChipButton* m_copy = nullptr;
    QPushButton* m_secondary = nullptr;
    QPushButton* m_retry = nullptr;
    BusyPill* m_pill = nullptr;
    SkeletonView* m_skeleton = nullptr;
    SkillKind m_kind = SkillKind::Text;
    bool m_running = false;
    bool m_placeholder = false; ///< the skeleton shows until the first word
    Step m_step = Step::Reading;
    Tokens m_tokens;
};

} // namespace paraphrase::app

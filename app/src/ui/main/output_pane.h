#pragma once

#include "ui/common/theme.h"
#include "ui/common/widgets.h"
#include "ui/core/skill_catalog.h"

class QLabel;
class QPushButton;
class QStackedWidget;

namespace paraphrase::app {

class ChipButton;
class FullReplyView;
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
    void setRunning(bool running);
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
    SkillKind m_kind = SkillKind::Text;
    bool m_running = false;
    Tokens m_tokens;
};

} // namespace paraphrase::app

#pragma once

#include "ui/common/widgets.h"
#include "ui/core/skill_catalog.h"

#include <QHash>
#include <QIcon>

class QLabel;
class QPushButton;
class QStackedWidget;

namespace paraphrase::util {
class LanguageDetector;
}

namespace paraphrase::app {

class ChipButton;
class ComposeInput;
class FullReplyInput;
class InputPage;

/// The left pane: what the user gives the action. It shows the page for the action's kind,
/// the words and the language of its text, Paste and Clear, and for replies the switch
/// between quick replies and a full reply. Switching pages carries the text along.
class InputPane : public Pane
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(InputPane)

public:
    explicit InputPane(const util::LanguageDetector& detector, QWidget* parent = nullptr);
    ~InputPane() override = default;

    void showKind(SkillKind kind);
    [[nodiscard]] InputPage* page() const;
    [[nodiscard]] FullReplyInput* fullReplyInput() const { return m_full; }
    [[nodiscard]] ComposeInput* composeInput() const { return m_compose; }
    /// The language code of the text on the current page.
    [[nodiscard]] QString language() const;

    void setFullReplyAvailable(bool available);
    void setFullReply(bool full);

    /// The run button (main-window-run.html): named after what it does, with that action's
    /// icon in both colours, filled while no result for this text is showing.
    void setRunAction(const QString& label, const QIcon& onFill, const QIcon& plain);
    void setRunState(bool primary, bool busy);
    /// Nothing to run yet: the field the action needs is empty.
    [[nodiscard]] bool isEmpty() const;
    /// The cursor in the page's first field, ready to type or paste.
    void focusField() const;

Q_SIGNALS:
    void edited();
    void fullReplyToggled(bool full);
    void runClicked();

private:
    void buildHead();
    void updateInfo();
    void updateHint();
    void updateRun();

    const util::LanguageDetector& m_detector;
    QLabel* m_title = nullptr;
    QLabel* m_info = nullptr;
    QLabel* m_pill = nullptr;
    SegmentedControl* m_replyKind = nullptr;
    QPushButton* m_clear = nullptr;
    QPushButton* m_paste = nullptr;
    QStackedWidget* m_pages = nullptr;
    QHash<SkillKind, InputPage*> m_byKind;
    FullReplyInput* m_full = nullptr;
    ComposeInput* m_compose = nullptr;
    QLabel* m_hint = nullptr;
    ChipButton* m_run = nullptr;
    QString m_runLabel;
    QIcon m_runIcon;
    QIcon m_runPlainIcon;
    bool m_runPrimary = true;
    bool m_busy = false;
    SkillKind m_kind = SkillKind::Text;
    bool m_fullAvailable = false;
};

} // namespace paraphrase::app

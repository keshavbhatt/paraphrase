#pragma once

#include "skill_catalog.h"
#include "widgets.h"

#include <QHash>

class QLabel;
class QPushButton;
class QStackedWidget;

namespace paraphrase::util {
class LanguageDetector;
}

namespace paraphrase::app {

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
    /// The language code of the text on the current page.
    [[nodiscard]] QString language() const;

    void setFullReplyAvailable(bool available);
    void setFullReply(bool full);

Q_SIGNALS:
    void edited();
    void fullReplyToggled(bool full);

private:
    void buildHead();
    void updateInfo();

    const util::LanguageDetector& m_detector;
    QLabel* m_title = nullptr;
    QLabel* m_info = nullptr;
    QLabel* m_pill = nullptr;
    SegmentedControl* m_replyKind = nullptr;
    QPushButton* m_clear = nullptr;
    QStackedWidget* m_pages = nullptr;
    QHash<SkillKind, InputPage*> m_byKind;
    FullReplyInput* m_full = nullptr;
    SkillKind m_kind = SkillKind::Text;
    bool m_fullAvailable = false;
};

} // namespace paraphrase::app

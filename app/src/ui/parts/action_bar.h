#pragma once

#include "ui/common/theme.h"
#include "ui/core/skill_catalog.h"

#include <QHash>
#include <QWidget>

#include <cstdint>
#include <functional>

class QLayout;
class QMenu;

namespace paraphrase::util {
class LanguageDetector;
}

namespace paraphrase::app {

class ChipButton;

/// The row of actions (main-window.html, note 2): one button per skill with "Ctrl N"; a skill
/// with presets opens a menu of them, a skill that writes in a chosen language opens a menu of
/// languages, Reply opens the kind of reply used last. It only reports what the user picked.
class ActionBar : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(ActionBar)

public:
    /// How the actions are numbered and laid out: "Ctrl 1" in one row (the main window), or
    /// "1" wrapping onto more rows (the popup, where plain number keys pick an action).
    enum class Keys : std::uint8_t
    {
        Ctrl,
        Digits,
    };

    explicit ActionBar(const util::LanguageDetector& detector, Keys keys = Keys::Ctrl,
                       QWidget* parent = nullptr);
    ~ActionBar() override = default;

    void setCatalog(const SkillCatalog* catalog);
    /// Shows `skill` as the current action, naming its preset or language: "Tone: Polite".
    void setCurrent(const SkillInfo& skill, const QString& targetLang);
    /// The language of the text now, left out of the language menu.
    void setTextLanguage(std::function<QString()> currentLanguage);
    /// Marks an action without running it: the popup's likely action (popup-actions.html).
    void highlight(const QString& base);
    void applyTheme(const Tokens& t);

Q_SIGNALS:
    void chosen(const QString& skillId, const QString& targetLang);
    void replyChosen();

private:
    [[nodiscard]] ChipButton* addAction(int number, const QString& base);
    [[nodiscard]] QMenu* presetMenu(ChipButton* button, const QString& base);
    [[nodiscard]] QMenu* languageMenu(ChipButton* button, const QString& skillId);
    void fillLanguages(QMenu* menu, const QString& skillId);
    void updateArrows();

    const util::LanguageDetector& m_detector;
    const SkillCatalog* m_catalog = nullptr;
    Keys m_keys;
    QLayout* m_row = nullptr;
    QHash<QString, ChipButton*> m_buttons; ///< base -> button
    std::function<QString()> m_textLanguage;
    Tokens m_tokens;
};

} // namespace paraphrase::app

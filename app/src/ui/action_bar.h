#pragma once

#include "skill_catalog.h"
#include "theme.h"

#include <QHash>
#include <QWidget>

#include <functional>

class QHBoxLayout;
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
    ActionBar(const util::LanguageDetector& detector, QWidget* parent = nullptr);
    ~ActionBar() override = default;

    void setCatalog(const SkillCatalog* catalog);
    /// Shows `skill` as the current action, naming its preset or language: "Tone: Polite".
    void setCurrent(const SkillInfo& skill, const QString& targetLang);
    /// The language of the text now, left out of the language menu.
    void setTextLanguage(std::function<QString()> currentLanguage);
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
    QHBoxLayout* m_row = nullptr;
    QHash<QString, ChipButton*> m_buttons; ///< base -> button
    std::function<QString()> m_textLanguage;
    Tokens m_tokens;
};

} // namespace paraphrase::app

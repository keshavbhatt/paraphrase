#pragma once

#include "ui/common/theme.h"

#include <QWidget>

namespace paraphrase::app {

class Preferences;
class SegmentedControl;
class ToggleSwitch;

/// Settings, General (settings-general-shortcut.html, note 1): start at login, keep running in
/// the tray, appearance.
class GeneralPage : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(GeneralPage)

public:
    GeneralPage(Preferences& preferences, bool hasTray, QWidget* parent = nullptr);
    ~GeneralPage() override = default;

    /// Shows the current preferences (the page is opened).
    void reload();
    void applyTheme(const Tokens& t);

private:
    [[nodiscard]] static QWidget* option(const QString& title, const QString& detail, QWidget* control);

    Preferences& m_preferences;
    ToggleSwitch* m_login = nullptr;
    ToggleSwitch* m_tray = nullptr;
    SegmentedControl* m_appearance = nullptr;
};

} // namespace paraphrase::app

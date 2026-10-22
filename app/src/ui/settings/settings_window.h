#pragma once

#include "ui/common/theme.h"
#include "ui/core/desktop_support.h"

#include <QWidget>

#include <memory>

class QStackedWidget;

namespace paraphrase::client {
class DaemonClient;
}

namespace paraphrase::app {

class AboutPage;
class ActionsPage;
class GeneralPage;
class Preferences;
class ShortcutSettingsPage;
class ModelsController;
class ModelsSettingsPage;
class SettingsNav;

/// Settings (settings-models.html): the page list and the pages. Models is complete; General,
/// Shortcut and My actions say what is coming; Privacy and About say what is true now.
class SettingsWindow : public QWidget
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(SettingsWindow)

public:
    enum Page : std::uint8_t
    {
        General,
        Shortcut,
        MyActions,
        Models,
        Privacy,
        About,
    };

    /// `preferences` is the app's (General); without one the window keeps its own.
    explicit SettingsWindow(client::DaemonClient& client, Preferences* preferences = nullptr,
                            bool hasTray = false, QWidget* parent = nullptr);
    ~SettingsWindow() override;

    /// Opens a page and reads the models again.
    void showPage(Page page);
    /// Opens My actions with the editor on a new action ("New action..." in the menus).
    void newAction();
    /// What this desktop lets Paraphrase do, for the Shortcut page.
    void setDesktopSupport(const DesktopSupport& support);

protected:
    void changeEvent(QEvent* event) override;

private:
    void addPages();
    void applyTheme();

    client::DaemonClient& m_client;
    Tokens m_tokens;
    ModelsController* m_models;
    SettingsNav* m_nav;
    QStackedWidget* m_pages;
    ModelsSettingsPage* m_modelsPage = nullptr;
    AboutPage* m_aboutPage = nullptr;
    ActionsPage* m_actionsPage = nullptr;
    std::unique_ptr<Preferences> m_ownPreferences;
    Preferences* m_preferences = nullptr;
    bool m_hasTray = false;
    GeneralPage* m_generalPage = nullptr;
    ShortcutSettingsPage* m_shortcutPage = nullptr;
};

} // namespace paraphrase::app

#pragma once

#include "ui/common/theme.h"

#include <QWidget>

class QStackedWidget;

namespace paraphrase::client {
class DaemonClient;
}

namespace paraphrase::app {

class AboutPage;
class ActionsPage;
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

    explicit SettingsWindow(client::DaemonClient& client, QWidget* parent = nullptr);
    ~SettingsWindow() override;

    /// Opens a page and reads the models again.
    void showPage(Page page);
    /// Opens My actions with the editor on a new action ("New action..." in the menus).
    void newAction();

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
};

} // namespace paraphrase::app

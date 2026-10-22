#include "ui/settings/settings_window.h"

#include "ui/common/window_style.h"
#include "ui/models/models_controller.h"
#include "ui/settings/about_page.h"
#include "ui/settings/actions_page.h"
#include "ui/settings/models_settings_page.h"
#include "ui/settings/settings_nav.h"
#include "ui/settings/text_page.h"

#include <QEvent>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QStackedWidget>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

QWidget* scrolled(QWidget* page)
{
    auto* area = new QScrollArea;
    area->setWidgetResizable(true);
    area->setFrameShape(QFrame::NoFrame);
    area->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    area->setWidget(page);
    return area;
}

} // namespace

SettingsWindow::SettingsWindow(client::DaemonClient& client, QWidget* parent)
    : QWidget(parent)
    , m_client(client)
    , m_tokens(Theme::current())
    , m_models(new ModelsController(client, this))
    , m_nav(new SettingsNav({
          tr("General"),
          tr("Shortcut"),
          tr("My actions"),
          tr("Models"),
          tr("Privacy"),
          tr("About"),
      }))
    , m_pages(new QStackedWidget)
{
    setObjectName(u"settingsWindow"_s);
    setWindowTitle(tr("Paraphrase settings"));
    resize(1080, 780);
    auto* root = new QHBoxLayout(this);
    root->setContentsMargins(24, 24, 24, 24);
    root->setSpacing(28);
    root->addWidget(m_nav, 0, Qt::AlignTop);
    root->addWidget(m_pages, 1);
    addPages();
    connect(m_nav, &SettingsNav::pageChosen, this, [this](int page) { showPage(static_cast<Page>(page)); });
    applyTheme();
}

SettingsWindow::~SettingsWindow() = default;

void SettingsWindow::addPages()
{
    m_pages->addWidget(scrolled(new TextPage(
        tr("General"),
        {
            tr("Starting with your computer, the tray icon and the language of the app come here with "
               "the popup, in a later version."),
        })));
    m_pages->addWidget(scrolled(new TextPage(
        tr("Shortcut"),
        {
            tr("The global shortcut that opens Paraphrase over any app comes in a later version. Until "
               "then everything works in Paraphrase's own window."),
        })));
    m_actionsPage = new ActionsPage(m_client);
    m_pages->addWidget(scrolled(m_actionsPage));
    m_modelsPage = new ModelsSettingsPage(*m_models);
    m_pages->addWidget(scrolled(m_modelsPage));
    m_pages->addWidget(scrolled(new TextPage(
        tr("Privacy"),
        {
            tr("What you write is read by the model on this computer and nowhere else. Paraphrase keeps "
               "none of it: there is no history, no account and no usage data. It goes online only to "
               "download a model you chose."),
            tr("A history you can turn on, kept on this computer only, may come in a later version. It "
               "will be off unless you turn it on."),
        })));
    m_aboutPage = new AboutPage(*m_models);
    m_pages->addWidget(scrolled(m_aboutPage));
}

void SettingsWindow::showPage(Page page)
{
    m_nav->setCurrent(page);
    m_pages->setCurrentIndex(page);
    m_models->refresh();
    m_actionsPage->reload();
}

void SettingsWindow::newAction()
{
    showPage(MyActions);
    m_actionsPage->startNew();
}

void SettingsWindow::applyTheme()
{
    m_tokens = Theme::current();
    setStyleSheet(windowStyleSheet(m_tokens));
    m_modelsPage->setTheme(m_tokens);
    m_aboutPage->setTheme(m_tokens);
}

void SettingsWindow::changeEvent(QEvent* event)
{
    if ((event->type() == QEvent::ThemeChange || event->type() == QEvent::PaletteChange) &&
        Theme::current().dark != m_tokens.dark) {
        applyTheme();
    }
    QWidget::changeEvent(event);
}

} // namespace paraphrase::app

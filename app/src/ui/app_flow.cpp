#include "app_flow.h"

#include "daemon_client.h"
#include "main_window.h"
#include "model_info.h"
#include "settings_window.h"
#include "setup_window.h"

#include <QDBusMessage>
#include <QTimer>

#include <algorithm>

using namespace Qt::StringLiterals;

namespace paraphrase::app {

AppFlow::AppFlow(QObject* parent)
    : QObject(parent)
    , m_client(std::make_unique<client::DaemonClient>())
{}

AppFlow::~AppFlow() = default;

void AppFlow::start()
{
    // When the daemon cannot start, the main window says why (it connects on its own).
    if (m_client->connectToDaemon().isEmpty() && !hasModel()) {
        showSetup();
        return;
    }
    showMain();
}

bool AppFlow::hasModel()
{
    const QList<ModelInfo> models =
        parseModels(m_client->call(u"ListModels"_s, {}).arguments().value(0).toString());
    return std::ranges::any_of(models, [](const ModelInfo& m) { return m.isModel() && m.installed; });
}

void AppFlow::showSetup()
{
    m_setup = std::make_unique<SetupWindow>(*m_client);
    connect(m_setup.get(), &SetupWindow::finished, this, [this] {
        m_setup->hide();
        showMain();
        m_setup.release()->deleteLater();
    });
    m_setup->show();
    QTimer::singleShot(0, m_setup.get(), &SetupWindow::start);
}

void AppFlow::showMain()
{
    m_main = std::make_unique<MainWindow>();
    connect(m_main.get(), &MainWindow::settingsRequested, this, &AppFlow::showSettings);
    m_main->show();
    // Connect after the window is up: starting the daemon can take a moment.
    QTimer::singleShot(0, m_main.get(), &MainWindow::start);
}

void AppFlow::showSettings()
{
    if (!m_settings) {
        m_settings = std::make_unique<SettingsWindow>(*m_client);
    }
    m_settings->showPage(SettingsWindow::Models);
    m_settings->show();
    m_settings->raise();
    m_settings->activateWindow();
}

} // namespace paraphrase::app

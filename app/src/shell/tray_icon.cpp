#include "shell/tray_icon.h"

#include <QAction>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>
#include <QDBusServiceWatcher>
#include <QGuiApplication>
#include <QIcon>
#include <QMenu>
#include <QSystemTrayIcon>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr int kMenuIconSize = 16;
const QString kWatcher = u"org.kde.StatusNotifierWatcher"_s;
const QString kWatcherPath = u"/StatusNotifierWatcher"_s;

/// The tray protocol's own answer, asked without Qt so Qt's one check is not spent early.
bool hostRegistered(const QString& service)
{
    const QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.interface()->isServiceRegistered(service)) {
        return false;
    }
    const QDBusInterface watcher(service, kWatcherPath, kWatcher, bus);
    return watcher.property("IsStatusNotifierHostRegistered").toBool();
}

/// Waits for the watcher and its first host, then calls `ready` once.
class TrayWaiter : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TrayWaiter)

public:
    TrayWaiter(QObject* context, std::function<void()> ready, QString service)
        : QObject(context)
        , m_ready(std::move(ready))
        , m_service(std::move(service))
        , m_names(new QDBusServiceWatcher(m_service, QDBusConnection::sessionBus(),
                                          QDBusServiceWatcher::WatchForRegistration, this))
    {
        connect(m_names, &QDBusServiceWatcher::serviceRegistered, this, &TrayWaiter::check);
        // A watcher may come first and its host (the panel's tray) a moment later.
        QDBusConnection::sessionBus().connect(m_service, kWatcherPath, kWatcher,
                                              u"StatusNotifierHostRegistered"_s, this, SLOT(check()));
    }

    ~TrayWaiter() override = default;

public Q_SLOTS:
    void check()
    {
        if (m_done || !hostRegistered(m_service)) {
            return;
        }
        m_done = true;
        m_ready();
        deleteLater();
    }

private:
    std::function<void()> m_ready;
    QString m_service;
    QDBusServiceWatcher* m_names = nullptr;
    bool m_done = false;
};

} // namespace

TrayIcon::TrayIcon(QObject* parent)
    : QObject(parent)
    , m_tray(new QSystemTrayIcon(this))
{
    auto* menu = new QMenu;
    // An icon per item, as in the app's menus (menus.html). The desktop draws this menu in
    // its own theme, so its icons come first; ours, in the muted colour, where it has none.
    const auto item = [this, menu](const QString& text, auto signal, const QString& themed,
                                   const QString& own) {
        QAction* action = menu->addAction(text, this, signal);
        m_icons.append({action, {themed, own}});
    };
    item(tr("Open Paraphrase"), &TrayIcon::windowRequested, u"document-edit"_s, u"pen"_s);
    item(tr("Work on the selected text"), &TrayIcon::popupRequested, u"edit-select-text"_s,
         u"text-cursor-input"_s);
    menu->addSeparator();
    item(tr("Settings"), &TrayIcon::settingsRequested, u"configure"_s, u"settings"_s);
    item(tr("Quit"), &TrayIcon::quitRequested, u"application-exit"_s, u"power"_s);
    m_tray->setContextMenu(menu);
    m_tray->setToolTip(tr("Paraphrase: writing help on this device"));
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            Q_EMIT windowRequested();
        }
    });
    applyTheme(Theme::current()); // an icon before it shows: none set made Qt warn
    m_tray->show();
}

void TrayIcon::whenAvailable(QObject* context, const std::function<void()>& ready, const QString& watcher)
{
    const QString service = watcher.isEmpty() ? kWatcher : watcher;
    // An X11 desktop may have only an XEmbed tray, which Qt finds without the watcher.
    if (hostRegistered(service) ||
        (watcher.isEmpty() && QGuiApplication::platformName() == u"xcb"_s && available())) {
        ready();
        return;
    }
    new TrayWaiter(context, ready, service);
}

TrayIcon::~TrayIcon()
{
    delete m_tray->contextMenu();
}

bool TrayIcon::available()
{
    return QSystemTrayIcon::isSystemTrayAvailable();
}

void TrayIcon::applyTheme(const Tokens& t)
{
    m_tray->setIcon(Theme::icon(u"pen"_s, t.accent, 64));
    for (const auto& [action, names] : std::as_const(m_icons)) {
        action->setIcon(QIcon::fromTheme(names.first, Theme::icon(names.second, t.muted, kMenuIconSize)));
    }
}

} // namespace paraphrase::app

#include "tray_icon.moc"

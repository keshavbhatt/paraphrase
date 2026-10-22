#include "portal_paste_keys.h"

#include "portal.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QGuiApplication>
#include <QSettings>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

const QString kInterface = u"org.freedesktop.portal.RemoteDesktop"_s;
const QString kTokenKey = u"integration/remoteDesktopToken"_s;
constexpr int kControlL = 0xffe3; // XKB keysyms
constexpr int kV = 0x76;

} // namespace

PortalPasteKeys::PortalPasteKeys()
    : m_available(QGuiApplication::platformName() == u"wayland"_s && portal::hasInterface(kInterface))
{}

PortalPasteKeys::~PortalPasteKeys()
{
    closeSession();
}

QString PortalPasteKeys::name() const
{
    return QObject::tr("Paste through the RemoteDesktop portal");
}

bool PortalPasteKeys::startSession()
{
    portal::registerApp();
    const auto created = portal::request(kInterface, u"CreateSession"_s,
                                         {QVariantMap{{u"session_handle_token"_s, portal::token()}}}, 10'000);
    if (!created) {
        return false;
    }
    const QString session = created->value(u"session_handle"_s).toString();
    QVariantMap select{{u"types"_s, kKeyboard}, {u"persist_mode"_s, kPersistUntilRevoked}};
    if (const QString saved = QSettings().value(kTokenKey).toString(); !saved.isEmpty()) {
        select.insert(u"restore_token"_s, saved);
    }
    const QVariant path = QVariant::fromValue(QDBusObjectPath(session));
    if (!portal::request(kInterface, u"SelectDevices"_s, {path, select}, 10'000)) {
        return false;
    }
    const auto started =
        portal::request(kInterface, u"Start"_s, {path, QString(), QVariantMap()}, kStartTimeoutMs);
    if (!started) {
        return false;
    }
    // A new token each time: the old one is used up (portal spec).
    QSettings().setValue(kTokenKey, started->value(u"restore_token"_s).toString());
    m_session = session;
    return true;
}

bool PortalPasteKeys::key(int keysym, bool pressed) const
{
    QDBusMessage call = QDBusMessage::createMethodCall(QString::fromLatin1(portal::kService),
                                                       QString::fromLatin1(portal::kPath), kInterface,
                                                       u"NotifyKeyboardKeysym"_s);
    call << QVariant::fromValue(QDBusObjectPath(m_session)) << QVariantMap() << keysym
         << static_cast<uint>(pressed ? 1 : 0);
    return QDBusConnection::sessionBus().call(call, QDBus::Block, 2000).type() == QDBusMessage::ReplyMessage;
}

bool PortalPasteKeys::sendPaste()
{
    if (!m_available || (m_session.isEmpty() && !startSession())) {
        return false;
    }
    if (key(kControlL, true) && key(kV, true) && key(kV, false) && key(kControlL, false)) {
        return true;
    }
    // The session ended (revoked, the desktop restarted): start a new one next time.
    (void)key(kControlL, false);
    closeSession();
    return false;
}

void PortalPasteKeys::closeSession()
{
    if (m_session.isEmpty()) {
        return;
    }
    const QDBusMessage call = QDBusMessage::createMethodCall(QString::fromLatin1(portal::kService), m_session,
                                                             u"org.freedesktop.portal.Session"_s, u"Close"_s);
    QDBusConnection::sessionBus().call(call, QDBus::NoBlock);
    m_session.clear();
}

} // namespace paraphrase::app

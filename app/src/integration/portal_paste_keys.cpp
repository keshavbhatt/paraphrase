#include "portal_paste_keys.h"

#include "portal.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QGuiApplication>
#include <QSettings>

#include <array>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

const QString kInterface = u"org.freedesktop.portal.RemoteDesktop"_s;
const QString kTokenKey = u"integration/remoteDesktopToken"_s;
const QString kRefusedKey = u"integration/remoteDesktopRefused"_s;

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

bool PortalPasteKeys::startSession(bool* cancelled)
{
    int response = portal::kNoResponse;
    const auto said = [&response, cancelled] {
        if (cancelled != nullptr) {
            *cancelled = response == portal::kCancelled;
        }
        return false;
    };
    portal::registerApp();
    const auto created =
        portal::request(kInterface, u"CreateSession"_s,
                        {QVariantMap{{u"session_handle_token"_s, portal::token()}}}, 10'000, &response);
    if (!created) {
        return said();
    }
    const QString session = created->value(u"session_handle"_s).toString();
    QVariantMap select{{u"types"_s, kKeyboard}, {u"persist_mode"_s, kPersistUntilRevoked}};
    if (const QString saved = QSettings().value(kTokenKey).toString(); !saved.isEmpty()) {
        select.insert(u"restore_token"_s, saved);
    }
    const QVariant path = QVariant::fromValue(QDBusObjectPath(session));
    if (!portal::request(kInterface, u"SelectDevices"_s, {path, select}, 10'000, &response)) {
        return said();
    }
    const auto started =
        portal::request(kInterface, u"Start"_s, {path, QString(), QVariantMap()}, kStartTimeoutMs, &response);
    if (!started) {
        return said();
    }
    // A new token each time: the old one is used up (portal spec).
    QSettings().setValue(kTokenKey, started->value(u"restore_token"_s).toString());
    QSettings().remove(kRefusedKey);
    m_session = session;
    return true;
}

TypingPermission PortalPasteKeys::typingPermission() const
{
    if (!m_available) {
        return TypingPermission::Unavailable;
    }
    if (!m_session.isEmpty()) {
        return TypingPermission::Allowed;
    }
    const QSettings settings;
    if (settings.value(kRefusedKey, false).toBool()) {
        return TypingPermission::Refused;
    }
    // A token from an earlier answer: allowed then, and restored without asking.
    return settings.value(kTokenKey).toString().isEmpty() ? TypingPermission::NotAsked
                                                          : TypingPermission::Allowed;
}

TypingPermission PortalPasteKeys::askTyping()
{
    bool cancelled = false;
    if (m_available && m_session.isEmpty() && !startSession(&cancelled) && cancelled) {
        // Only a "no" is kept. A portal error or no answer is asked again next time, as a
        // failure kept for good would leave Replace on the clipboard for no reason.
        QSettings().setValue(kRefusedKey, true);
    }
    return typingPermission();
}

bool PortalPasteKeys::key(std::uint32_t keysym, bool pressed) const
{
    QDBusMessage call = QDBusMessage::createMethodCall(QString::fromLatin1(portal::kService),
                                                       QString::fromLatin1(portal::kPath), kInterface,
                                                       u"NotifyKeyboardKeysym"_s);
    call << QVariant::fromValue(QDBusObjectPath(m_session)) << QVariantMap() << static_cast<int>(keysym)
         << static_cast<uint>(pressed ? 1 : 0);
    return QDBusConnection::sessionBus().call(call, QDBus::Block, 2000).type() == QDBusMessage::ReplyMessage;
}

bool PortalPasteKeys::pressChord(PasteChord chord, bool* firstFailed) const
{
    const std::array<std::uint32_t, 3> keys = chordKeysyms(chord);
    const std::size_t count = keys.back() == 0 ? 2 : 3; // Shift+Insert has two keys
    std::size_t pressed = 0;
    while (pressed < count && key(keys.at(pressed), true)) {
        ++pressed;
    }
    *firstFailed = pressed == 0;
    const bool all = pressed == count;
    bool released = true;
    while (pressed > 0) { // release what went down, so no modifier stays held
        released = key(keys.at(--pressed), false) && released;
    }
    return all && released;
}

bool PortalPasteKeys::sendPaste(PasteChord chord)
{
    if (!m_available || (m_session.isEmpty() && !startSession())) {
        return false;
    }
    bool firstFailed = false;
    if (pressChord(chord, &firstFailed)) {
        return true;
    }
    // The session ended (revoked, the desktop restarted). When not one key went through, a
    // fresh session tries once more: nothing can be pasted twice. Otherwise the result stays
    // on the clipboard.
    closeSession();
    if (!firstFailed || !startSession()) {
        return false;
    }
    return pressChord(chord, &firstFailed);
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

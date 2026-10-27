#include "x11_backends.h"

#include "x11.h"

#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QKeySequence>

#include <array>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <ranges>
#include <vector>
#include <xcb/xcb.h>
#include <xcb/xtest.h>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

/// Num Lock (Mod2) and Caps Lock change the key state; grab with each combination.
constexpr std::array<std::uint16_t, 4> kLockMasks = {
    0,
    XCB_MOD_MASK_2,
    XCB_MOD_MASK_LOCK,
    XCB_MOD_MASK_2 | XCB_MOD_MASK_LOCK,
};
constexpr std::uint8_t kKeyMask = 0x7f; ///< response_type without the "sent" bit
/// Every modifier bit but Num Lock (Mod2) and Caps Lock.
constexpr unsigned kWithoutLocks =
    0xffffU ^ (static_cast<unsigned>(XCB_MOD_MASK_2) | static_cast<unsigned>(XCB_MOD_MASK_LOCK));

template <typename T>
using Reply = std::unique_ptr<T, decltype(&std::free)>;

QByteArray windowProperty(xcb_connection_t* conn, std::uint32_t window, std::uint32_t property,
                          std::uint32_t type)
{
    const Reply<xcb_get_property_reply_t> reply(
        xcb_get_property_reply(conn, xcb_get_property(conn, 0, window, property, type, 0, 1024), nullptr),
        &std::free);
    if (!reply) {
        return {};
    }
    return {static_cast<const char*>(xcb_get_property_value(reply.get())),
            xcb_get_property_value_length(reply.get())};
}

} // namespace

X11WindowTracker::X11WindowTracker()
    : m_conn(x11::connection())
{
    if (m_conn != nullptr) {
        m_netActive = x11::atom(m_conn, "_NET_ACTIVE_WINDOW");
    }
}

std::uint32_t X11WindowTracker::active() const
{
    const QByteArray value = windowProperty(m_conn, x11::rootWindow(m_conn), m_netActive, XCB_ATOM_WINDOW);
    std::uint32_t window = 0;
    if (value.size() >= static_cast<qsizetype>(sizeof window)) {
        std::memcpy(&window, value.constData(), sizeof window);
    }
    return window;
}

QString X11WindowTracker::activeWindow() const
{
    const std::uint32_t window = m_conn != nullptr ? active() : 0;
    return window == 0 ? QString() : QString::number(window);
}

QString X11WindowTracker::activeApp() const
{
    if (m_conn == nullptr || active() == 0) {
        return {};
    }
    // WM_CLASS is "instance\0Class\0": the class names the app.
    const QByteArray wmClass = windowProperty(m_conn, active(), XCB_ATOM_WM_CLASS, XCB_ATOM_STRING);
    return QString::fromLatin1(wmClass.split('\0').value(1));
}

bool X11WindowTracker::waitUntilActive(const QString& window, int timeoutMs)
{
    const QDeadlineTimer deadline(timeoutMs);
    while (activeWindow() != window && !deadline.hasExpired()) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
    return !window.isEmpty() && activeWindow() == window;
}

XTestPasteKeys::XTestPasteKeys()
    : m_conn(x11::connection())
{
    if (m_conn != nullptr && !x11::isXwayland(m_conn)) {
        const xcb_query_extension_reply_t* ext = xcb_get_extension_data(m_conn, &xcb_test_id);
        m_available = ext != nullptr && ext->present != 0;
    }
}

QString XTestPasteKeys::name() const
{
    return QObject::tr("Paste with XTest");
}

bool XTestPasteKeys::sendPaste(PasteChord chord)
{
    if (!m_available) {
        return false;
    }
    std::vector<std::uint8_t> codes;
    for (const std::uint32_t keysym : chordKeysyms(chord)) {
        if (keysym == 0) {
            continue;
        }
        const auto code = x11::keycode(m_conn, keysym);
        if (!code) {
            return false; // a layout with no such key: the result stays on the clipboard
        }
        codes.push_back(*code);
    }
    const std::uint32_t root = x11::rootWindow(m_conn);
    for (const std::uint8_t code : codes) {
        xcb_test_fake_input(m_conn, XCB_KEY_PRESS, code, XCB_CURRENT_TIME, root, 0, 0, 0);
    }
    for (const std::uint8_t code : std::views::reverse(codes)) {
        xcb_test_fake_input(m_conn, XCB_KEY_RELEASE, code, XCB_CURRENT_TIME, root, 0, 0, 0);
    }
    xcb_flush(m_conn);
    return true;
}

X11Shortcut::X11Shortcut(QObject* parent)
    : ShortcutBackend(parent)
    , m_conn(x11::connection())
{
    if (m_conn != nullptr && x11::isXwayland(m_conn)) {
        m_conn = nullptr; // the GlobalShortcuts portal does it on Wayland
    }
    if (m_conn != nullptr) {
        QCoreApplication::instance()->installNativeEventFilter(this);
    }
}

X11Shortcut::~X11Shortcut()
{
    ungrab();
    if (m_conn != nullptr && QCoreApplication::instance() != nullptr) {
        QCoreApplication::instance()->removeNativeEventFilter(this);
    }
}

QString X11Shortcut::name() const
{
    return tr("Global shortcut (X11)");
}

bool X11Shortcut::bind(const QKeySequence& keys)
{
    ungrab();
    if (m_conn == nullptr || keys.isEmpty()) {
        return false;
    }
    const QKeyCombination combo = keys[0];
    const auto sym = x11::keysym(combo.key());
    const auto code = sym ? x11::keycode(m_conn, *sym) : std::nullopt;
    if (!code) {
        return false;
    }
    m_keycode = *code;
    m_modifiers = x11::modifiers(combo.keyboardModifiers());
    bool ok = true;
    for (const std::uint16_t lock : kLockMasks) {
        const xcb_void_cookie_t cookie =
            xcb_grab_key_checked(m_conn, 1, x11::rootWindow(m_conn), m_modifiers | lock, m_keycode,
                                 XCB_GRAB_MODE_ASYNC, XCB_GRAB_MODE_ASYNC);
        const Reply<xcb_generic_error_t> error(xcb_request_check(m_conn, cookie), &std::free);
        ok = ok && !error; // another app holds these keys
    }
    if (!ok) {
        ungrab();
        return false;
    }
    m_keys = keys;
    return true;
}

void X11Shortcut::ungrab()
{
    if (m_conn == nullptr || m_keycode == 0) {
        return;
    }
    for (const std::uint16_t lock : kLockMasks) {
        xcb_ungrab_key(m_conn, m_keycode, x11::rootWindow(m_conn), m_modifiers | lock);
    }
    xcb_flush(m_conn);
    m_keycode = 0;
    m_keys = {};
}

QString X11Shortcut::description() const
{
    return m_keys.isEmpty() ? tr("No shortcut: the keys are taken by another app")
                            : tr("%1 opens the popup").arg(m_keys.toString(QKeySequence::NativeText));
}

bool X11Shortcut::nativeEventFilter(const QByteArray& eventType, void* message, qintptr* /*result*/)
{
    if (m_keycode == 0 || eventType != "xcb_generic_event_t") {
        return false;
    }
    const auto* event = static_cast<const xcb_generic_event_t*>(message);
    if ((event->response_type & kKeyMask) != XCB_KEY_PRESS) {
        return false;
    }
    const auto* key = static_cast<const xcb_key_press_event_t*>(message);
    const auto state = static_cast<std::uint16_t>(static_cast<unsigned>(key->state) & kWithoutLocks);
    if (key->detail != m_keycode || state != m_modifiers) {
        return false;
    }
    trigger();
    return true;
}

} // namespace paraphrase::app

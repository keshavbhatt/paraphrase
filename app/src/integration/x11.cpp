#include "x11.h"

#include <QGuiApplication>

#include <cstdlib>
#include <memory>
#include <utility>
#include <xcb/xcb.h>

using namespace Qt::StringLiterals;

namespace paraphrase::app::x11 {
namespace {

constexpr std::uint32_t kKeysymSpace = 0x20;
constexpr std::uint32_t kKeysymF1 = 0xffbe;

template <typename T>
using Reply = std::unique_ptr<T, decltype(&std::free)>;

} // namespace

xcb_connection_t* connection()
{
    if (QGuiApplication::platformName() != u"xcb"_s) {
        return nullptr;
    }
    const auto* native = qGuiApp->nativeInterface<QNativeInterface::QX11Application>();
    return native != nullptr ? native->connection() : nullptr;
}

std::uint32_t rootWindow(xcb_connection_t* conn)
{
    return xcb_setup_roots_iterator(xcb_get_setup(conn)).data->root;
}

bool isXwayland(xcb_connection_t* conn)
{
    static constexpr char kName[] = "XWAYLAND";
    const Reply<xcb_query_extension_reply_t> reply(
        xcb_query_extension_reply(conn, xcb_query_extension(conn, sizeof kName - 1, kName), nullptr),
        &std::free);
    return reply && reply->present != 0;
}

std::uint32_t atom(xcb_connection_t* conn, const QByteArray& name)
{
    const Reply<xcb_intern_atom_reply_t> reply(
        xcb_intern_atom_reply(
            conn, xcb_intern_atom(conn, 0, static_cast<std::uint16_t>(name.size()), name.constData()),
            nullptr),
        &std::free);
    return reply ? reply->atom : static_cast<std::uint32_t>(XCB_ATOM_NONE);
}

std::optional<std::uint8_t> keycode(xcb_connection_t* conn, std::uint32_t keysym)
{
    const xcb_setup_t* setup = xcb_get_setup(conn);
    const auto count = static_cast<std::uint8_t>(setup->max_keycode - setup->min_keycode + 1);
    const Reply<xcb_get_keyboard_mapping_reply_t> map(
        xcb_get_keyboard_mapping_reply(conn, xcb_get_keyboard_mapping(conn, setup->min_keycode, count),
                                       nullptr),
        &std::free);
    if (!map) {
        return std::nullopt;
    }
    const xcb_keysym_t* syms = xcb_get_keyboard_mapping_keysyms(map.get());
    const int perCode = map->keysyms_per_keycode;
    for (int code = 0; std::cmp_less(code, count); ++code) {
        for (int i = 0; i < perCode; ++i) {
            if (syms[(code * perCode) + i] == keysym) {
                return static_cast<std::uint8_t>(setup->min_keycode + code);
            }
        }
    }
    return std::nullopt;
}

std::optional<std::uint32_t> keysym(Qt::Key key)
{
    if (key >= Qt::Key_A && key <= Qt::Key_Z) {
        return static_cast<std::uint32_t>(key - Qt::Key_A) + 'a';
    }
    if (key >= Qt::Key_0 && key <= Qt::Key_9) {
        return static_cast<std::uint32_t>(key);
    }
    if (key == Qt::Key_Space) {
        return kKeysymSpace;
    }
    if (key >= Qt::Key_F1 && key <= Qt::Key_F12) {
        return kKeysymF1 + static_cast<std::uint32_t>(key - Qt::Key_F1);
    }
    return std::nullopt;
}

std::uint16_t modifiers(Qt::KeyboardModifiers mods)
{
    std::uint16_t out = 0;
    if (mods.testFlag(Qt::ShiftModifier)) {
        out |= XCB_MOD_MASK_SHIFT;
    }
    if (mods.testFlag(Qt::ControlModifier)) {
        out |= XCB_MOD_MASK_CONTROL;
    }
    if (mods.testFlag(Qt::AltModifier)) {
        out |= XCB_MOD_MASK_1;
    }
    if (mods.testFlag(Qt::MetaModifier)) {
        out |= XCB_MOD_MASK_4;
    }
    return out;
}

} // namespace paraphrase::app::x11

#pragma once

#include <QByteArray>
#include <Qt>

#include <cstdint>
#include <optional>

struct xcb_connection_t;

namespace paraphrase::app::x11 {

/// Qt's X11 connection, or nullptr when the app does not run on X11 (platform "xcb").
[[nodiscard]] xcb_connection_t* connection();
[[nodiscard]] std::uint32_t rootWindow(xcb_connection_t* conn);
/// True on Xwayland (a Wayland session running an X11 app): there key grabs only see keys
/// sent to X windows and XTest input goes nowhere, so the X11 backends step aside.
[[nodiscard]] bool isXwayland(xcb_connection_t* conn);
[[nodiscard]] std::uint32_t atom(xcb_connection_t* conn, const QByteArray& name);
/// The key code that produces `keysym` on the current keyboard layout.
[[nodiscard]] std::optional<std::uint8_t> keycode(xcb_connection_t* conn, std::uint32_t keysym);
/// The X keysym for a Qt key: letters, digits, space and F1 to F12; nothing for others.
[[nodiscard]] std::optional<std::uint32_t> keysym(Qt::Key key);
/// X modifier bits for Qt modifiers (Shift, Control, Mod1 for Alt, Mod4 for Meta).
[[nodiscard]] std::uint16_t modifiers(Qt::KeyboardModifiers mods);

} // namespace paraphrase::app::x11

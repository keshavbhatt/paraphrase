#pragma once

#include "integration/backends.h"

#include <QString>

#include <array>
#include <cstdint>

namespace paraphrase::app {

/// The keys that paste. Shift+Insert pastes the clipboard in Qt, GTK, browsers, Electron,
/// LibreOffice and Java apps, and its keys are the same on every keyboard layout: a Russian
/// or Greek layout may have no "v" for Ctrl+V. Terminals paste the selection with
/// Shift+Insert, which is the text the user selected, so they get Ctrl+Shift+V.
enum class PasteChord : std::uint8_t
{
    ShiftInsert,
    CtrlShiftV,
};

/// The chord for an app, by its window class or desktop file name ("org.kde.konsole").
[[nodiscard]] PasteChord pasteChordFor(const QString& app);

/// A chord's XKB keysyms, modifiers first: pressed in order, released in reverse.
[[nodiscard]] std::array<std::uint32_t, 3> chordKeysyms(PasteChord chord);

/// Presses the paste keys in the active window: XTest on X11, the RemoteDesktop portal on
/// Wayland.
class PasteKeys
{
public:
    PasteKeys() = default;
    virtual ~PasteKeys() = default;
    PasteKeys(const PasteKeys&) = delete;
    PasteKeys& operator=(const PasteKeys&) = delete;
    PasteKeys(PasteKeys&&) = delete;
    PasteKeys& operator=(PasteKeys&&) = delete;

    [[nodiscard]] virtual QString name() const = 0;
    [[nodiscard]] virtual bool available() const = 0;
    /// False when the keys could not be sent (no permission, no session).
    [[nodiscard]] virtual bool sendPaste(PasteChord chord) = 0;
    [[nodiscard]] virtual TypingPermission typingPermission() const { return TypingPermission::NotNeeded; }
    virtual TypingPermission askTyping() { return typingPermission(); }
};

} // namespace paraphrase::app

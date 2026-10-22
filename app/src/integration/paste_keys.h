#pragma once

#include <QString>

namespace paraphrase::app {

/// Presses Ctrl+V in the active window: XTest on X11, the RemoteDesktop portal on Wayland.
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
    [[nodiscard]] virtual bool sendPaste() = 0;
};

} // namespace paraphrase::app

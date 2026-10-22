#pragma once

#include <QString>

namespace paraphrase::app {

/// The window that has the keyboard, so text is only ever typed into the window the user
/// chose (PLAN 6): its id is recorded when the text is captured and compared before typing.
class WindowTracker
{
public:
    WindowTracker() = default;
    virtual ~WindowTracker() = default;
    WindowTracker(const WindowTracker&) = delete;
    WindowTracker& operator=(const WindowTracker&) = delete;
    WindowTracker(WindowTracker&&) = delete;
    WindowTracker& operator=(WindowTracker&&) = delete;

    [[nodiscard]] virtual bool available() const = 0;
    /// An id of the active window, or "" when it cannot be told.
    [[nodiscard]] virtual QString activeWindow() const = 0;
    /// The active window's application, for "from Telegram"; may be "".
    [[nodiscard]] virtual QString activeApp() const = 0;
    /// Waits up to `timeoutMs` for `window` to be active again (the popup gives focus back
    /// when it hides); false when another window has it.
    [[nodiscard]] virtual bool waitUntilActive(const QString& window, int timeoutMs) = 0;
};

} // namespace paraphrase::app

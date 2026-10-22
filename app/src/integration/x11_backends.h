#pragma once

#include "backends.h"
#include "paste_keys.h"
#include "window_tracker.h"

#include <QAbstractNativeEventFilter>

#include <cstdint>

struct xcb_connection_t;

namespace paraphrase::app {

/// The active window on X11: the root window's _NET_ACTIVE_WINDOW, and its WM_CLASS.
class X11WindowTracker final : public WindowTracker
{
public:
    X11WindowTracker();
    [[nodiscard]] bool available() const override { return m_conn != nullptr; }
    [[nodiscard]] QString activeWindow() const override;
    [[nodiscard]] QString activeApp() const override;
    [[nodiscard]] bool waitUntilActive(const QString& window, int timeoutMs) override;

private:
    [[nodiscard]] std::uint32_t active() const;

    xcb_connection_t* m_conn = nullptr;
    std::uint32_t m_netActive = 0;
};

/// Ctrl+V on X11 through the XTEST extension.
class XTestPasteKeys final : public PasteKeys
{
public:
    XTestPasteKeys();
    [[nodiscard]] QString name() const override;
    [[nodiscard]] bool available() const override { return m_available; }
    [[nodiscard]] bool sendPaste() override;

private:
    xcb_connection_t* m_conn = nullptr;
    bool m_available = false;
};

/// The global shortcut on X11: a key grab on the root window, for every state of Num Lock
/// and Caps Lock, watched through Qt's native event filter.
class X11Shortcut final : public ShortcutBackend, public QAbstractNativeEventFilter
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(X11Shortcut)

public:
    explicit X11Shortcut(QObject* parent = nullptr);
    ~X11Shortcut() override;

    [[nodiscard]] QString name() const override;
    [[nodiscard]] bool available() const override { return m_conn != nullptr; }
    bool bind(const QKeySequence& keys) override;
    [[nodiscard]] QString description() const override;
    bool nativeEventFilter(const QByteArray& eventType, void* message, qintptr* result) override;

private:
    void ungrab();

    xcb_connection_t* m_conn = nullptr;
    std::uint8_t m_keycode = 0;
    std::uint16_t m_modifiers = 0;
    QKeySequence m_keys;
};

} // namespace paraphrase::app

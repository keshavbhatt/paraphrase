#pragma once

#include "paste_keys.h"

#include <QString>

#include <cstdint>

namespace paraphrase::app {

/// The paste keys through the RemoteDesktop portal (Wayland): a keyboard-only session, started the
/// first time a result is put back. The desktop asks the user once; the restore token it
/// gives back is kept in QSettings, so later sessions start without asking.
class PortalPasteKeys final : public PasteKeys
{
public:
    static constexpr int kStartTimeoutMs = 120'000; ///< the user may need to answer a dialog
    static constexpr uint kKeyboard = 1;            ///< device type: keyboard only
    static constexpr uint kPersistUntilRevoked = 2;

    PortalPasteKeys();
    ~PortalPasteKeys() override;
    PortalPasteKeys(const PortalPasteKeys&) = delete;
    PortalPasteKeys& operator=(const PortalPasteKeys&) = delete;
    PortalPasteKeys(PortalPasteKeys&&) = delete;
    PortalPasteKeys& operator=(PortalPasteKeys&&) = delete;

    [[nodiscard]] QString name() const override;
    [[nodiscard]] bool available() const override { return m_available; }
    [[nodiscard]] bool sendPaste(PasteChord chord) override;
    [[nodiscard]] TypingPermission typingPermission() const override;
    TypingPermission askTyping() override;

private:
    /// Starts a session; `cancelled` says whether the user said no in the desktop's dialog.
    [[nodiscard]] bool startSession(bool* cancelled = nullptr);
    [[nodiscard]] bool key(std::uint32_t keysym, bool pressed) const;
    /// Presses and releases the chord. Nothing pressed yet when the first key fails, so a
    /// second try cannot paste twice: `firstFailed` says so.
    [[nodiscard]] bool pressChord(PasteChord chord, bool* firstFailed) const;
    void closeSession();

    bool m_available = false;
    QString m_session;
};

} // namespace paraphrase::app

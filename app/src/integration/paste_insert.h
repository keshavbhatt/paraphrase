#pragma once

#include "backends.h"
#include "clipboard_access.h"
#include "paste_keys.h"
#include "window_tracker.h"

#include <memory>

namespace paraphrase::app {

/// Puts a result back by pasting it into the window the text came from: the result goes on
/// the clipboard, where it stays, and Ctrl+V pastes it. It types only into the remembered
/// window; when that window is not active again, or anything is missing, the user pastes it
/// (PLAN 6). The clipboard is not put back afterwards (owner, 2026-10-26): a paste with no text
/// field in focus lost the result.
class PasteInsert final : public InsertBackend
{
public:
    static constexpr int kFocusWaitMs = 600; ///< for focus to come back after the popup hides

    PasteInsert(QString name, std::shared_ptr<WindowTracker> tracker,
                std::shared_ptr<ClipboardAccess> clipboard, std::shared_ptr<PasteKeys> keys);
    ~PasteInsert() override = default;
    PasteInsert(const PasteInsert&) = delete;
    PasteInsert& operator=(const PasteInsert&) = delete;
    PasteInsert(PasteInsert&&) = delete;
    PasteInsert& operator=(PasteInsert&&) = delete;

    [[nodiscard]] QString name() const override { return m_name; }
    [[nodiscard]] bool available() const override;
    [[nodiscard]] InsertOutcome insert(const QString& text, const CapturedText& target) override;
    [[nodiscard]] TypingPermission typingPermission() const override { return m_keys->typingPermission(); }
    TypingPermission askTyping() override { return m_keys->askTyping(); }

private:
    QString m_name;
    std::shared_ptr<WindowTracker> m_tracker;
    std::shared_ptr<ClipboardAccess> m_clipboard;
    std::shared_ptr<PasteKeys> m_keys;
};

} // namespace paraphrase::app

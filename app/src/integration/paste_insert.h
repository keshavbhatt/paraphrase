#pragma once

#include "backends.h"
#include "clipboard_access.h"
#include "paste_keys.h"
#include "window_tracker.h"

#include <QTimer>

#include <memory>

namespace paraphrase::app {

/// Puts a result back by pasting it into the window the text came from: save the clipboard,
/// put the result on it, press Ctrl+V, put the user's clipboard back a moment later. It types
/// only into the remembered window; when that window is not active again, or anything is
/// missing, the result stays on the clipboard for the user to paste (PLAN 6).
class PasteInsert final : public InsertBackend
{
public:
    static constexpr int kFocusWaitMs = 600;    ///< for focus to come back after the popup hides
    static constexpr int kRestoreDelayMs = 700; ///< for the target app to read the clipboard

    PasteInsert(QString name, std::shared_ptr<WindowTracker> tracker,
                std::shared_ptr<ClipboardAccess> clipboard, std::shared_ptr<PasteKeys> keys);
    ~PasteInsert() override;
    PasteInsert(const PasteInsert&) = delete;
    PasteInsert& operator=(const PasteInsert&) = delete;
    PasteInsert(PasteInsert&&) = delete;
    PasteInsert& operator=(PasteInsert&&) = delete;

    [[nodiscard]] QString name() const override { return m_name; }
    [[nodiscard]] bool available() const override;
    [[nodiscard]] InsertOutcome insert(const QString& text, const CapturedText& target) override;
    /// Puts back what the user had copied now instead of after the delay (tests, shutdown).
    void restoreNow();

private:
    QString m_name;
    std::shared_ptr<WindowTracker> m_tracker;
    std::shared_ptr<ClipboardAccess> m_clipboard;
    std::shared_ptr<PasteKeys> m_keys;
    QTimer m_restore;
    ClipboardContent m_saved;
    bool m_pending = false;
};

} // namespace paraphrase::app

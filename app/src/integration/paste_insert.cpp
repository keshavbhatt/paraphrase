#include "paste_insert.h"

namespace paraphrase::app {

PasteInsert::PasteInsert(QString name, std::shared_ptr<WindowTracker> tracker,
                         std::shared_ptr<ClipboardAccess> clipboard, std::shared_ptr<PasteKeys> keys)
    : m_name(std::move(name))
    , m_tracker(std::move(tracker))
    , m_clipboard(std::move(clipboard))
    , m_keys(std::move(keys))
{}

bool PasteInsert::available() const
{
    return m_tracker->available() && m_keys->available();
}

InsertOutcome PasteInsert::insert(const QString& text, const CapturedText& target)
{
    // The result goes on the clipboard and stays there (owner, 2026-10-26): a paste that
    // lands nowhere, with no text field in focus, is one Ctrl+V away instead of lost.
    m_clipboard->setText(text);
    // Never type into a window the user did not choose (PLAN 6).
    const bool sameWindow =
        !target.window.isEmpty() && m_tracker->waitUntilActive(target.window, kFocusWaitMs);
    if (!sameWindow || !m_keys->sendPaste()) {
        return InsertOutcome::OnClipboard;
    }
    return InsertOutcome::Typed;
}

} // namespace paraphrase::app

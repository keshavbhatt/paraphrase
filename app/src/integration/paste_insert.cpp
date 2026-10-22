#include "paste_insert.h"

namespace paraphrase::app {

PasteInsert::PasteInsert(QString name, std::shared_ptr<WindowTracker> tracker,
                         std::shared_ptr<ClipboardAccess> clipboard, std::shared_ptr<PasteKeys> keys)
    : m_name(std::move(name))
    , m_tracker(std::move(tracker))
    , m_clipboard(std::move(clipboard))
    , m_keys(std::move(keys))
{
    m_restore.setSingleShot(true);
    m_restore.setInterval(kRestoreDelayMs);
    QObject::connect(&m_restore, &QTimer::timeout, [this] { restoreNow(); });
}

PasteInsert::~PasteInsert()
{
    restoreNow();
}

bool PasteInsert::available() const
{
    return m_tracker->available() && m_keys->available();
}

InsertOutcome PasteInsert::insert(const QString& text, const CapturedText& target)
{
    restoreNow(); // a paste still waiting to restore: finish it first
    // Never type into a window the user did not choose (PLAN 6).
    const bool sameWindow =
        !target.window.isEmpty() && m_tracker->waitUntilActive(target.window, kFocusWaitMs);
    if (!sameWindow) {
        m_clipboard->setText(text);
        return InsertOutcome::OnClipboard;
    }
    m_saved = m_clipboard->save();
    m_pending = true;
    m_clipboard->setText(text);
    if (!m_keys->sendPaste()) {
        m_pending = false; // the result stays on the clipboard for the user
        m_saved.clear();
        return InsertOutcome::OnClipboard;
    }
    m_restore.start();
    return InsertOutcome::Typed;
}

void PasteInsert::restoreNow()
{
    m_restore.stop();
    if (m_pending) {
        m_pending = false;
        m_clipboard->restore(m_saved);
        m_saved.clear();
    }
}

} // namespace paraphrase::app

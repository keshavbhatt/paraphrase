#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

#include <utility>

namespace paraphrase::app {

/// Clipboard content in several formats, as data-control offers it.
using ClipboardContent = QList<std::pair<QString, QByteArray>>; ///< (mime type, data)

/// Sets the clipboard without needing focus where the session allows it (Wayland
/// data-control); on X11 the Qt clipboard does. The result is put there to be pasted, and
/// it stays (owner, 2026-10-26; DOCS/platform-matrix.md, Clipboard).
class ClipboardAccess
{
public:
    ClipboardAccess() = default;
    virtual ~ClipboardAccess() = default;
    ClipboardAccess(const ClipboardAccess&) = delete;
    ClipboardAccess& operator=(const ClipboardAccess&) = delete;
    ClipboardAccess(ClipboardAccess&&) = delete;
    ClipboardAccess& operator=(ClipboardAccess&&) = delete;

    virtual void setText(const QString& text) = 0;
};

} // namespace paraphrase::app

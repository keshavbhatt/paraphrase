#pragma once

#include <QByteArray>
#include <QList>
#include <QString>

#include <utility>

namespace paraphrase::app {

/// The clipboard's whole content, every format, so it can be put back after a paste.
using ClipboardContent = QList<std::pair<QString, QByteArray>>; ///< (mime type, data)

/// Reads and writes the clipboard without needing focus where the session allows it
/// (Wayland data-control); on X11 the Qt clipboard does. Used to paste a result and then
/// put back what the user had copied (PLAN 6, clipboard hygiene).
class ClipboardAccess
{
public:
    ClipboardAccess() = default;
    virtual ~ClipboardAccess() = default;
    ClipboardAccess(const ClipboardAccess&) = delete;
    ClipboardAccess& operator=(const ClipboardAccess&) = delete;
    ClipboardAccess(ClipboardAccess&&) = delete;
    ClipboardAccess& operator=(ClipboardAccess&&) = delete;

    [[nodiscard]] virtual ClipboardContent save() = 0;
    virtual void setText(const QString& text) = 0;
    virtual void restore(const ClipboardContent& content) = 0;
};

} // namespace paraphrase::app

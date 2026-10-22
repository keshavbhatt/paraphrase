#pragma once

#include "clipboard_access.h"

namespace paraphrase::app {

/// ClipboardAccess through Qt's clipboard: on X11 it works without focus.
class QtClipboardAccess final : public ClipboardAccess
{
public:
    static constexpr qsizetype kMaxSaved = qsizetype{16} * 1024 * 1024; ///< bigger clipboards are not kept

    [[nodiscard]] ClipboardContent save() override;
    void setText(const QString& text) override;
    void restore(const ClipboardContent& content) override;
};

} // namespace paraphrase::app

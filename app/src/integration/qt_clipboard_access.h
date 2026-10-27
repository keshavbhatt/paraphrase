#pragma once

#include "clipboard_access.h"

namespace paraphrase::app {

/// ClipboardAccess through Qt's clipboard: on X11 it works without focus.
class QtClipboardAccess final : public ClipboardAccess
{
public:
    void setText(const QString& text) override;
};

} // namespace paraphrase::app

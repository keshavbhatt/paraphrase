#pragma once

#include "ui/core/desktop_support.h"

namespace paraphrase::app {

class Integrations;

/// What the backends found, in plain words for Settings (settings-general-shortcut.html,
/// note 2): no backend names, and the fallback said as what the user does instead.
[[nodiscard]] DesktopSupport describeDesktop(const Integrations& integrations);

} // namespace paraphrase::app

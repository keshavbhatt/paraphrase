#pragma once

#include <QString>

namespace paraphrase::app {

/// The name people know an app by, for "from KWrite" and "Replace in KWrite": the Name of its
/// desktop file when one is found ("org.kde.kwrite" -> "KWrite"), else the id's last part with
/// a capital ("telegram-desktop" -> "Telegram-desktop"). Empty stays empty.
[[nodiscard]] QString appDisplayName(const QString& appId);

} // namespace paraphrase::app

#pragma once

namespace paraphrase::app {

/// Starting Paraphrase at login, hidden in the tray (ADR-010): the Background portal inside a
/// Flatpak, an autostart entry elsewhere (the snap maps ~/.config/autostart to its own).
class Autostart
{
public:
    [[nodiscard]] static bool enabled();
    static void setEnabled(bool on);
};

} // namespace paraphrase::app

#include "paste_keys.h"

#include <QStringList>

using namespace Qt::StringLiterals;

namespace paraphrase::app {
namespace {

constexpr std::uint32_t kShiftL = 0xffe1; // XKB keysyms
constexpr std::uint32_t kControlL = 0xffe3;
constexpr std::uint32_t kInsert = 0xff63;
constexpr std::uint32_t kV = 0x76;

/// Terminals whose paste is Ctrl+Shift+V, by window class or desktop file name, lower case.
/// xterm is not here: Ctrl+Shift+V does nothing there, and Shift+Insert is its only paste.
const QStringList& terminals()
{
    static const QStringList names = {
        u"konsole"_s,
        u"org.kde.konsole"_s,
        u"yakuake"_s,
        u"org.kde.yakuake"_s,
        u"gnome-terminal"_s,
        u"gnome-terminal-server"_s,
        u"org.gnome.terminal"_s,
        u"org.gnome.console"_s,
        u"kgx"_s,
        u"org.gnome.ptyxis"_s,
        u"ptyxis"_s,
        u"kitty"_s,
        u"alacritty"_s,
        u"org.alacritty.alacritty"_s,
        u"foot"_s,
        u"footclient"_s,
        u"org.wezfurlong.wezterm"_s,
        u"wezterm"_s,
        u"com.mitchellh.ghostty"_s,
        u"ghostty"_s,
        u"tilix"_s,
        u"com.gexperts.tilix"_s,
        u"terminator"_s,
        u"xfce4-terminal"_s,
        u"mate-terminal"_s,
        u"lxterminal"_s,
        u"qterminal"_s,
        u"terminology"_s,
        u"com.raggesilver.blackbox"_s,
        u"cool-retro-term"_s,
    };
    return names;
}

} // namespace

PasteChord pasteChordFor(const QString& app)
{
    return terminals().contains(app.toLower()) ? PasteChord::CtrlShiftV : PasteChord::ShiftInsert;
}

std::array<std::uint32_t, 3> chordKeysyms(PasteChord chord)
{
    if (chord == PasteChord::CtrlShiftV) {
        return {kControlL, kShiftL, kV};
    }
    return {kShiftL, kInsert, 0}; // 0: no third key
}

} // namespace paraphrase::app

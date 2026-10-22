#pragma once

#include "backends.h"

#include <memory>
#include <vector>

namespace paraphrase::app {

/// The backends one session offers, best first; the fallbacks are added after them.
struct SessionBackends
{
    std::vector<std::unique_ptr<CaptureBackend>> capture;
    std::vector<std::unique_ptr<InsertBackend>> insert;
    std::vector<ShortcutBackend*> shortcuts; ///< owned by the QObject passed in
};

/// X11: XGrabKey, the PRIMARY selection, XTest paste; the shortcut portal after the grab.
void addX11Backends(SessionBackends& out, QObject* owner);
/// Wayland: the GlobalShortcuts portal, data-control and the RemoteDesktop portal; the window
/// check comes from KWin, so typing is only offered on KDE Plasma.
void addWaylandBackends(SessionBackends& out, QObject* owner);

} // namespace paraphrase::app

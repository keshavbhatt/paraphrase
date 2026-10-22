#include "session_backends.h"

#include "kwin_window_tracker.h"
#include "paste_insert.h"
#include "portal_paste_keys.h"
#include "portal_shortcut.h"
#include "qt_clipboard_access.h"
#include "selection_captures.h"
#include "x11_backends.h"

#ifdef PARAPHRASE_HAVE_DATA_CONTROL
#include "data_control_client.h"
#endif

using namespace Qt::StringLiterals;

namespace paraphrase::app {

void addX11Backends(SessionBackends& out, QObject* owner)
{
    const auto tracker = std::make_shared<X11WindowTracker>();
    out.capture.push_back(std::make_unique<X11SelectionCapture>(tracker));
    out.insert.push_back(std::make_unique<PasteInsert>(QObject::tr("Paste with XTest"), tracker,
                                                       std::make_shared<QtClipboardAccess>(),
                                                       std::make_shared<XTestPasteKeys>()));
    out.shortcuts.push_back(new X11Shortcut(owner));
    out.shortcuts.push_back(new PortalShortcut(owner));
}

void addWaylandBackends(SessionBackends& out, QObject* owner)
{
    out.shortcuts.push_back(new PortalShortcut(owner));
#ifdef PARAPHRASE_HAVE_DATA_CONTROL
    const std::shared_ptr<DataControlClient> client = DataControlClient::connectToCompositor();
    if (client == nullptr) {
        return; // the compositor lacks data-control: the clipboard fallbacks remain
    }
    const auto tracker = std::make_shared<KWinWindowTracker>();
    out.capture.push_back(std::make_unique<DataControlCapture>(client, tracker));
    out.insert.push_back(std::make_unique<PasteInsert>(
        QObject::tr("Paste through the RemoteDesktop portal"), tracker,
        std::make_shared<DataControlClipboard>(client), std::make_shared<PortalPasteKeys>()));
#endif
}

} // namespace paraphrase::app

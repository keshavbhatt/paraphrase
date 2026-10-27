# Platform matrix: shortcut, capture, insert

What the popup can do on each desktop, which backend does it, and what the user does when a
backend is missing. Code: `app/src/integration/` (interfaces in `backends.h`, the order in
`integrations.cpp` and `session_backends.cpp`). The rules are PLAN.md section 6; the package
format limits come from DOCS/packaging.md.

Every backend may be missing (ADR-013): the main window works with all of them switched off
(`PARAPHRASE_NO_INTEGRATIONS=1`), and the last backend of each kind always works: read the
clipboard, leave the result on the clipboard, bind `paraphrase-app --popup` by hand.

## What works where

| | X11 (any desktop) | KDE Plasma, Wayland | GNOME, Wayland | wlroots (Sway, Hyprland), Wayland |
|---|---|---|---|---|
| Global shortcut | `X11Shortcut`: XGrabKey on the root window, every Num Lock and Caps Lock state | `PortalShortcut`: GlobalShortcuts portal; Change keys and Choose keys open the desktop's own window (ConfigureShortcuts, portal version 2), else the shortcut is asked for again | `PortalShortcut` on GNOME 48 and later (portal there; not tested here) | portal where the desktop's portal has it; else bind the command |
| Shortcut fallback | `CommandShortcut`: bind the command the app shows (`paraphrase-app --popup`; `paraphrase --popup` in the snap, `flatpak run com.ktechpit.paraphrase --popup` in the Flatpak, the AppImage's path) in the desktop's settings | same | same | same, in the compositor config |
| Read the highlighted text | `X11SelectionCapture`: the PRIMARY selection, no focus needed | `DataControlCapture`: `ext-data-control-v1`, no focus needed | not possible: GNOME has no data-control. `ClipboardCapture` reads the copied text, and since Wayland sends the clipboard only to the window with focus, the popup reads it again once it has focus (`needsFocus`, and on each clipboard change until an action runs) | `DataControlCapture` (wlroots has data-control) |
| Which window the text came from | `X11WindowTracker`: `_NET_ACTIVE_WINDOW`, WM_CLASS for the app name | `KWinWindowTracker`: a KWin script reports activations over D-Bus | not known | not known |
| Put the result back | `PasteInsert` with `XTestPasteKeys`: clipboard, the paste keys through XTEST; the result stays on the clipboard | `PasteInsert` with `PortalPasteKeys`: clipboard through data-control, the paste keys through the RemoteDesktop portal; the result stays on the clipboard | result on the clipboard, "Press Ctrl+V" (no window check, so nothing is typed) | same as GNOME until a window check exists |
| Insert fallback | `ClipboardInsert`: the result stays on the clipboard | same | same | same |

An X11 app on a Wayland session (Xwayland, `QT_QPA_PLATFORM=xcb`) gets no key grab and no
XTest: Xwayland drops XTest input and only grabs keys for X windows. `x11::isXwayland` makes
those backends step aside and the portals take over.

## A highlighted text that went stale

Some apps never update the highlighted text: a browser's canvas editor, or a select-all from the
keyboard. After a result was put in place of a text, the PRIMARY selection can still hold the
old text, and the next popup opened on it (owner report, 2026-10-26). Every capture backend now
skips a selection (or clipboard) that reads as the text just replaced, which is gone from the
app. After such a skip the clipboard counts only if it changed since the result went in; else
the popup asks the user to copy the new text with Ctrl+C.

## Never typing into the wrong window

Text is typed only into the window the text came from (PLAN 6). The window is recorded when
the text is captured; before typing, `PasteInsert` waits up to 600 ms for that window to be
active again (the popup hands focus back when it hides). If it is not, or the window is not
known (the clipboard fallback, GNOME, wlroots, a sandbox that blocks `org.kde.KWin`), the
result stays on the clipboard and the popup says "Press Ctrl+V". On KDE the window id is
KWin's `internalId` of the active window; titles are never read.

## The clipboard and the paste keys

The result is put on the clipboard, the paste keys are pressed, and the result stays there
(owner, 2026-10-26): a paste that lands nowhere, with no text field in focus, is one Ctrl+V
away instead of lost. The user's earlier clipboard is not put back; dabri tried that and
removed it, as it raced the target app still reading the clipboard. On Wayland the clipboard
is set through data-control, because a window that just lost focus may not set it.

The paste keys are Shift+Insert (2026-10-27, from dabri #105 and #106): Qt, GTK, browsers,
Electron, LibreOffice and Java apps paste the clipboard with it, and its keys are the same on
every keyboard layout, where Ctrl+V needs a key that makes "v", which a Russian or Greek
layout may not have. Terminals paste the selection with Shift+Insert, which is the text the
user selected, so a window whose class or desktop file names a known terminal (Konsole,
GNOME Terminal, Console, Ptyxis, kitty, Alacritty, foot, WezTerm, Ghostty, Tilix and others,
`paste_keys.cpp`) gets Ctrl+Shift+V. xterm keeps Shift+Insert, its only paste.

When the portal session has ended (the desktop restarted, the user revoked it), a new one is
started and the keys sent once more, but only when not one key went through, so nothing is
pasted twice.

## Permissions the user sees

- KDE, first shortcut: the desktop may show what the popup's shortcut is (Ctrl+Alt+Space by
  default) and lets the user change it later in System Settings, Shortcuts.
- KDE, first Replace: the RemoteDesktop portal asks once to allow keyboard input. The restore
  token is kept in the app's settings (`integration/remoteDesktopToken`), so it is not asked
  again until the user revokes it. Only a "no" in that dialog is kept
  (`integration/remoteDesktopRefused`); a portal error or no answer asks again next time.
- The tray: an app started at login may run before the panel's tray. It watches for the
  tray (`org.kde.StatusNotifierWatcher` and its host) and adds its icon when it turns up;
  Qt is asked only then, as it checks once per run and keeps a "no".
- The shortcut held down: GNOME repeats Activated and X11 repeats the key press while the
  keys are held, so a press within 600 ms of the last one is ignored.

- History (off by default): turning it on asks the desktop's keyring for the history's key,
  the Secret portal first, else the Secret Service (KWallet, GNOME Keyring). A locked wallet
  may ask for its password then. Without a keyring the switch stays off and says why. The
  keyring used is remembered (`history/secretKind`), so later runs ask the same one.

## Package formats

| | snap | Flatpak | AppImage, native |
|---|---|---|---|
| X11 key grab, XTest | `x11` plug | `--socket=fallback-x11`, X11 sessions only | works |
| GlobalShortcuts, RemoteDesktop portals | `desktop` plug | no extra permission | works |
| data-control (KDE) | plain Wayland socket: unverified | reported hidden by Flatpak's Wayland security context: the clipboard fallbacks apply | works |
| History key: Secret portal, else Secret Service | Secret portal via `desktop`; the Secret Service needs `password-manager-service` | Secret portal, no permission | Secret portal on KDE; the Secret Service elsewhere |
| KWin window tracker (`org.kde.KWin` on the session bus) | the script goes to the runtime folder, which KWin can read (the snap's /tmp is its own); `org.kde.KWin` on a strict snap with AppArmor: unverified (not enforced on the dev machine); without it nothing is typed | needs `--talk-name=org.kde.KWin` | works |

## Building and dependencies

Build: `libxcb` and `libxcb-xtest` (the KDE Qt SDK snap ships both), `wayland-client`,
`wayland-scanner` and `wayland-protocols` for the generated `ext-data-control-v1` code; when
they are missing the build leaves data-control out (`PARAPHRASE_HAVE_DATA_CONTROL`). On Arch:
`pacman -S libxcb wayland wayland-protocols`.

Runtime: the kf6-core24 content snap ships `libxcb`, `libxcb-xtest` and `libwayland-client`.

## Tests

| Test | Where it runs | What it covers |
|---|---|---|
| `tst_integrations` | ctest | ranking, the fallbacks |
| `tst_paste_insert` | ctest | the window check, the result kept on the clipboard, keys that fail, portal trigger strings |
| `tst_wayland_backends` | `scripts/desktop-test.sh` (a headless KWin on its own D-Bus session) | data-control between two clients, save and restore of every format, capture |
| `tst_x11_backends` | `scripts/desktop-test.sh` (that KWin's Xwayland) | the window tracker, stepping aside on Xwayland; the key grab and XTest paste need a real X server and skip there |
| `tst_live_session` | by hand, `PARAPHRASE_TEST_LIVE=1` on the user's KDE Wayland session | read-only: the KWin tracker, the portals, a shortcut session opened and closed, data-control connects |

`scripts/desktop-test.sh` needs `kwin_wayland`, `Xwayland` and `dbus-run-session` on the host
(pacman: `kwin xorg-xwayland dbus`); without them it skips.

## Not verified yet

- The RemoteDesktop portal paste and the GlobalShortcuts binding on a live session: both may
  show a dialog the first time, which needs a person. To check by hand on KDE: run the app,
  press the shortcut over a text field, choose an action, Replace; accept the dialog once.
- The X11 key grab and XTest paste on a real X11 session (no X11 session or Xvfb on the
  development laptop).
- GNOME and wlroots sessions.
- The Secret portal's app secret for an app that is not sandboxed, on GNOME (KDE answers it).
- Each package format (table above).

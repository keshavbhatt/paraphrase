# Packaging: snap, Flatpak and AppImage against the architecture

Question (owner, 2026-10-22): Paraphrase ships mainly as a snap and a Flatpak, maybe an
AppImage. Do these formats support the architecture in PLAN (daemon, popup, capture and
inject, model downloads, browser extension, input method)?

Short answer: **the core works in all three** (daemon, popup, model downloads, portals for
the shortcut and for typing the result back). **Six things must be decided before M1**
(section 3). **Two later features do not fit the sandboxed formats**: the browser
extension with sandboxed browsers (M4) and the input-method addon (M6) (section 4).

Status words: **verified** = tested end to end by us (Omnidict counts, same owner and
stack); **documented** = the format's or the protocol's own documentation says so;
**reported** = third-party reports, 2025-2026, not yet tested by us.

## 1. What each format is

| | snap (strict) | Flatpak | AppImage |
|---|---|---|---|
| Sandbox | yes, AppArmor + seccomp, access through "interfaces" | yes, bubblewrap, access through "permissions" and portals | none: runs as the user, like a native app |
| Store | Snap Store (Omnidict publishes there) | Flathub (Omnidict ready, not submitted) | none; a file the user downloads |
| Qt 6 | `kde-neon-6` extension (Omnidict) | `org.kde.Platform` runtime (Omnidict) | bundled into the file |
| Updates | automatic | automatic | manual unless we add AppImageUpdate |
| Store review | manual review for a D-Bus name and for files outside the snap | Flathub review of the manifest and permissions | none |

## 2. The architecture, piece by piece

| Piece (PLAN) | snap | Flatpak | AppImage |
|---|---|---|---|
| Daemon + popup + CLI in one package (D1) | works: several `apps:` in one snap | works: one app, extra commands via `flatpak run --command=` | works |
| Daemon on the session D-Bus (5.1) | works with a `dbus` slot naming the bus name; **the store must approve the name** (manual review, documented) | works **if the name starts with the app ID**: a Flatpak may own `<app-id>` and `<app-id>.*` without extra permission (documented); other names need `--own-name`, which Flathub reviewers question | works, no rules |
| Starting the daemon | the popup starts it, or a user daemon / autostart entry (snap `autostart:`; user daemons: verify) | the popup starts it; D-Bus activation through an exported service file for app-ID names; login autostart through the Background portal (documented; one report of it not firing) | the popup starts it; autostart by writing `~/.config/autostart` (breaks if the file moves) |
| Daemon idle exit and restart (2026-10-25) | the daemon exits after 30 quiet minutes; the app starts it again before its next call (`DaemonClient::keepRunning`, the same `paraphrase-daemon` from `PATH`, or a new private socket when the bus name is not owned). Verify in the snap: `paraphrase-daemon` on the snap's `PATH`, and no `daemon:` service in snapcraft.yaml (systemd would restart it against the idle exit) | same: `/app/bin/paraphrase-daemon` started inside the sandbox; the bus name starts with the app ID, so the proxy lets the app see whether it is owned. Verify in the Flatpak | works |
| Unix socket API (5.1, D7) | under `$XDG_RUNTIME_DIR/snap.paraphrase/`, readable by host processes | must be under `$XDG_RUNTIME_DIR/app/<app-id>/` to be shared with the host | anywhere |
| Model downloads (D8), 3 to 10 GB | network plug; store models in `$SNAP_USER_COMMON` (see section 3) | `--share=network`; models in `~/.var/app/<app-id>/data` | anywhere; `~/.local/share/paraphrase` |
| llama.cpp on the CPU | works; needs a portable build (section 3) | same | same |
| Vulkan GPU | `opengl` plug and the graphics content snap the KDE extension brings | `--device=dri` and the runtime's Mesa extension (documented) | host Vulkan loader and drivers; do not bundle drivers |
| Hardware probe (D9) | `/proc/meminfo`, `/proc/cpuinfo` readable; some `/sys` needs `hardware-observe` | `/proc` readable; most of `/sys` readable | everything |
| Global shortcut, Wayland | GlobalShortcuts portal through the `desktop` plug: **verified** (Omnidict 1.0.0 snap, KDE Plasma, 2026-10-21) | GlobalShortcuts portal, no extra permission: **verified** on KDE (Omnidict) | portal or native |
| Global shortcut, X11 | XGrabKey through the `x11` plug | XGrabKey with `--socket=fallback-x11` (X11 sessions only) | XGrabKey |
| Shortcut fallback "bind this command" | `snap run paraphrase.popup` or `paraphrase.popup` | `flatpak run com.ktechpit.paraphrase popup` | the AppImage path, which breaks if moved |
| Put the result back, Wayland | RemoteDesktop portal: KDE works (keysym support and remembering the permission improved through Plasma 6.6, reported); GNOME has the portal (reported); wlroots and Hyprland portals still adding it (reported) | same portal, same status | same, plus `wtype` where installed |
| Put the result back, X11 | XTest through the `x11` plug | XTest on X11 sessions | XTest |
| Read the selection without focus, Wayland | KDE exposes `ext-data-control-v1`; whether it reaches a snap's plain Wayland socket: **unverified** | **reported not to work**: Flatpak's Wayland security context hides data-control from sandboxed apps; GNOME has no data-control at all | works on KDE and wlroots (not GNOME) |
| Read the clipboard | works while our popup has focus (all formats; Wayland rule, not a sandbox rule) | same | same |
| Tray icon | StatusNotifierItem over D-Bus (KDE); GNOME needs an extension | needs `--talk-name=org.kde.StatusNotifierWatcher` | works |

The practical difference for users: in a **Flatpak on Wayland** the hotkey works on the
**copied** text (press Ctrl+C, then the shortcut), not the **selected** text. PLAN 6
already has this "clipboard flow"; the popup and setup must explain it per format.

## 3. Decisions needed before M1

1. **App ID and D-Bus name.** Use one reverse-DNS ID everywhere, like Omnidict's
   `com.ktechpit.omnidict`: app ID `com.ktechpit.paraphrase`, daemon bus name
   `com.ktechpit.paraphrase.Daemon` (object path `/com/ktechpit/paraphrase/Daemon`, interface
   `com.ktechpit.paraphrase.Daemon1` for versioning). PLAN 5.1's `com.ktechpit.Paraphrase1`
   would need `--own-name` in the Flatpak. The snap still needs a store declaration for the
   name: request it with the first upload. The app owns a second name,
   `com.ktechpit.paraphrase.App` (object `/com/ktechpit/paraphrase/App`): one running copy,
   so `paraphrase-app --popup` bound to a desktop shortcut reaches it. Its methods
   `ShowPopup(s activation_token)` and `ShowWindow(s activation_token)` take the token the
   launcher gave the second copy (`XDG_ACTIVATION_TOKEN`, may be empty): on Wayland a window in
   the tray app comes to the front only with it. It starts with the app ID, so the Flatpak
   needs nothing extra; the snap declares it with the daemon's.
2. **Portable CPU build.** The M0 build uses `GGML_NATIVE=ON`, tuned to this laptop's CPU; a
   package built that way crashes on older CPUs. Packages build llama.cpp with
   `GGML_NATIVE=OFF`, `BUILD_SHARED_LIBS=ON`, `GGML_BACKEND_DL=ON` and `GGML_CPU_ALL_VARIANTS=ON`:
   one library per CPU generation, the best one picked at run time (Debian's llama.cpp
   packages do this, documented). Re-run the M0 bench on the portable build in M1; it
   should match the native one on the same CPU, but that is not measured yet.
3. **Where models live.** Each format has its own data folder (section 2). In a snap, use
   `$SNAP_USER_COMMON`, never the per-revision `$SNAP_USER_DATA`: snapd copies the
   per-revision folder on every refresh, which would duplicate gigabytes of models. snapd
   also snapshots a snap's data when it is removed; the uninstall text tells users about
   `snap remove --purge`. The UI shows the real path (the Settings mock currently says
   `~/.local/share/paraphrase/models`, true only for native and AppImage).
4. **How the daemon starts.** The baseline that works in all three formats: the popup (or
   the CLI) starts the daemon when it is not on the bus, and the daemon exits after a quiet
   period. Login autostart is an option in Settings, through the Background portal
   (Flatpak), the snap's `autostart` key, or `~/.config/autostart` (AppImage).
5. **The Unix socket.** Keep D-Bus as the main API. The socket, needed only by the browser's
   native messaging host (M4), lives in the folder each format shares with the host
   (section 2).
6. **One package, three commands.** Daemon, popup client and CLI ship together in each
   format; a separate daemon package would need cross-sandbox D-Bus rules for no gain.

## 4. What does not fit the sandboxed formats

**Browser extension (M4).** The browser starts our native messaging host from a manifest
in its own folder (`~/.mozilla/native-messaging-hosts/`, `~/.config/google-chrome/...`).
- Our app sandboxed, browser native (deb, rpm): works, but the app must be allowed to write
  the manifest there: Flatpak `--filesystem=~/.mozilla/native-messaging-hosts:create` (and
  the Chrome and Chromium folders; Bitwarden's Flatpak does this), snap `personal-files`
  (store review). The manifest points at `flatpak run ...` or `/snap/bin/...`.
- Browser sandboxed (Flatpak or snap Firefox, Flatpak Chrome): the browser cannot start
  host programs. A portal for this has been proposed since 2021 (xdg-desktop-portal #655,
  #705); an `xdg-native-messaging-proxy` with client support in Firefox 157+ was reported
  in October 2026, opt-in per app. Ubuntu's default Firefox is a snap, so this matters.
  **Plan M4 for native browsers first and re-check the proxy then.**

**Input-method addon (M6).** Fcitx5 and IBus load addons from the host's own folders; an
addon cannot live inside our Flatpak, snap or AppImage. Fcitx5's own Flatpak takes addons as
Flatpak extensions, but most users run their distro's Fcitx5. M6 therefore needs **distro
packages** (deb, rpm, AUR) for a thin addon that talks to the daemon over D-Bus; the daemon
itself can stay in the snap or Flatpak. PLAN already designs the IME as "just another
client" of the daemon API, which is what makes this split possible.

## 5. AppImage in short

No sandbox, so every native path works (XTest, data-control, the browser manifest folders,
`~/.config/autostart`). The costs: bundle Qt and llama.cpp's CPU variants and build on an old
enough glibc; no automatic updates; no D-Bus activation or desktop integration unless the
user installs AppImageLauncher or we write the files ourselves; paths break if the file
moves. Good as a portable "try it" build, not the main channel.

## 6. To verify in M1 (each is a short test, recorded here when done)

- [ ] snap: RemoteDesktop portal typing on KDE and GNOME Wayland.
- [ ] snap: whether `ext-data-control-v1` reaches a strict snap on KDE (selection without focus).
- [ ] Flatpak: confirm data-control is hidden on KDE (then the clipboard flow is the default there).
- [ ] snap: user daemons (`daemon-scope: user`) or `autostart`, and D-Bus activation for snap apps.
- [ ] Flatpak: Background portal autostart on KDE and GNOME.
- [ ] Portable llama.cpp build (`GGML_CPU_ALL_VARIANTS`) speed against the M0 numbers.
- [ ] GNOME: RemoteDesktop portal typing from a Flatpak.

## Sources

- Omnidict: `packaging/README.md`, `snap/snapcraft.yaml`, `packaging/flatpak/com.ktechpit.omnidict.yaml`, `DOCS/quick-lookup.md` (portal shortcut verified in the snap, 2026-10-21).
- Snap D-Bus interface: https://snapcraft.io/docs/reference/interfaces/dbus-interface/ and the forum thread on well-known names, https://forum.snapcraft.io/t/the-dbus-interface/2038
- RemoteDesktop portal: https://flatpak.github.io/xdg-desktop-portal/docs/doc-org.freedesktop.portal.RemoteDesktop.html ; KDE keysym bug https://www.mail-archive.com/kde-bugs-dist@kde.org/msg694602.html ; Hyprland portal PR https://github.com/hyprwm/xdg-desktop-portal-hyprland/pull/402 ; wlroots portal PR https://github.com/emersion/xdg-desktop-portal-wlr/pull/325
- Native messaging and sandboxed browsers: https://github.com/flatpak/xdg-desktop-portal/issues/655 , https://github.com/flatpak/xdg-desktop-portal/pull/705 , https://github.com/bopp-os/pkgbuilds/pull/28 , https://github.com/bitwarden/clients/issues/23554
- Data-control and sandboxes: https://github.com/storytold/photocraft/pull/5279 , https://github.com/mariinkys/clockode/issues/22
- llama.cpp portable builds: https://www.mintlify.com/ggml-org/llama.cpp/development/build-options , https://github.com/abetlen/llama-cpp-python/issues/2069 , https://dyn.manpages.debian.org/testing/llama.cpp-tools/llama-cli.1.en.html
- Fcitx5 in Flatpak: https://fcitx-im.org/wiki/Install_Fcitx_5
- Background portal: https://discourse.flathub.org/t/autostart-flatpak-application/4704 , https://invent.kde.org/office/crow-translate/-/issues/428

#!/usr/bin/env bash
# Runs the desktop backend tests (app/tests/tst_wayland_backends, tst_x11_backends) against
# a headless KWin with Xwayland, started here on its own D-Bus session, so the tests never
# touch the user's desktop: no clipboard change, no key press and no window outside it.
#
#   scripts/desktop-test.sh             build first with make build
#
# Needs kwin and xorg-xwayland on the host (pacman: kwin xorg-xwayland dbus).
set -euo pipefail

DIR="$(cd "$(dirname "$0")/.." && pwd)"
SOCKET="paraphrase-test-$$"
RUNTIME="${XDG_RUNTIME_DIR:?XDG_RUNTIME_DIR is not set}"

command -v kwin_wayland >/dev/null || { echo "kwin_wayland not found: skipping the desktop tests"; exit 0; }
command -v dbus-run-session >/dev/null || { echo "dbus-run-session not found: skipping"; exit 0; }

LOG="$(mktemp)"
xsockets() { find /tmp/.X11-unix -maxdepth 1 -name 'X*' -printf '%f\n' 2>/dev/null | sort; }
BEFORE="$(xsockets)"
# A clean environment for the host's KWin: nothing of the snap runtime, no parent display.
# Its own process group, so stopping it stops KWin, Xwayland and the bus together.
env -u WAYLAND_DISPLAY -u DISPLAY -u LD_LIBRARY_PATH -u QT_PLUGIN_PATH -u QML2_IMPORT_PATH \
    setsid dbus-run-session -- kwin_wayland --virtual --socket "$SOCKET" --xwayland \
    --width 1280 --height 800 >"$LOG" 2>&1 &
KWIN=$!
trap 'kill -TERM -- "-$KWIN" 2>/dev/null || true; wait "$KWIN" 2>/dev/null || true; rm -f "$LOG"' EXIT

# KWin picks the X display itself: the socket that was not there before.
XDISPLAY=""
for _ in $(seq 1 100); do
    NEW="$(comm -13 <(echo "$BEFORE") <(xsockets) | head -1)"
    if [ -S "$RUNTIME/$SOCKET" ] && [ -n "$NEW" ]; then
        XDISPLAY=":${NEW#X}"
        break
    fi
    sleep 0.1
done
if [ -z "$XDISPLAY" ]; then
    echo "the headless KWin did not start:" >&2
    cat "$LOG" >&2
    exit 1
fi

status=0
run() {
    PARAPHRASE_TEST_NESTED=1 WAYLAND_DISPLAY="$SOCKET" DISPLAY="$XDISPLAY" PARAPHRASE_RUN="app/tests/$1" \
        QT_QPA_PLATFORM="$2" "$DIR/scripts/dev-run.sh" || status=1
}
run tst_wayland_backends wayland
run tst_x11_backends xcb
exit "$status"

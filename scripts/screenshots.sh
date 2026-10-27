#!/usr/bin/env bash
# Makes the README's screenshots and banner, as Glate does: real actions with the real models
# (tst_screenshots grabs each window in the dark theme), then scripts/screenshots.py frames them
# with a headline into screenshots/.
#
#   scripts/screenshots.sh     # needs a dev build (make build) and E2B and E4B in models/
#
# It runs on a private session bus with a scratch profile under the build folder, so it never
# reaches an installed Paraphrase, its daemon, settings or history.
set -euo pipefail

DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${PARAPHRASE_BUILD_DIR:-$DIR/build/dev}"
PROFILE="$BUILD/screenshot-profile"
RAW="$BUILD/screenshots-raw"
MODELS="${PARAPHRASE_MODELS_DIR:-$DIR/models}"

[ -x "$BUILD/app/tests/tst_screenshots" ] || { echo "no dev build in $BUILD: run make build" >&2; exit 1; }
for m in gemma-4-e2b-qat-q4_0 gemma-4-e4b-qat-q4_0; do
    [ -d "$MODELS/$m" ] || { echo "model $m missing in $MODELS" >&2; exit 1; }
done

rm -rf "$PROFILE" "$RAW"
mkdir -p "$PROFILE/config" "$PROFILE/data" "$PROFILE/cache" "$PROFILE/runtime" "$RAW"
chmod 700 "$PROFILE/runtime"

dbus-run-session -- env \
    XDG_CONFIG_HOME="$PROFILE/config" XDG_DATA_HOME="$PROFILE/data" \
    XDG_CACHE_HOME="$PROFILE/cache" XDG_RUNTIME_DIR="$PROFILE/runtime" \
    QT_QPA_PLATFORM=offscreen PARAPHRASE_THEME=dark \
    PARAPHRASE_MODELS_DIR="$MODELS" PARAPHRASE_DAEMON="$BUILD/daemon/src/paraphrase-daemon" \
    PARAPHRASE_SHOTS_DIR="$RAW" PARAPHRASE_RUN=app/tests/tst_screenshots \
    "$DIR/scripts/dev-run.sh" >"$RAW/run.log" 2>&1 || {
    echo "the screenshot run failed, see $RAW/run.log" >&2
    exit 1
}

# The daemon this run started outlives the bus for a moment: stop it, and only it.
for pid in $(pgrep -x paraphrase-daem || true); do
    if tr '\0' '\n' <"/proc/$pid/environ" 2>/dev/null | grep -qx "XDG_RUNTIME_DIR=$PROFILE/runtime"; then
        kill "$pid" || true
    fi
done

uv run --quiet --with pillow python3 "$DIR/scripts/screenshots.py" "$RAW"

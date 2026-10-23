#!/usr/bin/env bash
# Runs the dev build against the kf6-core24 runtime content snap, the very libraries the
# shipped snap uses. Host fonts, display and D-Bus work directly.
#
#   scripts/dev-run.sh                       # start paraphrase-daemon
#   scripts/dev-run.sh --ctest [args]        # run the test suite (env baked in by CMake)
#   scripts/dev-run.sh -- --some-flag        # pass arguments to paraphrase-daemon
#   PARAPHRASE_RUN=cli/paraphrase scripts/dev-run.sh -- proofread   # another program
set -euo pipefail

DIR="$(cd "$(dirname "$0")/.." && pwd)"
BUILD="${PARAPHRASE_BUILD_DIR:-$DIR/build/dev}"

# shellcheck source-path=SCRIPTDIR source=snap-runtime-env.sh
source "$DIR/scripts/snap-runtime-env.sh"

[ -d "$PARAPHRASE_RT/usr/lib/x86_64-linux-gnu" ] || {
    echo "kf6-core24 runtime snap missing: sudo snap install kf6-core24" >&2; exit 1; }

if [ "${1:-}" = "--ctest" ]; then
    # Tests run where the build ran: core24, the shipped snap's base (scripts/core24-run.sh).
    if ! paraphrase_in_core24; then
        exec "$DIR/scripts/core24-run.sh" scripts/dev-run.sh "$@"
    fi
    # ctest from the cmake snap, like the configure step. The tests get the runtime env
    # through their CTest ENVIRONMENT property (daemon/tests/CMakeLists.txt).
    cd "$BUILD"
    exec "${PARAPHRASE_CTEST:-/snap/cmake/current/bin/ctest}" --output-on-failure "${@:2}"
fi

paraphrase_prepare_runtime_farm "$BUILD"
paraphrase_export_runtime_env "$BUILD"
export QT_LOGGING_RULES="${QT_LOGGING_RULES:-paraphrase.*.debug=true}"

[ "${1:-}" = "--" ] && shift

# The desktop portal names a host app only after it finds the app's .desktop file; without
# one it files the app under whatever launched it (the global shortcut showed up as
# Konsole's). Install a dev copy whose Exec runs this script.
if [[ "${PARAPHRASE_RUN:-}" == *paraphrase-app ]]; then
    apps="${XDG_DATA_HOME:-$HOME/.local/share}/applications"
    desktop="$apps/com.ktechpit.paraphrase.desktop"
    exec_line="env PARAPHRASE_MODELS_DIR=${PARAPHRASE_MODELS_DIR:-$DIR/models} PARAPHRASE_RUN=${PARAPHRASE_RUN} $DIR/scripts/dev-run.sh --"
    wanted="$(sed -e "s|^Name=Paraphrase$|Name=Paraphrase (dev)|" \
        -e "s|^Exec=paraphrase-app|Exec=$exec_line|" "$DIR/app/data/com.ktechpit.paraphrase.desktop")"
    if [ "$(cat "$desktop" 2>/dev/null)" != "$wanted" ]; then
        mkdir -p "$apps"
        printf '%s\n' "$wanted" >"$desktop"
    fi
fi
cd "$DIR"
exec "$BUILD/${PARAPHRASE_RUN:-daemon/src/paraphrase-daemon}" "$@"

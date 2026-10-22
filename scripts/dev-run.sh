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
    # ctest is a host binary: keep the host environment. The tests themselves get the
    # runtime env through their CTest ENVIRONMENT property (daemon/tests/CMakeLists.txt).
    cd "$BUILD"
    exec ctest --output-on-failure "${@:2}"
fi

paraphrase_prepare_runtime_farm "$BUILD"
paraphrase_export_runtime_env "$BUILD"
export QT_LOGGING_RULES="${QT_LOGGING_RULES:-paraphrase.*.debug=true}"

[ "${1:-}" = "--" ] && shift
cd "$DIR"
exec "$BUILD/${PARAPHRASE_RUN:-daemon/src/paraphrase-daemon}" "$@"

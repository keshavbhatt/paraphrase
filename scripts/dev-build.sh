#!/usr/bin/env bash
# Builds the daemon (and from M2 the app) on the host against the KDE Qt 6.11 snap SDK
# (kde-qt6-core24-sdk), the same Qt the shipped snap uses from the kf6-core24 content snap
# and that Flathub's KDE runtime provides. Dev/prod parity: what compiles here is what ships.
#
#   scripts/dev-build.sh                                  # RelWithDebInfo into build/dev
#   PARAPHRASE_BUILD_TYPE=Debug scripts/dev-build.sh
#   PARAPHRASE_CMAKE_ARGS="-DPARAPHRASE_WERROR=ON" scripts/dev-build.sh
#   scripts/dev-build.sh --tests                          # also run ctest afterwards
set -euo pipefail

DIR="$(cd "$(dirname "$0")/.." && pwd)"
SDK=/snap/kde-qt6-core24-sdk/current
BUILD="${PARAPHRASE_BUILD_DIR:-$DIR/build/dev}"

# shellcheck source-path=SCRIPTDIR source=snap-runtime-env.sh
source "$DIR/scripts/snap-runtime-env.sh"

if [ ! -d "$SDK/usr/lib/x86_64-linux-gnu/cmake/Qt6" ]; then
    echo "Qt SDK snap missing. Install with:" >&2
    echo "  sudo snap install kde-qt6-core24-sdk" >&2
    exit 1
fi
if [ ! -d "$PARAPHRASE_RT/usr/lib/x86_64-linux-gnu" ]; then
    echo "kf6-core24 runtime snap missing (needed to run and test). Install with:" >&2
    echo "  sudo snap install kf6-core24" >&2
    exit 1
fi

# No LD_LIBRARY_PATH here: the SDK's build tools (moc, rcc) run as they are, and exporting
# the SDK's library folder makes the host's cmake load the SDK's libarchive, which fails
# on a newer host (libnettle.so.8 missing on Arch, 2026-10-22).

# Extra CMake arguments: PARAPHRASE_CMAKE_ARGS="-DPARAPHRASE_WERROR=ON -DFOO=bar"
read -r -a EXTRA_ARGS <<< "${PARAPHRASE_CMAKE_ARGS:-}"

paraphrase_prepare_runtime_farm "$BUILD"

cmake -S "$DIR" -B "$BUILD" -G Ninja \
    -DCMAKE_BUILD_TYPE="${PARAPHRASE_BUILD_TYPE:-RelWithDebInfo}" \
    -DCMAKE_PREFIX_PATH="$SDK/usr" \
    -DCMAKE_LIBRARY_PATH="$SDK/usr/lib/x86_64-linux-gnu" \
    -DQt6_DIR="$SDK/usr/lib/x86_64-linux-gnu/cmake/Qt6" \
    -DCMAKE_EXE_LINKER_FLAGS="-Wl,-rpath-link,$SDK/usr/lib/x86_64-linux-gnu -Wl,--allow-shlib-undefined" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DPARAPHRASE_SNAP_RUNTIME="$PARAPHRASE_RT" \
    -DPARAPHRASE_SNAP_RUNTIME_LIB_PATH="$(paraphrase_runtime_lib_path "$BUILD")" \
    "${EXTRA_ARGS[@]}"
cmake --build "$BUILD" -j"$(nproc)"

# clangd / IDEs look for compile_commands.json at the repo root.
ln -sf "$BUILD/compile_commands.json" "$DIR/compile_commands.json"

if [ "${1:-}" = "--tests" ]; then
    "$DIR/scripts/dev-run.sh" --ctest
fi

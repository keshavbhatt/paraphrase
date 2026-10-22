#!/usr/bin/env bash
# Builds the daemon (and from M2 the app) exactly as the snap pipeline does (owner,
# 2026-10-22): in core24 (Ubuntu 24.04) with its GCC 13, the cmake snap and the KDE Qt 6.11
# snap SDK, the Qt the shipped snap uses from the kf6-core24 content snap. Outside core24
# (an Arch laptop, say) the script runs itself in that environment through
# scripts/core24-run.sh; on Ubuntu 24.04 (CI) it builds directly.
#
#   scripts/dev-build.sh                                  # RelWithDebInfo into build/dev
#   PARAPHRASE_BUILD_TYPE=Debug scripts/dev-build.sh
#   PARAPHRASE_CMAKE_ARGS="-DPARAPHRASE_WERROR=ON" scripts/dev-build.sh
#   scripts/dev-build.sh --tests                          # also run ctest afterwards
set -euo pipefail

DIR="$(cd "$(dirname "$0")/.." && pwd)"
SDK=/snap/kde-qt6-core24-sdk/current
BUILD="${PARAPHRASE_BUILD_DIR:-$DIR/build/dev}"
# The cmake snap, the one snapcraft installs as a build snap (as Omnidict's snapcraft.yaml),
# never the distro's cmake. Its binary is called directly: /snap/bin wrappers need snapd,
# which the build container does not have.
CMAKE="${PARAPHRASE_CMAKE:-/snap/cmake/current/bin/cmake}"

# shellcheck source-path=SCRIPTDIR source=snap-runtime-env.sh
source "$DIR/scripts/snap-runtime-env.sh"

if ! paraphrase_in_core24; then
    exec "$DIR/scripts/core24-run.sh" scripts/dev-build.sh "$@"
fi

if [ ! -d "$SDK/usr/lib/x86_64-linux-gnu/cmake/Qt6" ]; then
    echo "Qt SDK snap missing. Install with:" >&2
    echo "  sudo snap install kde-qt6-core24-sdk" >&2
    exit 1
fi
if [ ! -x "$CMAKE" ]; then
    echo "cmake snap missing (the snap build uses it). Install on the host with:" >&2
    echo "  sudo snap install cmake --classic" >&2
    exit 1
fi
if [ ! -d "$PARAPHRASE_RT/usr/lib/x86_64-linux-gnu" ]; then
    echo "kf6-core24 runtime snap missing (needed to run and test). Install with:" >&2
    echo "  sudo snap install kf6-core24" >&2
    exit 1
fi

# The SDK's own build tools (moc, rcc, ...) need the SDK's libraries, some of which sit in
# the libproxy subdirectory. Only inside core24 (this point is never reached elsewhere): on
# a host the SDK's libraries break the host's tools, and without them the SDK's moc quietly
# loads the host's Qt instead (found on Arch, 2026-10-22).
export LD_LIBRARY_PATH="$SDK/usr/lib/x86_64-linux-gnu:$SDK/usr/lib/x86_64-linux-gnu/libproxy${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# Extra CMake arguments: PARAPHRASE_CMAKE_ARGS="-DPARAPHRASE_WERROR=ON -DFOO=bar"
read -r -a EXTRA_ARGS <<< "${PARAPHRASE_CMAKE_ARGS:-}"

paraphrase_prepare_runtime_farm "$BUILD"

"$CMAKE" -S "$DIR" -B "$BUILD" -G Ninja \
    -DCMAKE_BUILD_TYPE="${PARAPHRASE_BUILD_TYPE:-RelWithDebInfo}" \
    -DCMAKE_PREFIX_PATH="$SDK/usr" \
    -DCMAKE_LIBRARY_PATH="$SDK/usr/lib/x86_64-linux-gnu" \
    -DQt6_DIR="$SDK/usr/lib/x86_64-linux-gnu/cmake/Qt6" \
    -DCMAKE_EXE_LINKER_FLAGS="-Wl,-rpath-link,$SDK/usr/lib/x86_64-linux-gnu -Wl,--allow-shlib-undefined" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
    -DPARAPHRASE_SNAP_RUNTIME="$PARAPHRASE_RT" \
    -DPARAPHRASE_SNAP_RUNTIME_LIB_PATH="$(paraphrase_runtime_lib_path "$BUILD")" \
    "${EXTRA_ARGS[@]}"
"$CMAKE" --build "$BUILD" -j"$(nproc)"

# clangd / IDEs look for compile_commands.json at the repo root.
ln -sf "$BUILD/compile_commands.json" "$DIR/compile_commands.json"

if [ "${1:-}" = "--tests" ]; then
    "$DIR/scripts/dev-run.sh" --ctest
fi

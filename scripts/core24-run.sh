#!/usr/bin/env bash
# Runs a command in the snap's build environment (core24 = Ubuntu 24.04, GCC 13) so local
# builds and tests match the automated pipelines (owner, 2026-10-22). The image is built
# from packaging/core24/Dockerfile and rebuilt only when that file changes. The
# repository is mounted at its own path and the host's snaps (KDE Qt SDK, kf6-core24
# runtime, cmake) read-only at /snap, so paths in compile_commands.json stay valid.
#
#   scripts/core24-run.sh scripts/dev-build.sh --tests
set -euo pipefail

DIR="$(cd "$(dirname "$0")/.." && pwd)"
DOCKERFILE="$DIR/packaging/core24/Dockerfile"
TAG="paraphrase-build-core24:$(sha256sum "$DOCKERFILE" | cut -c1-12)"
SNAPS="$(readlink -f /snap)"

command -v docker >/dev/null || {
    echo "docker is needed to build in the snap's environment (Ubuntu 24.04)." >&2; exit 1; }

if ! docker image inspect "$TAG" >/dev/null 2>&1; then
    echo "building the core24 build image $TAG" >&2
    docker build -q -t "$TAG" -f "$DOCKERFILE" "$DIR/packaging/core24" >/dev/null
fi

tty=()
[ -t 1 ] && tty=(-t)
exec docker run --rm "${tty[@]}" \
    --user "$(id -u):$(id -g)" \
    --volume "$SNAPS:/snap:ro" \
    --volume "$DIR:$DIR" \
    --workdir "$DIR" \
    --env HOME=/tmp \
    --env GIT_CONFIG_COUNT=1 --env GIT_CONFIG_KEY_0=safe.directory --env GIT_CONFIG_VALUE_0='*' \
    --env PARAPHRASE_BUILD_DIR --env PARAPHRASE_BUILD_TYPE --env PARAPHRASE_CMAKE_ARGS \
    "$TAG" "$@"

#!/usr/bin/env bash
# clang-format and clang-tidy as the pipeline runs them: in core24 (Ubuntu 24.04, GCC 13
# headers) with the versions CI pins, so a host with other compiler headers cannot change the
# result (owner: local runs must match the automated pipelines).
#
#   scripts/lint-cxx.sh          # check formatting, then clang-tidy every translation unit
#   scripts/lint-cxx.sh --fix    # reformat in place
set -euo pipefail

DIR="$(cd "$(dirname "$0")/.." && pwd)"
# shellcheck source-path=SCRIPTDIR source=snap-runtime-env.sh
source "$DIR/scripts/snap-runtime-env.sh"
if ! paraphrase_in_core24; then
    exec "$DIR/scripts/core24-run.sh" scripts/lint-cxx.sh "$@"
fi

cd "$DIR"
BUILD="${PARAPHRASE_BUILD_DIR:-$DIR/build/dev}"
mapfile -t sources < <(find daemon client cli app -name '*.cpp' -o -name '*.h' | sort)
mapfile -t units < <(find daemon client cli app -name '*.cpp' | sort)

if [ "${1:-}" = "--fix" ]; then
    clang-format -i "${sources[@]}"
    exit 0
fi
clang-format --dry-run --Werror "${sources[@]}"
# One clang-tidy per unit, in parallel; xargs fails when any of them does.
printf '%s\n' "${units[@]}" \
    | xargs -P "$(nproc)" -n 1 clang-tidy -p "$BUILD" --quiet --warnings-as-errors='*'

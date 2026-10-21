#!/usr/bin/env bash
# Builds the pinned llama.cpp submodule twice, for the M0 benchmarks:
#   build/llama-cpu      CPU only (reference machine A, and the CPU fallback)
#   build/llama-vulkan   CPU + Vulkan (reference machine B)
# Only the tools the eval harness uses are built. HTTPS (OpenSSL) is off: models are
# fetched by scripts/fetch_model.py, never by llama.cpp.
#
# Usage: scripts/build-llama.sh [cpu|vulkan|all]   (default: all)
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
src="$root/third_party/llama.cpp"
which="${1:-all}"
jobs="$(nproc)"

if [[ ! -f "$src/CMakeLists.txt" ]]; then
    echo "llama.cpp submodule missing: run 'git submodule update --init'" >&2
    exit 1
fi

build() {
    local name="$1"
    shift
    local dir="$root/build/llama-$name"
    cmake -S "$src" -B "$dir" -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=OFF \
        -DGGML_NATIVE=ON \
        -DLLAMA_OPENSSL=OFF \
        -DLLAMA_BUILD_TESTS=OFF \
        -DLLAMA_BUILD_EXAMPLES=OFF \
        -DLLAMA_BUILD_SERVER=ON \
        "$@"
    cmake --build "$dir" -j "$jobs" --target llama-server llama-cli llama-bench
    echo "built $dir/bin"
}

case "$which" in
    cpu) build cpu -DGGML_VULKAN=OFF ;;
    vulkan) build vulkan -DGGML_VULKAN=ON ;;
    all)
        build cpu -DGGML_VULKAN=OFF
        build vulkan -DGGML_VULKAN=ON
        ;;
    *)
        echo "usage: $0 [cpu|vulkan|all]" >&2
        exit 2
        ;;
esac

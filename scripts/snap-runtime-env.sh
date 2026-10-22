#!/usr/bin/env bash
# Sourced by dev-build.sh / dev-run.sh. Provides:
#   PARAPHRASE_RT                runtime content snap root (kf6-core24)
#   paraphrase_prepare_runtime_farm <build-dir>
#   paraphrase_runtime_lib_path     <build-dir>
#   paraphrase_export_runtime_env   <build-dir>
#
# The dev build links against the kde-qt6-core24-sdk snap and runs against the
# kf6-core24 content snap, the exact libraries the shipped snap uses (as Omnidict).
# Host glibc stays authoritative (the content snap carries no libc).

PARAPHRASE_RT=/snap/kf6-core24/current

# True inside the snap's build environment (core24 = Ubuntu 24.04): the CI runner, a core24
# machine, or the container of scripts/core24-run.sh. Builds and tests run only there.
paraphrase_in_core24() (
    # shellcheck disable=SC1091
    . /etc/os-release 2>/dev/null || exit 1
    [ "${ID:-}" = ubuntu ] && [ "${VERSION_ID:-}" = "24.04" ]
)
PARAPHRASE_CORE24_BASE=/snap/core24/current/usr/lib/x86_64-linux-gnu

# Some runtime-snap libs depend on Ubuntu base libraries the host may not
# have (libselinux, liblerc, ...). Farm them from the core24 base snap,
# EXCLUDING the glibc family: the host's newer glibc must stay
# authoritative or the loader chokes on GLIBC_PRIVATE symbols.
paraphrase_prepare_runtime_farm() {
    local build="$1"
    local farm="$build/core24-libs"
    mkdir -p "$farm"
    local f b
    for f in "$PARAPHRASE_CORE24_BASE"/*.so*; do
        b="$(basename "$f")"
        case "$b" in
            libc.so*|libm.so*|libmvec.so*|libpthread.so*|libdl.so*|librt.so*|\
            ld-linux*|libresolv.so*|libnsl.so*|libnss_*|libanl.so*|libutil.so*|\
            libBrokenLocale.so*|libthread_db.so*) continue ;;
        esac
        ln -sf "$f" "$farm/$b"
    done
}

paraphrase_runtime_lib_path() {
    local build="$1"
    local rt="$PARAPHRASE_RT/usr/lib/x86_64-linux-gnu"
    echo "$rt:$rt/libproxy:$build/core24-libs"
}

paraphrase_export_runtime_env() {
    local build="$1"
    local lib_path
    lib_path="$(paraphrase_runtime_lib_path "$build")"
    export LD_LIBRARY_PATH="$lib_path${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export QT_PLUGIN_PATH="$PARAPHRASE_RT/usr/lib/x86_64-linux-gnu/qt6/plugins"
    # This Qt build logs to journald when stderr is not a TTY; dev runs want plain stderr.
    export QT_FORCE_STDERR_LOGGING=1
}

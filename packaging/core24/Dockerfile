# The snap's build environment: core24 is Ubuntu 24.04, so this image has the compiler and
# system libraries snapcraft builds with (GCC 13). Qt, cmake and the runtime come from the
# host's snaps, mounted read-only at /snap (scripts/core24-run.sh). GLib, D-Bus and Duktape
# are what the SDK's own tools (moc, rcc) load from the base system; snapcraft's build image
# has them.
FROM ubuntu:24.04@sha256:68ee22004a9109b44973313b02ff7b1de250f1de0f5ecb628d73f579aaff33ac

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential git ca-certificates \
        libglib2.0-0t64 libdbus-1-3 libduktape207 \
    && rm -rf /var/lib/apt/lists/*

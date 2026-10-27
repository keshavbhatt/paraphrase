# The snap's build environment: core24 is Ubuntu 24.04, so this image has the compiler and
# system libraries snapcraft builds with (GCC 13). Qt, cmake and the runtime come from the
# host's snaps, mounted read-only at /snap (scripts/core24-run.sh). GLib, D-Bus and Duktape
# are what the SDK's own tools (moc, rcc) load from the base system; snapcraft's build image
# has them. dbus-daemon gives a test its own session bus (tst_integrations). clang-format and clang-tidy are the versions CI pins (.github/workflows/ci.yml),
# so local lint reads the same headers as the pipeline (scripts/lint-cxx.sh).
FROM ubuntu:24.04@sha256:68ee22004a9109b44973313b02ff7b1de250f1de0f5ecb628d73f579aaff33ac

RUN apt-get update \
    && apt-get install -y --no-install-recommends build-essential git ca-certificates \
        libglib2.0-0t64 libdbus-1-3 libduktape207 dbus-daemon python3-venv \
    && rm -rf /var/lib/apt/lists/*
RUN python3 -m venv /opt/lint \
    && /opt/lint/bin/pip install --no-cache-dir clang-format==23.1.1 clang-tidy==23.1.3
ENV PATH=/opt/lint/bin:$PATH

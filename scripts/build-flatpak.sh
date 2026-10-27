#!/usr/bin/env bash
# Builds the Flatpak from this checkout and installs it for the current user, to test it before
# it goes to Flathub (DOCS/packaging.md, 5b). Needs flatpak, org.flatpak.Builder and the KDE
# 6.11 runtime and SDK from Flathub:
#
#   flatpak install --user flathub org.flatpak.Builder org.kde.Platform//6.11 org.kde.Sdk//6.11
#   scripts/build-flatpak.sh            # lint, build, install
#   scripts/build-flatpak.sh --lint     # lint the manifest and the metainfo only
#
# Run it: flatpak run com.ktechpit.paraphrase   (the CLI: --command=paraphrase)
# The build state stays in build/flatpak; models download into ~/.var/app/com.ktechpit.paraphrase.
set -euo pipefail

DIR="$(cd "$(dirname "$0")/.." && pwd)"
MANIFEST="$DIR/packaging/flatpak/com.ktechpit.paraphrase.yaml"
METAINFO="$DIR/app/data/com.ktechpit.paraphrase.metainfo.xml"
OUT="$DIR/build/flatpak"

lint() {
    echo "== manifest"
    flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest "$MANIFEST" || true
    echo "== metainfo"
    flatpak run --command=flatpak-builder-lint org.flatpak.Builder appstream "$METAINFO" || true
}

lint
[ "${1:-}" = "--lint" ] && exit 0

mkdir -p "$OUT"
flatpak run --command=flatpak-builder --filesystem="$DIR" org.flatpak.Builder \
    --user --install --force-clean --ccache \
    --state-dir="$OUT/state" "$OUT/app" "$MANIFEST"
echo "installed: flatpak run com.ktechpit.paraphrase"

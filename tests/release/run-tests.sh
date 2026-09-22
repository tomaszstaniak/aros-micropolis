#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
VERSION="${MICROPOLIS_VERSION:-0.1.0-rc1}"
COMMIT="$(git -C "$ROOT" rev-parse HEAD)"
SOURCE="$ROOT/build/release/Micropolis-AROS-$VERSION-source.zip"
BINARY="$ROOT/build/release/micropolis.x86_64-aros-v11.lha"
SUBMISSION="$ROOT/build/release/micropolis.x86_64-aros-v11.upload.txt"
OVERLAY="$ROOT/build/release/micropolis.x86_64-aros-v11.overlay.toml"
READER="${LHA_READER:-lha}"

MICROPOLIS_ALLOW_DIRTY=1 MICROPOLIS_VERSION="$VERSION" LHA_WRITER="${LHA_WRITER:?set LHA_WRITER to a create-capable lha}" \
    bash "$ROOT/scripts/package-release.sh"
python3 "$ROOT/tests/release/source-test.py" "$SOURCE" "$COMMIT" "$VERSION"
"$READER" tq2 "$BINARY"
OUT="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-release-test.XXXXXX")"
trap 'rm -rf "$OUT"' EXIT
"$READER" xq2w="$OUT" "$BINARY"
(cd "$OUT" && zip -q -D -r "$OUT/payload.zip" Micropolis Micropolis.info .arospkg)
python3 "$ROOT/tests/launch/package-test.py" "$OUT/payload.zip"
grep -q "Corresponding source: Micropolis-AROS-$VERSION-source.zip" "$OUT/Micropolis/ReadMe.txt"
python3 "$ROOT/tests/release/submission-test.py" "$SUBMISSION" "$VERSION" "$(basename "$BINARY")" "$(basename "$SOURCE")"
python3 "$ROOT/tests/release/arospkg-manifest-test.py" \
    "$OUT/.arospkg/manifest.toml" "$SOURCE" "$OUT/Micropolis/Micropolis" "$COMMIT" "$VERSION"
python3 "$ROOT/tests/release/arospkg-overlay-test.py" "$OVERLAY" "$BINARY" "$VERSION"
echo "PASS release: LHA CRC, extracted payload, source reference, matching source archive, AROS Archives metadata, embedded manifest and overlay draft"

#!/bin/bash
# Create the AROS Archive binary LHA and its exact corresponding-source ZIP.
# Publication/upload remains a separate, explicit user action.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/env.sh"

[ "$AROS_TARGET" = one ] || {
    echo "package-release: only the runtime-verified AROS One ABIv11 target may be released" >&2
    exit 1
}
VERSION="${MICROPOLIS_VERSION:?set MICROPOLIS_VERSION (for example 0.1.0-rc1)}"
LHA_WRITER="${LHA_WRITER:?set LHA_WRITER to a create-capable classic lha executable}"
[ -x "$LHA_WRITER" ] || { echo "package-release: LHA_WRITER is not executable: $LHA_WRITER" >&2; exit 1; }
"$LHA_WRITER" --help 2>&1 | grep -q 'a   Add' || {
    echo "package-release: $LHA_WRITER cannot create archives (Homebrew Lhasa is read-only)" >&2
    exit 1
}
[ "${MICROPOLIS_ALLOW_DIRTY:-0}" = 1 ] || [ -z "$(git -C "$PROJECT_ROOT" status --porcelain)" ] || {
    echo "package-release: project repository is dirty; commit or restore it first" >&2
    exit 1
}
[ -z "$(git -C "$WORK_DIR" status --porcelain)" ] || {
    echo "package-release: patched engine work tree is dirty" >&2
    exit 1
}

bash "$SCRIPT_DIR/check-reproduction.sh"
bash "$SCRIPT_DIR/build-game.sh"

DEST_DIR="$PROJECT_ROOT/build/release"
SOURCE_NAME="Micropolis-AROS-$VERSION-source.zip"
BINARY_NAME="micropolis.x86_64-aros-v11.lha"
SUBMISSION_NAME="micropolis.x86_64-aros-v11.upload.txt"
OVERLAY_NAME="micropolis.x86_64-aros-v11.overlay.toml"
SOURCE="$DEST_DIR/$SOURCE_NAME"
BINARY="$DEST_DIR/$BINARY_NAME"
SUBMISSION="$DEST_DIR/$SUBMISSION_NAME"
OVERLAY="$DEST_DIR/$OVERLAY_NAME"
TEMP="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-release.XXXXXX")"
trap 'rm -rf "$TEMP"' EXIT
MICROPOLIS_STAGE_DIR="$TEMP/Micropolis" bash "$SCRIPT_DIR/stage-game.sh"

python3 "$SCRIPT_DIR/package-source.py" "$PROJECT_ROOT" "$WORK_DIR" "$SOURCE" "$VERSION"
mkdir -p "$TEMP/.arospkg"
python3 "$SCRIPT_DIR/write-arospkg-manifest.py" \
    "$TEMP/.arospkg/manifest.toml" "$VERSION" "$SOURCE" \
    "$(git -C "$PROJECT_ROOT" rev-parse HEAD)"
python3 - "$TEMP" "$VERSION" "$SOURCE_NAME" "$PROJECT_ROOT" <<'PY'
from pathlib import Path
import hashlib
import subprocess
import sys

stage, version, source_name, root = Path(sys.argv[1]), sys.argv[2], sys.argv[3], Path(sys.argv[4])
readme = stage / "Micropolis/ReadMe.txt"
text = readme.read_text()
text = text.replace("Micropolis for AROS - development build", f"Micropolis for AROS {version}", 1)
text = text.replace("This is not a public release package.", f"Corresponding source: {source_name}", 1)
readme.write_text(text)
commit = subprocess.check_output(["git", "-C", str(root), "rev-parse", "HEAD"], text=True).strip()
(stage / "Micropolis/BUILD.txt").write_text(
    f"Micropolis for AROS {version}\nArchitecture: x86_64\nABI: ABIv11\n"
    f"Source baseline: {commit}\nCorresponding source: {source_name}\n"
    "Do not run this package on mainline-v1 or another ABI.\n")
paths = sorted(path for path in stage.rglob("*") if path.is_file())
(stage / "Micropolis/SHA256SUMS").write_text("".join(
    hashlib.sha256(path.read_bytes()).hexdigest() + "  " + path.relative_to(stage).as_posix() + "\n"
    for path in paths))
PY

mkdir -p "$DEST_DIR"
rm -f "$BINARY"
(cd "$TEMP" && "$LHA_WRITER" aq2o51 "$BINARY" Micropolis Micropolis.info .arospkg)
python3 "$SCRIPT_DIR/write-arospkg-overlay.py" "$OVERLAY" "$VERSION" "$BINARY"
python3 "$SCRIPT_DIR/write-archive-submission.py" \
    "$SUBMISSION" "$VERSION" "$BINARY_NAME" "$SOURCE_NAME"
echo "$BINARY"
echo "$SOURCE"
echo "$SUBMISSION"
echo "$OVERLAY"

#!/bin/bash
# Local pre-release bundle; publication/source distribution is a separate task.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/env.sh"
TEMP="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-package.XXXXXX")"
trap 'rm -rf "$TEMP"' EXIT
MICROPOLIS_STAGE_DIR="$TEMP/Micropolis" bash "$SCRIPT_DIR/stage-game.sh"
case "$AROS_TARGET" in one) ABI=abiv11;; mainline) ABI=mainline-v1;; esac
DEST="$BUILD_DIR/Micropolis-x86_64-aros-$ABI.zip"
python3 "$SCRIPT_DIR/package-game.py" "$TEMP" "$DEST" "$ABI" "$PROJECT_ROOT"

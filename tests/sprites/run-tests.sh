#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
source "$ROOT/scripts/env.sh"
command -v pkg-config >/dev/null || { echo "Install pkg-config and libpng for host tests" >&2; exit 1; }
TMP="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-sprites.XXXXXX")"
trap 'rm -rf "$TMP"' EXIT
# Sanitizers are host diagnostics, not target engine build flags.
read -r -a PNG_CFLAGS <<< "$(pkg-config --cflags libpng)"
read -r -a PNG_LIBS <<< "$(pkg-config --libs libpng)"
"${CXX:-clang++}" -std=c++17 -fsanitize=address,undefined -fno-sanitize-recover=all -g \
    -I"$ROOT/src" -I"$WORK_DIR/packages/micropolis-engine/src" "${PNG_CFLAGS[@]}" \
    "$ROOT/src/sprites.cpp" "$ROOT/tests/sprites/test.cpp" "${PNG_LIBS[@]}" -o "$TMP/test"
"$TMP/test" "$WORK_DIR/content/micropolis/images"

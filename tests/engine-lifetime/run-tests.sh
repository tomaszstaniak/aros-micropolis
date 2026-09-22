#!/bin/bash
# Engine ownership regression: both active and pooled SimSprite objects must
# be destroyed with their Micropolis instance.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../../scripts/env.sh"
ENGINE="$WORK_DIR/packages/micropolis-engine"
OUT="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-lifetime.XXXXXX")"
trap 'rm -rf "$OUT"' EXIT

SOURCES=()
while IFS= read -r file; do
    case "$file" in src/callback.cpp|src/emscripten.cpp) continue;; esac
    SOURCES+=("$ENGINE/$file")
done < <(awk '/^SOURCES = \\/{on=1;next} on {print;if($0 !~ /\\$/)exit}' "$ENGINE/makefile" | sed -e 's/[\\[:space:]]*$//' -e 's/^[[:space:]]*//')

"${CXX:-clang++}" -std=c++17 -g -fsanitize=address,undefined \
    -fno-sanitize-recover=all -I"$ENGINE/src" \
    "$SCRIPT_DIR/test.cpp" "${SOURCES[@]}" -o "$OUT/test"
"$OUT/test"

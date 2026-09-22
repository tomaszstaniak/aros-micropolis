#!/bin/bash
# Actual engine scenario loading and startup selection, with host sanitizers.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../../scripts/env.sh"
ENGINE="$WORK_DIR/packages/micropolis-engine"
OUT="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-startup.XXXXXX")"
trap 'rm -rf "$OUT"' EXIT
SOURCES=()
while IFS= read -r file; do
    case "$file" in src/callback.cpp|src/emscripten.cpp) continue;; esac
    SOURCES+=("$ENGINE/$file")
done < <(awk '/^SOURCES = \\/{on=1;next} on {print;if($0 !~ /\\$/)exit}' "$ENGINE/makefile" | sed -e 's/[\\[:space:]]*$//' -e 's/^[[:space:]]*//')
# Host test instrumentation is deliberately additional to upstream C++17.
"${CXX:-clang++}" -std=c++17 -g -fsanitize=address,undefined -fno-sanitize-recover=all \
    -I"$ENGINE/src" -I"$PROJECT_ROOT/src" "$SCRIPT_DIR/test.cpp" \
    "${SOURCES[@]}" -o "$OUT/test"
(cd "$WORK_DIR/content/micropolis" && "$OUT/test" "$OUT")

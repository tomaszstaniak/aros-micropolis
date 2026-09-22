#!/bin/bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
OUT="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-messages.XXXXXX")"
trap 'rm -rf "$OUT"' EXIT
"${CXX:-clang++}" -std=c++17 -Wall -Wextra -Werror -pedantic -g \
    -fsanitize=address,undefined -fno-sanitize-recover=all \
    -I"$SCRIPT_DIR/../../src" "$SCRIPT_DIR/test.cpp" \
    "$SCRIPT_DIR/../../src/messages.cpp" -o "$OUT/test"
ASAN_OPTIONS="${ASAN_OPTIONS:+$ASAN_OPTIONS:}halt_on_error=1:abort_on_error=1" \
UBSAN_OPTIONS="${UBSAN_OPTIONS:+$UBSAN_OPTIONS:}halt_on_error=1" \
    "$OUT/test" "$SCRIPT_DIR"
python3 "$SCRIPT_DIR/integration.py"

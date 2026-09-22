#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-options.XXXXXX")"
trap 'rm -rf "$OUT"' EXIT
"${CXX:-clang++}" -std=c++17 -Wall -Wextra -Werror \
    -fsanitize=address,undefined -fno-sanitize-recover=all \
    -I"$ROOT/src" "$ROOT/tests/game-options/test.cpp" -o "$OUT/test"
"$OUT/test"

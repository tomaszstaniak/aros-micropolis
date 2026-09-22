#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT
"${CXX:-c++}" -std=c++17 -Wall -Wextra -fsanitize=address,undefined -fno-sanitize-recover=all \
    -I "$ROOT/src" "$ROOT/tests/audio/decoder-test.cpp" -o "$OUT/decoder-test"
UBSAN_OPTIONS=halt_on_error=1 "$OUT/decoder-test"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -fsanitize=address,undefined -fno-sanitize-recover=all \
    -I "$ROOT/tests/audio/stubs" -I "$ROOT/src" \
    "$ROOT/tests/audio/transport-test.cpp" "$ROOT/src/game-audio.cpp" -o "$OUT/transport-test"
UBSAN_OPTIONS=halt_on_error=1 "$OUT/transport-test" "$OUT"

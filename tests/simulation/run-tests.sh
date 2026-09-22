#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-simulation.XXXXXX")"
trap 'rm -rf "$OUT"' EXIT
clang++ -std=c++17 -fsanitize=address,undefined -fno-sanitize-recover=all -I"$ROOT/src" "$ROOT/tests/simulation/test.cpp" -o "$OUT/test"
"$OUT/test"
clang++ -std=c++17 -fsanitize=address,undefined -fno-sanitize-recover=all -I"$ROOT/tests/simulation/stubs" -I"$ROOT/src" "$ROOT/tests/simulation/timer-test.cpp" -o "$OUT/timer-test"
"$OUT/timer-test"

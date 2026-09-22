#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-launch.XXXXXX")"
trap 'rm -rf "$OUT"' EXIT
clang++ -std=c++17 -fsanitize=address,undefined -fno-sanitize-recover=all -I"$ROOT/tests/launch/stubs" -I"$ROOT/src" "$ROOT/tests/launch/test.cpp" -o "$OUT/test"
"$OUT/test"

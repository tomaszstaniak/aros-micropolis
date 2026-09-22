#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${TMPDIR:-/tmp}/micropolis-game-menu-test"
${CXX:-clang++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I"$ROOT/src" "$ROOT/tests/game-menu/test.cpp" -o "$OUT"
"$OUT"

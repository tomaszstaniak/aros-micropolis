#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
OUT="${TMPDIR:-/tmp}/micropolis-overview-render-test"
${CXX:-clang++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined \
  -I"$ROOT/src" "$ROOT/tests/overview-render/test.cpp" -o "$OUT"
"$OUT"

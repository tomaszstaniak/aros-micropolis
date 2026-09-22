#!/bin/bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)";OUT="${TMPDIR:-/tmp}/micropolis-notice-preview-test"
${CXX:-clang++} -std=c++17 -Wall -Wextra -Werror -fsanitize=address,undefined -I"$ROOT/src" "$ROOT/tests/notice-preview/test.cpp" "$ROOT/src/messages.cpp" -o "$OUT"
"$OUT"

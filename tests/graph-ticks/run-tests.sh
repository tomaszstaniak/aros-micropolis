#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
OUT="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-host-test.XXXXXX")"
trap 'rm -rf "$OUT"' EXIT
"${CXX:-clang++}" -std=c++17 -Wall -Wextra -g -fsanitize=address,undefined -fno-sanitize-recover=all \
    -I../../src test.cpp -o "$OUT/test"
"$OUT/test"

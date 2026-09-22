#!/bin/bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/env.sh"
mkdir -p "$BUILD_DIR"
"$AROS_CXX" -std=c++17 -I"$PROJECT_ROOT/src" -I"$WORK_DIR/packages/micropolis-engine/src" \
    "$PROJECT_ROOT/src/sprites.cpp" "$PROJECT_ROOT/tests/sprites/test.cpp" \
    -lpng_nostdio -lz.static -o "$BUILD_DIR/sprite-smoke"
echo 'Run: sprite-smoke PATH:Micropolis/sprites RAM:micropolis-sprite-report.txt'

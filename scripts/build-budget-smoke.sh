#!/bin/bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/env.sh"
bash "$SCRIPT_DIR/build-engine.sh"
"$AROS_CXX" -std=c++17 -I"$WORK_DIR/packages/micropolis-engine/src" \
    -I"$PROJECT_ROOT/src" "$PROJECT_ROOT/tests/budget/test.cpp" \
    "$BUILD_DIR/engine/libmicropolisengine.a" -o "$BUILD_DIR/budget-smoke"
echo 'Run: budget-smoke RAM:micropolis-budget-test.cty RAM:micropolis-budget-report.txt'

#!/bin/bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/env.sh"
mkdir -p "$BUILD_DIR"
"$AROS_CXX" -std=c++17 "$PROJECT_ROOT/tests/display/aros-smoke.cpp" \
    "$PROJECT_ROOT/src/display.cpp" -o "$BUILD_DIR/display-smoke"
echo "Built $BUILD_DIR/display-smoke; output: RAM:micropolis-display-report.txt"

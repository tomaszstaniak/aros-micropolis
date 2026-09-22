#!/bin/bash
# The same deterministic engine suite as the host, built for a real AROS guest.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/env.sh"
[ -f "$BUILD_DIR/engine/libmicropolisengine.a" ] || { echo "Run scripts/build-engine.sh first" >&2; exit 1; }
"$AROS_CXX" -std=c++17 -I "$WORK_DIR/packages/micropolis-engine/src" \
    "$PROJECT_ROOT/tests/disasters/test.cpp" "$BUILD_DIR/engine/libmicropolisengine.a" \
    -o "$BUILD_DIR/disaster-smoke"

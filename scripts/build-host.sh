#!/bin/bash
# Host (macOS) debug build of the engine + harness, with AddressSanitizer.
# The engine core is UI-independent, so crashes reproduce here in seconds
# instead of through a 5-minute VM round trip. Not an AROS artifact:
# everything lands in build/host/ and is never shipped.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=env.sh
source "$SCRIPT_DIR/env.sh"

ENGINE_SRC="$WORK_DIR/packages/micropolis-engine/src"
OUT="$PROJECT_ROOT/build/host"
mkdir -p "$OUT"

# Same exclusions as build-engine.sh: JS-only translation units.
SOURCES=$(ls "$ENGINE_SRC"/*.cpp | grep -v "emscripten.cpp" | grep -v "callback.cpp")

clang++ -std=c++17 -g -fsanitize=address -fno-omit-frame-pointer \
    -I "$ENGINE_SRC" \
    -o "$OUT/harness" \
    "$PROJECT_ROOT/src/harness.cpp" \
    $SOURCES

echo "host harness: $OUT/harness"
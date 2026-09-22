#!/bin/bash
# Build the MicropolisCore engine as a static library for the selected
# AROS target (AROS_TARGET, default one/ABIv11).
#
# The source list is read from the upstream makefile, so the recipe comes
# from the source rather than a hand-copied list.
#
# Two translation units are excluded from the native build:
#   src/emscripten.cpp — embind/WASM glue (js_callback.h is only used there)
#   src/callback.cpp    — ConsoleCallback, a JS-console demo using EM_ASM_
# The engine API itself compiles unchanged natively: micropolis.h already
# provides a non-Emscripten stub for emscripten::val (checked 2026-09-15).
#
# Flags: exactly the upstream baseline (-std=c++17). No -O, no extra flags;
# any deviation from upstream flags must be explicit and commented.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=env.sh
source "$SCRIPT_DIR/env.sh"

ENGINE_DIR="$WORK_DIR/packages/micropolis-engine"
ENGINE_SRC="$ENGINE_DIR/src"
MAKEFILE="$ENGINE_DIR/makefile"
[ -f "$MAKEFILE" ] || { echo "build-engine: no engine makefile under $ENGINE_DIR — run scripts/bootstrap.sh first" >&2; exit 1; }

OUT="$BUILD_DIR/engine"
OBJ="$OUT/obj"
LIB="$OUT/libmicropolisengine.a"
mkdir -p "$OBJ"

# --- read the source list from the upstream makefile -----------------------
NATIVE_SOURCES="$(
  awk '/^SOURCES = \\/{flag=1; next} flag{ if ($0 ~ /\\$/) { print } else { print; exit } }' "$MAKEFILE" |
    sed -e 's/[\\[:space:]]*$//' -e 's/^[[:space:]]*//' |
    sed -e 's|^src/||'
)"
[ -n "$NATIVE_SOURCES" ] || { echo "build-engine: could not parse SOURCES from $MAKEFILE" >&2; exit 1; }

count=0
echo "build-engine: target=$AROS_TARGET  CC=$AROS_CXX  out=$LIB"
while IFS= read -r src; do
  case "$src" in
    emscripten.cpp|callback.cpp) echo "build-engine: skipping $src (JS-only)"; continue ;;
  esac
  obj="$OBJ/${src//\//_}.o"
  # This small engine has 25 native units. Rebuild them all: cpp-only
  # timestamps miss header/flag/pin changes and can silently ship stale fixes.
  echo "build-engine: CXX $src"
  "$AROS_CXX" -std=c++17 -I"$ENGINE_SRC" -c "$ENGINE_SRC/$src" -o "$obj"
  count=$((count + 1))
done <<< "$NATIVE_SOURCES"

echo "build-engine: archiving $count objects"
"$AROS_GCC_ROOT/x86_64-aros-ar" rcs "$LIB" "$OBJ"/*.o
ls -l "$LIB"
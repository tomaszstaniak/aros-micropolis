#!/bin/bash
# Build the native frontend game for the selected AROS target.
# Compiles src/bmp.cpp + src/game.cpp and links against the engine
# library built by build-engine.sh (run that first).
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=env.sh
source "$SCRIPT_DIR/env.sh"

# A frontend build must include current engine patches, even if a library
# from an older build already exists.
bash "$SCRIPT_DIR/build-engine.sh"
LIB="$BUILD_DIR/engine/libmicropolisengine.a"
[ -f "$LIB" ] || { echo "build-game: missing $LIB — run scripts/build-engine.sh first" >&2; exit 1; }

OUT="$BUILD_DIR/micropolis"
python3 "$SCRIPT_DIR/build-classic-art.py" "$BUILD_DIR/generated/classic-art.h"
python3 "$SCRIPT_DIR/build-classic-art.py" --family graphs "$BUILD_DIR/generated/classic-graph-art.h"
python3 "$SCRIPT_DIR/build-classic-art.py" --family overview "$BUILD_DIR/generated/classic-overview-art.h"
"$AROS_CXX" -std=c++17 -I "$WORK_DIR/packages/micropolis-engine/src" -I src \
    -c src/startup-window.cpp -o "$BUILD_DIR/engine/startup-window.o"
"$AROS_CXX" -std=c++17 -I "$WORK_DIR/packages/micropolis-engine/src" -I src -I "$BUILD_DIR/generated" \
    -c src/graph-window.cpp -o "$BUILD_DIR/engine/graph-window.o"
"$AROS_CXX" -std=c++17 -I "$WORK_DIR/packages/micropolis-engine/src" -I src -I "$BUILD_DIR/generated" \
    -c src/overview-window.cpp -o "$BUILD_DIR/engine/overview-window.o"
"$AROS_CXX" -std=c++17 -I src -c src/game-menu.cpp -o "$BUILD_DIR/engine/game-menu.o"
"$AROS_CXX" -std=c++17 -I src -I "$BUILD_DIR/generated" \
    -c src/classic-tool-ui.cpp -o "$BUILD_DIR/engine/classic-tool-ui.o"
"$AROS_CXX" -std=c++17 \
    -I "$WORK_DIR/packages/micropolis-engine/src" \
    -I src \
    -c src/bmp.cpp -o "$BUILD_DIR/engine/bmp.o"
"$AROS_CXX" -std=c++17 \
    -I "$WORK_DIR/packages/micropolis-engine/src" \
    -I src \
    -c src/game.cpp -o "$BUILD_DIR/engine/game.o"
"$AROS_CXX" -std=c++17 \
    -I "$WORK_DIR/packages/micropolis-engine/src" \
    -I src \
    -c src/save-replace.cpp -o "$BUILD_DIR/engine/save-replace.o"
"$AROS_CXX" -std=c++17 -I src \
    -c src/display.cpp -o "$BUILD_DIR/engine/display.o"
"$AROS_CXX" -std=c++17 -I "$WORK_DIR/packages/micropolis-engine/src" -I src \
    -c src/city-windows.cpp -o "$BUILD_DIR/engine/city-windows.o"
"$AROS_CXX" -std=c++17 -I "$WORK_DIR/packages/micropolis-engine/src" -I src \
    -c src/sprites.cpp -o "$BUILD_DIR/engine/sprites.o"
"$AROS_CXX" -std=c++17 -I src \
    -c src/messages.cpp -o "$BUILD_DIR/engine/messages.o"
"$AROS_CXX" -std=c++17 -I "$WORK_DIR/packages/micropolis-engine/src" -I src \
    -c src/message-window.cpp -o "$BUILD_DIR/engine/message-window.o"
"$AROS_CXX" -std=c++17 -I src -c src/game-audio.cpp -o "$BUILD_DIR/engine/game-audio.o"
"$AROS_CXX" -o "$OUT" \
    "$BUILD_DIR/engine/game-audio.o" "$BUILD_DIR/engine/startup-window.o" "$BUILD_DIR/engine/bmp.o" "$BUILD_DIR/engine/game.o" "$BUILD_DIR/engine/classic-tool-ui.o" "$BUILD_DIR/engine/graph-window.o" "$BUILD_DIR/engine/overview-window.o" "$BUILD_DIR/engine/game-menu.o" \
    "$BUILD_DIR/engine/save-replace.o" "$BUILD_DIR/engine/display.o" "$BUILD_DIR/engine/city-windows.o" "$BUILD_DIR/engine/sprites.o" "$BUILD_DIR/engine/messages.o" "$BUILD_DIR/engine/message-window.o" "$LIB" -lgadtools -lpng_nostdio -lz.static

ls -l "$OUT"

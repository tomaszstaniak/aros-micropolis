#!/bin/bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/env.sh"
mkdir -p "$BUILD_DIR"
"$AROS_CXX" -std=c++17 -Wall -Wextra -I "$PROJECT_ROOT/src" \
    -DMICROPOLIS_AUDIO_TARGET=\""$AROS_TARGET"\" \
    "$PROJECT_ROOT/tests/audio/smoke.cpp" "$PROJECT_ROOT/src/game-audio.cpp" \
    -o "$BUILD_DIR/audio-smoke"

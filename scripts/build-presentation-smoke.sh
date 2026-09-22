#!/bin/bash
# Test-only frontend: real game loop plus explicit disaster/result triggers.
# Never staged by stage-game.sh or included in the distribution package.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/env.sh"
[ -f "$BUILD_DIR/engine/game.o" ] || { echo "Run scripts/build-game.sh first" >&2; exit 1; }
python3 - "$PROJECT_ROOT/src/game.cpp" "$BUILD_DIR/presentation-smoke.cpp" <<'PY'
from pathlib import Path
import sys
s=Path(sys.argv[1]).read_text()
anchor='                        case 0x50: uiCommand=showGameCommands(win,micropolis->enableSound);'
assert s.count(anchor)==1
insert='''                        // Test fixture only: drive real engine -> callbacks -> mainloop.
                        case 0x56: // F7: quake while paused
                            running=false;
                            micropolis->makeEarthquake();needRender=true;break;
                        case 0x57: // F8: scenario victory deadline
                        case 0x22: // D: scenario loss deadline
                            micropolis->scenario=SC_DULLSVILLE;
                            micropolis->scoreType=SC_DULLSVILLE;
                            micropolis->scoreWait=1;
                            micropolis->cityClass=code==0x57?CC_METROPOLIS:CC_CITY;
                            cb->scenario.reset();
                            micropolis->sendMessages();needRender=true;break;
'''
s=s.replace(anchor,insert+anchor)
Path(sys.argv[2]).write_text(s)
PY
"$AROS_CXX" -std=c++17 -I "$WORK_DIR/packages/micropolis-engine/src" -I "$PROJECT_ROOT/src" \
    -c "$BUILD_DIR/presentation-smoke.cpp" -o "$BUILD_DIR/presentation-smoke.o"
OBJECTS=()
for name in startup-window bmp game-audio classic-tool-ui graph-window save-replace display city-windows sprites messages message-window; do
    OBJECTS+=("$BUILD_DIR/engine/$name.o")
done
"$AROS_CXX" -o "$BUILD_DIR/presentation-smoke" "$BUILD_DIR/presentation-smoke.o" \
    "${OBJECTS[@]}" "$BUILD_DIR/engine/libmicropolisengine.a" -lpng_nostdio -lz.static

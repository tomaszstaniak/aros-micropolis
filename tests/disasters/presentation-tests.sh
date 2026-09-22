#!/bin/bash
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/../../scripts/env.sh"
ENGINE="$WORK_DIR/packages/micropolis-engine"
if [[ ! -f "$ENGINE/src/micropolis.h" ]]; then
    echo "Missing engine headers; run bash scripts/bootstrap.sh first." >&2
    exit 1
fi
OUT="$(mktemp -d "${TMPDIR:-/tmp}/micropolis-disaster-presentation.XXXXXX")"
trap 'rm -rf "$OUT"' EXIT
# Host-only fatal sanitizers supplement the project's C++17 build mode.
"${CXX:-clang++}" -std=c++17 -fsanitize=address,undefined -fno-sanitize-recover=all \
    -I"$ENGINE/src" -I"$PROJECT_ROOT/src" "$SCRIPT_DIR/presentation-test.cpp" -o "$OUT/test"
"$OUT/test"
python3 - "$PROJECT_ROOT/src/game.cpp" <<'PY'
from pathlib import Path
import sys

source = Path(sys.argv[1]).read_text()
start = source.index("if(outcome!=ScenarioPresentation::None && !done)")
end = source.index("cb->dialogShown=false", start)
block = source[start:end]
assert block.index("updateTitle(") < block.index("EasyRequestArgs("), (
    "scenario outcome must refresh the paused title before opening its modal requester"
)
print("scenario result: paused title is refreshed before requester")
PY

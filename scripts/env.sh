# Single place for machine-local paths and upstream pins. Scripts must not
# hardcode paths elsewhere.
#
# Precedence: existing environment, then an optional untracked local.env in
# the project root (see local.env.example), then the defaults below.
#
# Overridable variables:
#   AROS_TARGET, AROS_GCC_ROOT, AROS_SDK (or per target: AROS_ONE_GCC_ROOT,
#   AROS_ONE_SDK, AROS_MAINLINE_GCC_ROOT, AROS_MAINLINE_SDK), AROS_BUILD_VOLUME,
#   REPO_ID, UPSTREAM_URL

# --- project root and local.env --------------------------------------------
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
if [ -f "$PROJECT_ROOT/local.env" ]; then
  eval "$(python3 - "$PROJECT_ROOT/local.env" <<'PY'
import os, shlex, sys
from pathlib import Path
path = Path(sys.argv[1])
for raw in path.read_text().splitlines():
    line = raw.strip()
    if not line or line.startswith("#") or "=" not in line:
        continue
    key, val = (part.strip() for part in line.split("=", 1))
    if len(val) >= 2 and val[0] == val[-1] and val[0] in "\"'":
        val = val[1:-1]
    if not key or key in os.environ:
        continue
    val = os.path.expanduser(val)
    # Relative paths resolve against the project root; bare words stay settings.
    if val and ("/" in val or val.startswith(".")) and not Path(val).is_absolute():
        val = str((path.parent / val).resolve())
    print(f"export {key}={shlex.quote(val)}")
PY
)"
fi

# --- AROS target selection -------------------------------------------------
# one      = x86_64 ABIv11, as shipped by AROS One and other distributions
#            (primary target of this port);
# mainline = x86_64 ABIv1 from the AROS nightly tree (secondary, build-only).
# Toolchain, SDK and the machine running the binary must be the same ABI:
# a binary built for one ABI crashes on the other.
AROS_TARGET="${AROS_TARGET:-one}"

case "$AROS_TARGET" in
  mainline)
    AROS_GCC_ROOT="${AROS_GCC_ROOT:-${AROS_MAINLINE_GCC_ROOT:-/opt/aros/mainline/toolchain}}"
    AROS_SDK="${AROS_SDK:-${AROS_MAINLINE_SDK:-/opt/aros/mainline/Developer}}"
    ;;
  one)
    AROS_GCC_ROOT="${AROS_GCC_ROOT:-${AROS_ONE_GCC_ROOT:-/opt/aros/abiv11/toolchain}}"
    AROS_SDK="${AROS_SDK:-${AROS_ONE_SDK:-/opt/aros/abiv11/Developer}}"
    ;;
  *)
    echo "scripts/env.sh: unknown AROS_TARGET '$AROS_TARGET' (mainline|one)" >&2
    return 1 2>/dev/null || exit 1
    ;;
esac

# Both toolchains keep the cross tools directly in the toolchain root,
# not in bin/ (checked 2026-09-14).
AROS_CC="${AROS_CC:-$AROS_GCC_ROOT/x86_64-aros-gcc}"
AROS_CXX="${AROS_CXX:-$AROS_GCC_ROOT/x86_64-aros-g++}"
AROS_STRIP="${AROS_STRIP:-$AROS_GCC_ROOT/x86_64-aros-strip}"

# A mainline toolchain built in place may have its linker path baked into
# collect-aros; this names the volume that must be mounted for it (optional).
AROS_BUILD_VOLUME="${AROS_BUILD_VOLUME:-}"

# --- Upstream pin ----------------------------------------------------------
# upstreams.json is the authoritative pin (and URL); scripts read the commit
# from there. These defaults are consumed only by tools that need a name or
# a fallback URL, and are overridable (the patch-workflow tests use a
# synthetic local repository through them).
REPO_ID="${REPO_ID:-micropoliscore}"
UPSTREAM_URL="${UPSTREAM_URL:-https://github.com/SimHacker/MicropolisCore.git}"

# --- Project layout --------------------------------------------------------
UPSTREAM_DIR="$PROJECT_ROOT/upstream/$REPO_ID"
WORK_DIR="$PROJECT_ROOT/work/$REPO_ID"
PATCHES_DIR="$PROJECT_ROOT/patches/$REPO_ID"
BUILD_DIR="$PROJECT_ROOT/build/$AROS_TARGET"
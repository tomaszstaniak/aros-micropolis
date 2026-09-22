# Shared functions for the patch-workflow scripts (bootstrap, save-patch).
# Sourced, not executed; expects env.sh to be sourced first and the work
# copy to exist where a function requires it.
#
# The in-sync definition used everywhere: work HEAD tree == reference tree
# reconstructed from the manifest pin + complete series. Commit subjects and
# the recorded base are provenance hints for diagnostics, never proof of
# consistency (names can lie; content was made to lie in review round 2).

manifest_pin() {
  # Full pinned commit for $REPO_ID from upstreams.json.
  python3 -c '
import json, sys
with open(sys.argv[1]) as f:
    m = json.load(f)
print(m["repositories"][sys.argv[2]]["commit"])
' "$PROJECT_ROOT/upstreams.json" "$REPO_ID"
}

manifest_url() {
  python3 -c '
import json, sys
with open(sys.argv[1]) as f:
    m = json.load(f)
print(m["repositories"][sys.argv[2]]["url"])
' "$PROJECT_ROOT/upstreams.json" "$REPO_ID"
}

series_entries() {
  # series file -> one patch filename per line, comments/blanks removed
  grep -ve '^[[:space:]]*\#' "$PATCHES_DIR/series" 2>/dev/null | sed -e '/^[[:space:]]*$/d' || true
}

reference_tree() {
  # Tree object of (pinned base + complete series), reconstructed without
  # touching work/: a temporary index inside UPSTREAM's GIT_DIR, fed by
  # read-tree + git apply --cached (handles binary patches, new and
  # deleted files). The upstream object database is used deliberately:
  # work/'s objects may predate a pin fetched after the last bootstrap,
  # and a pin that is not present there must not make reconstruction fail.
  # Prints the tree SHA; nonzero exit if the series does not apply to
  # the pin.
  local pin="$1" idx out
  idx="$(mktemp "${TMPDIR:-/tmp}/aros-micropolis-ref.XXXXXX")"
  out="$(
    export GIT_DIR="$UPSTREAM_DIR/.git" GIT_INDEX_FILE="$idx"
    git read-tree "$pin" || exit 1
    while IFS= read -r patch; do
      git apply --cached "$PATCHES_DIR/$patch" || exit 1
    done < <(series_entries)
    git write-tree
  )"
  local rc=$?
  rm -f "$idx"
  printf '%s\n' "$out"
  return "$rc"
}

work_head_tree() {
  git -C "$WORK_DIR" rev-parse 'HEAD^{tree}'
}

work_tree_dirty() {
  [ -n "$(git -C "$WORK_DIR" status --porcelain)" ]
}

work_chain() {
  # Local commit subjects, oldest first (diagnostic only).
  local base
  base="$(git -C "$WORK_DIR" config project.base || true)"
  [ -n "$base" ] || return 1
  git -C "$WORK_DIR" log --format=%s "$base..HEAD" | tail -r
}

series_drop_last() {
  # Remove the last non-comment line of the series file; used only to roll
  # back an entry this process has just appended.
  local tmp
  tmp="$(mktemp "${TMPDIR:-/tmp}/aros-micropolis-series.XXXXXX")"
  cp "$PATCHES_DIR/series" "$tmp"
  python3 - "$tmp" <<'PYEOF'
import sys
lines = open(sys.argv[1]).readlines()
for i in range(len(lines) - 1, -1, -1):
    if lines[i].strip() and not lines[i].lstrip().startswith("#"):
        del lines[i]
        break
open(sys.argv[1], "w").writelines(lines)
PYEOF
  cat "$tmp" > "$PATCHES_DIR/series"
  rm -f "$tmp"
}
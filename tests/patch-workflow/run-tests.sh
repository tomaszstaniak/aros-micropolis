#!/bin/bash
# Regression tests for the patch workflow scripts (bootstrap, save-patch,
# check-reproduction). Host-side only: everything runs against a synthetic
# upstream in a throwaway directory; no AROS toolchain or machine needed.
#
#   tests/patch-workflow/run-tests.sh
#
# Covers the scenarios from the 2026-09-14 verification run and the three
# holes reproduced in review round 2 (2026-09-15):
#   - bootstrap must verify CONTENT (reconstructed pin+series tree), not
#     commit subjects; an edited patch or an amended commit must not pass
#     as in-sync;
#   - --recreate must preserve the previous work/ (with .git and stashes);
#   - save-patch must refuse a stale base and must write patch + series
#     BEFORE advancing the local baseline, with rollback on failure.
set -uo pipefail   # NOT -e: expected failures are asserted explicitly

PROJ_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
TESTDIR="$(mktemp -d "${TMPDIR:-/tmp}/aros-micropolis-patchtest.XXXXXX")"
export REPO_ID=testrepo
PROJ="$TESTDIR/proj"
W="$PROJ/work/testrepo"
PASS=0; FAIL=0

log() { printf '\n=== %s ===\n' "$*" | tee -a "$TESTDIR/test.log"; }
expect_ok() {
  local desc="$1"; shift
  if "$@" >>"$TESTDIR/test.log" 2>&1; then
    echo "PASS  $desc"; PASS=$((PASS+1))
  else
    echo "FAIL  $desc (expected success, got $?)"; FAIL=$((FAIL+1))
  fi
}
expect_fail() {
  local desc="$1"; shift
  if "$@" >>"$TESTDIR/test.log" 2>&1; then
    echo "FAIL  $desc (expected refusal, got success)"; FAIL=$((FAIL+1))
  else
    echo "PASS  $desc (refused as expected)"; PASS=$((PASS+1))
  fi
}
check() {
  local desc="$1"; shift
  if "$@" >/dev/null 2>&1; then
    echo "PASS  $desc"; PASS=$((PASS+1))
  else
    echo "FAIL  $desc"; FAIL=$((FAIL+1))
  fi
}
backups_before() { ls -d "$W".pre-recreate.* 2>/dev/null | wc -l; }
newest_backup() { ls -dt "$W".pre-recreate.* 2>/dev/null | head -n 1; }

# --- synthetic environment --------------------------------------------------
rm -rf "$TESTDIR/fake-upstream" "$PROJ"
mkdir -p "$TESTDIR/fake-upstream" "$PROJ/scripts" "$PROJ/patches/testrepo"
(
  cd "$TESTDIR/fake-upstream"
  git init -q
  mkdir -p "dir with space"
  printf 'hello\n' > hello.txt
  printf 'space content v1\n' > "dir with space/file with space.txt"
  printf '\x00\x01\x02\xff\xfe binary v1' > data.bin
  printf 'to be deleted\n' > gone.txt
  git add -A && git commit -q -m "fake upstream commit A"
  printf 'unrelated B\n' > unrelated-b.txt
  git add -A && git commit -q -m "fake upstream commit B"
  printf 'unrelated C\n' > unrelated-c.txt
  git add -A && git commit -q -m "fake upstream commit C"
)
PIN_A="$(git -C "$TESTDIR/fake-upstream" rev-parse HEAD~2)"
PIN_B="$(git -C "$TESTDIR/fake-upstream" rev-parse HEAD~1)"
PIN_C="$(git -C "$TESTDIR/fake-upstream" rev-parse HEAD)"
for s in env.sh workflow-lib.sh bootstrap.sh save-patch.sh check-reproduction.sh; do
  cp "$PROJ_ROOT/scripts/$s" "$PROJ/scripts/"
  chmod +x "$PROJ/scripts/$s"
done
: > "$PROJ/patches/testrepo/series"
write_manifest() {
  printf '{\n  "repositories": {\n    "testrepo": {\n      "url": "%s/fake-upstream",\n      "commit": "%s"\n    }\n  }\n}\n' \
    "$TESTDIR" "$1" > "$PROJ/upstreams.json"
}
write_manifest "$PIN_A"
: > "$TESTDIR/test.log"
cd "$PROJ"

# --- 1. fresh bootstrap -----------------------------------------------------
log "1. fresh bootstrap"
expect_ok "bootstrap creates upstream+work" ./scripts/bootstrap.sh
check "work HEAD == pin A" test "$(git -C "$W" rev-parse HEAD)" = "$PIN_A"
check "work tree clean" test -z "$(git -C "$W" status --porcelain)"

log "2. idempotent second bootstrap"
expect_ok "bootstrap in sync" ./scripts/bootstrap.sh

log "3. save-patch refuses nothing-staged"
printf 'unstaged edit\n' >> "$W/hello.txt"
expect_fail "save-patch refuses nothing-staged" ./scripts/save-patch.sh 0001-x --problem p --solution s --scope g
git -C "$W" checkout -- hello.txt

log "4. patch 1: text edit + new file"
printf 'hello patched\n' > "$W/hello.txt"
printf 'brand new\n' > "$W/new file.txt"
git -C "$W" add hello.txt "new file.txt"
expect_ok "save-patch 0001-first" ./scripts/save-patch.sh 0001-first --problem "test problem 1" --solution "test solution 1" --scope test
check "series lists 0001" grep -qx "0001-first.patch" patches/testrepo/series
check "header Base-Commit == pin A" test "$(grep -m1 '^Base-Commit:' patches/testrepo/0001-first.patch | awk '{print $2}')" = "$PIN_A"
check "header Requires == none" grep -q '^Requires: none$' patches/testrepo/0001-first.patch
check "work clean after save" test -z "$(git -C "$W" status --porcelain)"

log "5. patch 2: path with spaces + deleted file + binary change"
printf 'space content v2\n' > "$W/dir with space/file with space.txt"
printf '\x0a\x0b\x0c\xfd\xfc binary v2 -- different length' > "$W/data.bin"
git -C "$W" rm -q gone.txt
git -C "$W" add "dir with space/file with space.txt" data.bin
expect_ok "save-patch 0002-second" ./scripts/save-patch.sh 0002-second --problem "test problem 2" --solution "test solution 2" --scope test
check "header Base-Commit still pin A (not local commit)" test "$(grep -m1 '^Base-Commit:' patches/testrepo/0002-second.patch | awk '{print $2}')" = "$PIN_A"
check "header Requires == 0001-first.patch" grep -q '^Requires: 0001-first.patch$' patches/testrepo/0002-second.patch
check "0002 contains GIT binary patch" grep -q "GIT binary patch" patches/testrepo/0002-second.patch
check "0002 records deletion" grep -q "deleted file mode" patches/testrepo/0002-second.patch
check "work clean after save" test -z "$(git -C "$W" status --porcelain)"
TREE_P2="$(git -C "$W" rev-parse 'HEAD^{tree}')"

log "6. duplicate name refusal"
printf 'more\n' >> "$W/hello.txt"
git -C "$W" add hello.txt
expect_fail "save-patch refuses duplicate series name" ./scripts/save-patch.sh 0002-second --problem p --solution s --scope g
git -C "$W" reset -q && git -C "$W" checkout -- hello.txt

log "7. reproduction check in isolated dir"
expect_ok "check-reproduction applies series" ./scripts/check-reproduction.sh

log "8. recreate from scratch reproduces identical tree"
rm -rf "$W"
expect_ok "bootstrap recreates work with full series" ./scripts/bootstrap.sh
check "recreated tree identical to pre-delete tree" test "$(git -C "$W" rev-parse 'HEAD^{tree}')" = "$TREE_P2"

log "9. dirty work tree is protected"
printf 'unsaved!\n' >> "$W/hello.txt"
expect_fail "bootstrap refuses dirty work" ./scripts/bootstrap.sh
expect_fail "even --recreate refuses dirty work" ./scripts/bootstrap.sh --recreate
git -C "$W" checkout -- hello.txt

log "10. series divergence: refuse, then --recreate (old copy preserved)"
BB="$(backups_before)"
grep -v '0002-second' patches/testrepo/series > "$TESTDIR/series.tmp" && mv "$TESTDIR/series.tmp" patches/testrepo/series
expect_fail "bootstrap refuses after series edit (content check)" ./scripts/bootstrap.sh
expect_ok "bootstrap --recreate follows new series" ./scripts/bootstrap.sh --recreate
check "gone.txt back (0002 no longer applied)" test -f "$W/gone.txt"
check "0001 still applied" grep -q "hello patched" "$W/hello.txt"
check "old copy preserved when trees differ" test "$(backups_before)" -gt "$BB"
printf '0002-second.patch\n' >> patches/testrepo/series
expect_ok "bootstrap --recreate back to full series" ./scripts/bootstrap.sh --recreate
check "full-series tree identical again" test "$(git -C "$W" rev-parse 'HEAD^{tree}')" = "$TREE_P2"
check "both recreates preserved their predecessors" test "$(backups_before)" -eq "$((BB + 2))"

log "11. amended commit + stash: content check refuses; --recreate loses nothing"
printf 'rogue\n' > "$W/rogue.txt" && git -C "$W" add rogue.txt && git -C "$W" commit -q --amend --no-edit
printf 'stash me\n' >> "$W/hello.txt" && git -C "$W" stash push -q -m "kept in backup"
expect_fail "bootstrap refuses amended work (content, not names)" ./scripts/bootstrap.sh
BB="$(backups_before)"
expect_ok "bootstrap --recreate rebuilds" ./scripts/bootstrap.sh --recreate
BAK="$(newest_backup)"
check "backup created" test -d "$BAK/.git"
check "rogue file preserved in backup" test -f "$BAK/rogue.txt"
check "stash preserved in backup" test -n "$(git -C "$BAK" stash list)"
check "rogue file absent in new work" test ! -f "$W/rogue.txt"
check "new work tree == series tree" test "$(git -C "$W" rev-parse 'HEAD^{tree}')" = "$TREE_P2"
expect_ok "bootstrap in sync after recreate" ./scripts/bootstrap.sh

log "12. pin change: diagnose, then safe --recreate"
write_manifest "$PIN_B"
expect_fail "bootstrap refuses stale pin without --recreate" ./scripts/bootstrap.sh
expect_ok "bootstrap --recreate rebuilds at new pin" ./scripts/bootstrap.sh --recreate
check "work base config == pin B" test "$(git -C "$W" config project.base)" = "$PIN_B"
REF="$TESTDIR/refclone"; rm -rf "$REF"
git clone -q --no-hardlinks "$PROJ/upstream/testrepo" "$REF"
git -C "$REF" checkout -q --detach "$PIN_B"
git -C "$REF" apply --index "$PROJ/patches/testrepo/0001-first.patch" && git -C "$REF" commit -q -m 0001
git -C "$REF" apply --index "$PROJ/patches/testrepo/0002-second.patch" && git -C "$REF" commit -q -m 0002
check "work tree == reference B+series tree" test "$(git -C "$W" rev-parse 'HEAD^{tree}')" = "$(git -C "$REF" write-tree)"
check "unrelated-b.txt present (pin B applied)" test -f "$W/unrelated-b.txt"
check "patches still effective on B" grep -q "hello patched" "$W/hello.txt"
expect_ok "reproduction check at new pin" ./scripts/check-reproduction.sh

log "13. upstream pollution is refused"
printf 'dirt\n' >> "$PROJ/upstream/testrepo/hello.txt"
expect_fail "bootstrap refuses dirty upstream" ./scripts/bootstrap.sh
git -C "$PROJ/upstream/testrepo" checkout -- hello.txt
expect_ok "bootstrap ok after cleanup" ./scripts/bootstrap.sh

log "14. REGRESSION (review r2): edited patch content must not pass as in-sync"
python3 - "$PROJ/patches/testrepo/0001-first.patch" <<'PYEOF'
import sys
p = sys.argv[1]
s = open(p).read().replace("+hello patched\n", "+hello patched differently\n")
open(p, "w").write(s)
PYEOF
expect_fail "bootstrap detects edited patch (content check)" ./scripts/bootstrap.sh
expect_ok "bootstrap --recreate applies the edited patch" ./scripts/bootstrap.sh --recreate
check "edited content applied" grep -q "hello patched differently" "$W/hello.txt"
python3 - "$PROJ/patches/testrepo/0001-first.patch" <<'PYEOF'
import sys
p = sys.argv[1]
s = open(p).read().replace("+hello patched differently\n", "+hello patched\n")
open(p, "w").write(s)
PYEOF
expect_fail "bootstrap detects divergence again after restore" ./scripts/bootstrap.sh
expect_ok "bootstrap --recreate back to original series" ./scripts/bootstrap.sh --recreate
check "original content back" grep -q "hello patched" "$W/hello.txt"
expect_ok "bootstrap in sync" ./scripts/bootstrap.sh

log "15. REGRESSION (review r2): save-patch refuses a stale base"
HEAD_BEFORE="$(git -C "$W" rev-parse HEAD)"
write_manifest "$PIN_C"
printf 'change on stale base\n' >> "$W/hello.txt"
git -C "$W" add hello.txt
expect_fail "save-patch refuses stale pin" ./scripts/save-patch.sh 0003-stale --problem p --solution s --scope test
check "no patch file left behind" test ! -f patches/testrepo/0003-stale.patch
check "work HEAD unchanged" test "$(git -C "$W" rev-parse HEAD)" = "$HEAD_BEFORE"
check "series unchanged (2 entries)" test "$(grep -c . patches/testrepo/series)" = 2
check "staged change still staged (recoverable)" test -n "$(git -C "$W" diff --cached)"
write_manifest "$PIN_B"
expect_ok "save-patch succeeds after pin restored" ./scripts/save-patch.sh 0003-stale --problem p --solution s --scope test
check "0003 saved with Base-Commit == pin B" test "$(grep -m1 '^Base-Commit:' patches/testrepo/0003-stale.patch | awk '{print $2}')" = "$PIN_B"
rm -f patches/testrepo/0003-stale.patch
python3 - "$PROJ/patches/testrepo/series" <<'PYEOF'
import sys
p = sys.argv[1]
lines = open(p).readlines()
open(p, "w").writelines(l for l in lines if l.strip() != "0003-stale.patch")
PYEOF
expect_ok "bootstrap --recreate back to 2-patch state" ./scripts/bootstrap.sh --recreate
expect_ok "bootstrap in sync" ./scripts/bootstrap.sh

log "16. REGRESSION (review r2): read-only series -> rollback, no baseline shift"
chmod 444 patches/testrepo/series
printf 'rollback test\n' >> "$W/hello.txt"
git -C "$W" add hello.txt
HEAD_BEFORE="$(git -C "$W" rev-parse HEAD)"
expect_fail "save-patch fails on read-only series" ./scripts/save-patch.sh 0003-ro --problem p --solution s --scope test
check "no patch file left behind" test ! -f patches/testrepo/0003-ro.patch
check "work HEAD unchanged (no baseline shift)" test "$(git -C "$W" rev-parse HEAD)" = "$HEAD_BEFORE"
check "series unchanged (2 entries)" test "$(grep -c . patches/testrepo/series)" = 2
chmod 644 patches/testrepo/series
expect_ok "save-patch succeeds after chmod" ./scripts/save-patch.sh 0003-ro --problem p --solution s --scope test
rm -f patches/testrepo/0003-ro.patch
python3 - "$PROJ/patches/testrepo/series" <<'PYEOF'
import sys
p = sys.argv[1]
lines = open(p).readlines()
open(p, "w").writelines(l for l in lines if l.strip() != "0003-ro.patch")
PYEOF
expect_ok "bootstrap --recreate back to 2-patch state" ./scripts/bootstrap.sh --recreate
expect_ok "in-sync check" ./scripts/bootstrap.sh
expect_ok "reproduction check" ./scripts/check-reproduction.sh

log "17. REGRESSION (review r3): new pin fetched after first bootstrap"
rm -f patches/testrepo/0001-first.patch patches/testrepo/0002-second.patch
: > patches/testrepo/series
rm -rf "$W"
expect_ok "fresh 0-patch work at pin B" ./scripts/bootstrap.sh
(
  cd "$TESTDIR/fake-upstream"
  printf 'unrelated D\n' > unrelated-d.txt
  git add -A && git commit -q -m "fake upstream commit D"
)
PIN_D="$(git -C "$TESTDIR/fake-upstream" rev-parse HEAD)"
write_manifest "$PIN_D"
expect_fail "bootstrap refuses stale pin (no --recreate)" ./scripts/bootstrap.sh
expect_ok "bootstrap --recreate works when the new pin is absent from work's objects" ./scripts/bootstrap.sh --recreate
check "work HEAD == pin D" test "$(git -C "$W" rev-parse HEAD)" = "$PIN_D"
check "unrelated-d.txt present" test -f "$W/unrelated-d.txt"
check "work base config == pin D" test "$(git -C "$W" config project.base)" = "$PIN_D"
expect_ok "bootstrap in sync at pin D" ./scripts/bootstrap.sh

log "18. REGRESSION (review r3): new pin with identical tree must not pass as in-sync"
(
  cd "$TESTDIR/fake-upstream"
  git commit -q --allow-empty -m "fake upstream commit E (identical tree)"
)
PIN_E="$(git -C "$TESTDIR/fake-upstream" rev-parse HEAD)"
write_manifest "$PIN_E"
expect_fail "identical-tree pin change must NOT report in-sync" ./scripts/bootstrap.sh
expect_ok "bootstrap --recreate updates the recorded base" ./scripts/bootstrap.sh --recreate
check "work base config == pin E" test "$(git -C "$W" config project.base)" = "$PIN_E"
check "work HEAD == pin E" test "$(git -C "$W" rev-parse HEAD)" = "$PIN_E"
printf 'after empty pin\n' >> "$W/hello.txt"
git -C "$W" add hello.txt
expect_ok "save-patch works after identical-tree pin update" ./scripts/save-patch.sh 0004-after-empty --problem p --solution s --scope test
check "0004 Base-Commit == pin E" test "$(grep -m1 '^Base-Commit:' patches/testrepo/0004-after-empty.patch | awk '{print $2}')" = "$PIN_E"
expect_ok "final reproduction check" ./scripts/check-reproduction.sh

echo
echo "RESULT: $PASS passed, $FAIL failed (log: $TESTDIR/test.log)"
[ "$FAIL" = 0 ]
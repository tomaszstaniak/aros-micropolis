#!/bin/bash
# Bootstrap: clean pinned upstream checkout + editable work copy with the
# full patch series applied, verified by CONTENT against the manifest.
#
#   scripts/bootstrap.sh              create or verify (in-sync: no-op)
#   scripts/bootstrap.sh --recreate   rebuild work/ after a pin/series change
#
# "In sync" means: the work copy's recorded base equals the manifest pin,
# AND its HEAD tree equals the reconstructed (pin + complete series) tree
# (reconstruction uses upstream/'s objects, so a newly fetched pin works).
# Commit subjects are not proof of anything — a patch whose content
# changed, or an amended local commit, must be detected, not assumed away.
#
# Safety rules:
#   - A dirty upstream or a dirty work tree is never touched.
#   - --recreate never deletes the previous work copy: it is moved aside
#     with its full .git (including any stash) and reported. Since divergence
#     is proven by content, a rebuilt tree can never equal the old one, so
#     there is no "identical, safe to drop" case to detect — the old copy is
#     always kept for manual inspection and removal.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
# shellcheck source=env.sh
source "$SCRIPT_DIR/env.sh"
# shellcheck source=workflow-lib.sh
source "$SCRIPT_DIR/workflow-lib.sh"

SERIES="$PATCHES_DIR/series"
MANIFEST="$PROJECT_ROOT/upstreams.json"
RECREATE=0
[ "${1:-}" = "--recreate" ] && RECREATE=1
[ $# -gt 1 ] && { echo "usage: bootstrap.sh [--recreate]" >&2; exit 1; }

[ -f "$MANIFEST" ] || { echo "bootstrap: missing $MANIFEST — create it first (see upstreams.example.json)" >&2; exit 1; }

PINNED_URL="$(manifest_url)"
PINNED_COMMIT="$(manifest_pin)"
[ -n "$PINNED_COMMIT" ] || { echo "bootstrap: no pinned commit for '$REPO_ID' in upstreams.json" >&2; exit 1; }

# --- upstream/: clean checkout of the pinned commit, never edited ----------
if [ -d "$UPSTREAM_DIR/.git" ]; then
  # Never modify another project's checkout; ours must stay clean too.
  if [ -n "$(git -C "$UPSTREAM_DIR" status --porcelain)" ]; then
    echo "bootstrap: $UPSTREAM_DIR is dirty — upstream must stay clean. Inspect manually." >&2
    exit 1
  fi
else
  echo "bootstrap: cloning $PINNED_URL into $UPSTREAM_DIR"
  git clone --quiet "$PINNED_URL" "$UPSTREAM_DIR"
fi

git -C "$UPSTREAM_DIR" fetch --all --quiet
git -C "$UPSTREAM_DIR" checkout --quiet --detach "$PINNED_COMMIT"
ACTUAL="$(git -C "$UPSTREAM_DIR" rev-parse HEAD)"
[ "$ACTUAL" = "$PINNED_COMMIT" ] || { echo "bootstrap: upstream HEAD $ACTUAL != manifest $PINNED_COMMIT" >&2; exit 1; }
echo "bootstrap: upstream $REPO_ID at $PINNED_COMMIT (clean)"

create_work() {
  echo "bootstrap: creating $WORK_DIR from $PINNED_COMMIT (+ $(series_entries | wc -l | tr -d ' ') patch(es))"
  git clone --quiet --no-hardlinks "$UPSTREAM_DIR" "$WORK_DIR"
  git -C "$WORK_DIR" checkout --quiet --detach "$PINNED_COMMIT"
  git -C "$WORK_DIR" remote remove origin
  # Provenance hints for diagnostics; one commit per patch, subject = patch
  # filename. Consistency itself is always proven by tree content.
  git -C "$WORK_DIR" config project.base "$PINNED_COMMIT"
  series_entries | while IFS= read -r patch; do
    echo "bootstrap: applying $patch"
    git -C "$WORK_DIR" apply --index "$PATCHES_DIR/$patch"
    git -C "$WORK_DIR" commit --quiet -m "$patch"
  done
}

# --- work_: verify or create ------------------------------------------------
if [ ! -d "$WORK_DIR/.git" ]; then
  create_work
  echo "bootstrap: work copy ready at $WORK_DIR"
  exit 0
fi

if work_tree_dirty; then
  echo "bootstrap: $WORK_DIR has uncommitted changes; refusing to touch it." >&2
  echo "  Save them with scripts/save-patch.sh, or git -C '$WORK_DIR' stash/reset by hand." >&2
  exit 1
fi

if ! REF_TREE="$(reference_tree "$PINNED_COMMIT")"; then
  echo "bootstrap: the series does not apply to pinned commit $PINNED_COMMIT — fix patches/series first" >&2
  exit 1
fi
WORK_TREE="$(work_head_tree)"
WORK_BASE="$(git -C "$WORK_DIR" config project.base || true)"

# In sync = recorded base matches the manifest pin AND content matches the
# reconstructed pin+series tree. Content alone is not enough: a new pin
# with an identical tree would otherwise leave the stale base in place and
# make the next save-patch refuse.
if [ -n "$WORK_BASE" ] && [ "$WORK_BASE" = "$PINNED_COMMIT" ] && [ "$WORK_TREE" = "$REF_TREE" ]; then
  echo "bootstrap: work copy in sync with pin $PINNED_COMMIT and series ($(series_entries | wc -l | tr -d ' ') patch(es)) — verified by content"
  exit 0
fi

# --- divergence: diagnose, then rebuild only on explicit request -----------
echo "bootstrap: work copy does NOT match pin + series:" >&2
if [ -z "$WORK_BASE" ]; then
  echo "  work copy has no recorded base (created by an older tool?) — rebuild with --recreate" >&2
elif [ "$WORK_BASE" != "$PINNED_COMMIT" ] && [ "$WORK_TREE" = "$REF_TREE" ]; then
  echo "  pin changed ($WORK_BASE -> $PINNED_COMMIT) but the tree content is identical;" >&2
  echo "  the recorded base must be updated — re-run with --recreate" >&2
else
  echo "  work HEAD tree:    $WORK_TREE" >&2
  echo "  pin + series tree: $REF_TREE" >&2
  [ "$WORK_BASE" = "$PINNED_COMMIT" ] || echo "  hint: recorded base $WORK_BASE != manifest pin $PINNED_COMMIT (pin changed?)" >&2
  if ! diff <(work_chain 2>/dev/null) <(series_entries) >/dev/null 2>&1; then
    echo "  hint: local commit subjects differ from the series (patch added/removed/renamed?)" >&2
  fi
  echo "  A patch file may have been edited, or work/ has commits beyond the series." >&2
fi

if [ "$RECREATE" = 0 ]; then
  echo "bootstrap: refusing to proceed. After protecting unsaved work, re-run:" >&2
  echo "  scripts/bootstrap.sh --recreate" >&2
  exit 1
fi

# Never delete the old copy: move it aside with its full .git (stashes
# included). The name is collision-proof: two recreates within one second
# must not nest the old copies inside each other.
BACKUP="$WORK_DIR.pre-recreate.$(date +%Y%m%d-%H%M%S)"
N=0
while [ -e "$BACKUP" ]; do
  N=$((N + 1))
  BACKUP="$WORK_DIR.pre-recreate.$(date +%Y%m%d-%H%M%S).$N"
done
echo "bootstrap: --recreate: preserving previous copy at $BACKUP"
mv "$WORK_DIR" "$BACKUP"
create_work
if [ "$(git -C "$WORK_DIR" rev-parse 'HEAD^{tree}')" = "$WORK_TREE" ]; then
  echo "bootstrap: rebuilt content is identical (pin-only change); previous copy kept at" >&2
else
  echo "bootstrap: rebuilt work differs from the preserved copy; previous copy kept at" >&2
fi
echo "  $BACKUP" >&2
echo "  Inspect and delete it manually once you have what you need from it." >&2
echo "bootstrap: work copy ready at $WORK_DIR"
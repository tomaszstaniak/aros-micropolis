# Patch-workflow regression tests

Host-side tests for `scripts/bootstrap.sh`, `scripts/save-patch.sh`, and
`scripts/check-reproduction.sh`. They build a synthetic upstream in a
throwaway directory and drive the real scripts against it — no AROS
toolchain, SDK, or QEMU machine involved.

    tests/patch-workflow/run-tests.sh

Exit status is nonzero on any failure; the summary line names the counts.
Each run creates its own temporary directory and always leaves it behind
(the path is printed at the end) so failures can be inspected.

## What is covered

The suite encodes two review rounds:

- Round 1 (2026-09-14): fresh bootstrap and idempotency; sequential patches
  with new/deleted/binary files and paths with spaces; `Base-Commit` from
  the manifest and `Requires` from the series; duplicate-name and
  nothing-staged refusals; isolated reproduction; tree identity after
  rebuild; dirty work tree and dirty upstream protection; series edits and
  pin changes via `--recreate`; unsaved-commit protection.
- Round 2 (2026-09-15), from reproduced data-loss holes:
  - **content, not names**: an edited patch file and an amended local commit
    must make bootstrap refuse "in sync" (the check reconstructs the
    pin+series tree and compares);
  - **`--recreate` preserves everything**: the previous work copy is moved
    aside with `.git`, stashes included, and reported; nothing is deleted
    by the script. Backup names are collision-proof (two recreates within
    one second must not nest the old copies — caught by this suite during
    re-verification);
  - **stale base refused**: save-patch fails after a manifest pin change
    until work is reconciled, leaving work/ and the series untouched;
  - **ordering with rollback**: patch file + series are written before the
    local baseline advances; a read-only `series` fails with a full rollback
    (no patch file, no commit, series unchanged).
- Round 3 (2026-09-15), pin-update holes:
  - **reconstruction from the right objects**: a pin fetched after the last
    bootstrap must not break `--recreate` (the reference tree is built from
    `upstream/`'s object database, not work's);
  - **in sync includes the base**: a new pin with an identical tree must not
    pass as in-sync — the recorded base has to be updated, and save-patch
    must work afterwards.

## Scope and limitations

- The matrix proves the mechanics on a small synthetic repository. The first
  real engine patch may still surface surprises; its save gets its
  validation recorded in the patch header.
- `check-reproduction.sh` clones from the local `upstream/` checkout, so it
  does not re-prove the manifest URL is fetchable from the network.
- Backup directories (`work/<repo>.pre-recreate.*`) accumulate by design:
  the scripts never delete them, the tests only count them. In the synthetic
  environment they vanish with the temp directory.
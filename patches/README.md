# Engine patches

`micropoliscore/` holds the changes this port makes to the pinned
MicropolisCore engine (`upstreams.json`). `series` lists the patches in
application order; each patch starts with a header stating the problem, the
fix, how it was validated and when it can be removed.

`scripts/bootstrap.sh` recreates `work/micropoliscore` from the pin plus this
series, and `scripts/save-patch.sh` records a new engine change here. See
[docs/BUILDING.md](../docs/BUILDING.md).

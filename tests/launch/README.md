# Launch and package checks

`bash tests/launch/run-tests.sh` runs the production LaunchDirectory against
recording DOS calls under ASan/UBSan: explicit CLI arguments resolved before
PROGDIR switch, opaque Workbench argv, errors/ownership and caller restoration.

`python3 tests/launch/package-test.py <package.zip>` verifies the real ZIP:
all payload hashes, executable mode, mandatory asset families and notices,
and two CRC-valid PNG states with stack/type metadata for each icon.

These do not simulate Workbench or prove native icon recognition. See the
2026-09-20-workbench-launch report for double-click/startup/map/exit and
missing-resource requester evidence on ABIv11.

# Display tests

`bash tests/display/run-tests.sh`: six host ASan/UBSan checks of the geometry
used by the frontend: whole tiles, bounds, camera clamping and coordinates.

`scripts/build-display-smoke.sh`: build a native test with the same
`src/display.cpp` as the game. Run `display-smoke` on the matching ABI.
It writes `RAM:micropolis-display-report.txt` and exits after testing resize,
an invalid display ID, and three custom-screen lifetimes. Run while nothing else is
using the guest's screen. This test opens windows/screens temporarily.

The smoke does not load the simulation or inject allocation failures.
Game interaction evidence is separate in the save/display report.

# Classic tool selection regression

Run `bash tests/classic-tools/run-tests.sh` (C++17, ASan/UBSan).
The 59 assertions cover original palette hit regions including gaps and tall
icons, pie sectors, first-click latch, second center cancellation, submenu
transitions and palette-slot mappings. No native event transport is simulated
here. `python3 tests/gestures/run-tests.py` separately compiles the actual
game mouse switch with a recording engine (40 assertions), including both
Shift qualifiers, modal-drain request, middle-pan remainder and cancelled
building/painting. Guest checks are recorded in the classic-tools report.
`bash tests/display/run-tests.sh` also checks that intermediate margin clearing
never erases the visible map, the cause of the reported hover flash.

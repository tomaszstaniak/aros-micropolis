# Budget regression

Run `bash tests/budget/run-tests.sh` on the host (clang, ASan/UBSan).
It compiles the real native engine source list and tests annual settlement,
service effects, draft isolation, funding save/load round-trip, signed four-byte
city fields, exact disk bytes and neighboring-history preservation (16 assertions).
Sanitizer errors are fatal; a success exit alone was insufficient before patch 0007.
Run `scripts/build-budget-smoke.sh` to build the same test for the selected
AROS ABI. On the guest pass two scratch paths (city file, text report), e.g.
`budget-smoke RAM:budget-test.cty RAM:budget-report.txt`.
The test owns and overwrites these paths; never pass a user's saved city.
The budget window itself is exercised by hand on the target machine.

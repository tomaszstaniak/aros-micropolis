# Unsaved-city tests

`bash tests/unsaved-city/run-tests.sh`: `src/unsaved-city.h` against the real
engine (built like `tests/budget`), host ASan/UBSan. The fingerprint is stable
for an unchanged city, changes after a road edit, a tax change and simulated
time, and ignores pause/speed changes while restoring `simSpeed`. An
unwritable scratch path yields 0, which counts as modified; the scratch file is
removed. `confirmLeavingCity` is checked for every requester answer and for a
failed save.

The native requester and the real `RAM:` scratch path are not exercised here.

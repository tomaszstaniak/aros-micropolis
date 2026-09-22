# Sprite tests

Run `bash tests/sprites/run-tests.sh` from the project root. Requires clang++
and host libpng through pkg-config. ASan/UBSan failures are fatal.
The same test.cpp builds for the selected AROS ABI through
`scripts/build-sprite-smoke.sh`; its optional second argument is a report file.
The original art is pinned by hash; a live comparison against a screenshot
of the running game is a manual check.

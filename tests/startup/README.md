# Startup selection regression

Run from the project root:

- `bash tests/startup/run-tests.sh`: 60 assertions against the real pinned,
  patched engine, built with fatal ASan/UBSan. Tests eight scenarios, their
  dates/funds/disaster and score IDs, missing/truncated reads, preview ownership,
  history, difficulty funding, Play transfer and scaled button hit geometry.
- `python3 tests/startup/art-tests.py`: verifies source hashes, converts all
  27 original XPMs, then uses production `bmp.cpp` to compare every decoded
  pixel against the source RGB palette. This reproduced the padded-row bug
  before fixing the loader. Requires Python 3 and clang++, no Pillow.

Neither is a native event-loop test. Startup previews use new engine
instances because engine file loading itself is not transactional.

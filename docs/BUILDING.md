# Building Micropolis for AROS

## Targets

| `AROS_TARGET` | ABI | Status |
|---|---|---|
| `one` (default) | x86_64 ABIv11 (AROS One and other distributions) | primary, tested on AROS One 1.3 |
| `mainline` | x86_64 ABIv1 (AROS nightly tree) | compiles and links; not run |

A binary only runs on the ABI it was built for. Toolchain, SDK and the machine
that runs the result must match.

## Requirements

- An x86_64 AROS cross toolchain (GCC 10.5 is what this port is built with)
  whose directory contains `x86_64-aros-gcc` and `x86_64-aros-g++`, and the
  matching SDK `Developer` directory.
- `git`, `bash`, `python3` (3.11 or later), `ffmpeg` (the original MP3 effects
  are converted to PCM WAV at staging time).
- For host tests: `clang++` with AddressSanitizer/UBSan and `pkg-config`
  with libpng.
- For release archives: a create-capable classic `lha` (for example
  github.com/jca02266/lha). Lhasa can only extract.

## Configuration

All machine-local paths go through `scripts/env.sh`. Copy `local.env.example`
to `local.env` (untracked) and set the toolchain and SDK directories:

```sh
AROS_TARGET=one
AROS_ONE_GCC_ROOT=~/aros/abiv11/toolchain
AROS_ONE_SDK=~/aros/abiv11/Developer
```

Environment variables override `local.env`, which overrides the defaults
(`/opt/aros/abiv11/...`, `/opt/aros/mainline/...`).

## Engine sources and patches

The engine is not vendored. `upstreams.json` pins MicropolisCore; the port's
changes are the numbered patches in `patches/micropoliscore/`, applied in
`series` order.

| Command | Result |
|---|---|
| `bash scripts/bootstrap.sh` | clean pinned checkout in `upstream/`, patched working copy in `work/` (both ignored), verified by content |
| `bash scripts/bootstrap.sh --recreate` | rebuild `work/` after a pin or series change; the old copy is moved aside, never deleted |
| `bash scripts/save-patch.sh NNNN-name --problem … --solution … --scope …` | record staged changes in `work/` as the next patch |
| `bash scripts/check-reproduction.sh` | apply the whole series to a fresh clone of the pin |

Never edit `upstream/`. Deleting `work/` by hand discards unsaved engine
changes; check `git -C work/micropoliscore status` first.

## Building

```sh
bash scripts/build-game.sh                     # build/one/micropolis
AROS_TARGET=mainline bash scripts/build-game.sh
bash scripts/stage-game.sh                     # runnable drawer in build/one/package/
bash scripts/package-game.sh                   # build/one/Micropolis-x86_64-aros-abiv11.zip
python3 tests/launch/package-test.py build/one/Micropolis-x86_64-aros-abiv11.zip
```

`build-game.sh` always rebuilds all 25 engine units, so a stale engine
library can never hide a newer patch. Do not fully strip the executable:
`x86_64-aros-strip` without options produces a file some AROS loaders
relocate incorrectly. If stripping is needed, use
`--strip-unneeded --remove-section .comment`.

To run: copy the whole `Micropolis` drawer **and** `Micropolis.info` to an
installed disk. Command-line arguments are optional: a city, a tile sheet and
a sprite directory, resolved from the current directory.

## Host tests

Each directory under `tests/` has a `run-tests.sh` (and `README.md`); the
gesture suite is `python3 tests/gestures/run-tests.py`. They compile the real
frontend code against the real engine or against recording stand-ins for
Intuition and Exec, with ASan/UBSan. Run them all:

```sh
for t in tests/*/run-tests.sh; do
  [ "$t" = tests/release/run-tests.sh ] && continue
  bash "$t" || echo "FAILED: $t"
done
python3 tests/gestures/run-tests.py
```

Native test programs (`scripts/build-*-smoke.sh`) are built for the selected
ABI, run on the AROS machine and write their reports to files; they are never
part of a package.

## Release archives

```sh
MICROPOLIS_VERSION=0.1.0-rc4 LHA_WRITER=/path/to/lha bash scripts/package-release.sh
MICROPOLIS_VERSION=0.1.0-rc4 LHA_WRITER=/path/to/lha bash tests/release/run-tests.sh
```

The script refuses a dirty tree and the mainline target, rechecks patch
reproduction and rebuilds. `build/release/` then holds:

- `micropolis.x86_64-aros-v11.lha`: the drawer, its icon and the proposed
  embedded arospkg manifest `.arospkg/manifest.toml` (LHA level-1 headers keep
  the executable bit);
- `Micropolis-AROS-<version>-source.zip`: the complete corresponding source,
  i.e. this repository at the release commit plus the exact patched engine
  tree;
- `micropolis.x86_64-aros-v11.upload.txt`: fields for the AROS Archives form;
- `micropolis.x86_64-aros-v11.overlay.toml`: an arospkg catalogue entry for
  the exact archive.

`tests/release/run-tests.sh` checks every LHA member with an independent
extractor, the payload hashes, the executable bit, the source archive's
completeness and identity, and both manifests.

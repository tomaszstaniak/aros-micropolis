# Notices

## This repository

The AROS frontend (`src/`), scripts, tests and patch headers are licensed
under the GNU General Public License, version 3 or (at your option) any later
version. The full text is in [LICENSE](LICENSE).

## MicropolisCore engine and content

`scripts/bootstrap.sh` fetches MicropolisCore at the commit pinned in
`upstreams.json` and applies `patches/micropoliscore/`. The engine, tiles,
sprites, sounds and cities are distributed by that project under the
Micropolis GPL License Notice: GPL-3.0-or-later **with additional terms from
Electronic Arts under GPL section 7** (no trademark rights, keep the notice,
indemnification for contractual liability you assume, modified versions must
be marked as such). The upstream files `LICENSE`,
`MicropolisGPLLicenseNotice.md` and `MicropolisPublicNameLicense.md` are
copied into every package under `Micropolis/Licenses/`.

This is a **modified version** of Micropolis. It is not the original program
and is not affiliated with or endorsed by Electronic Arts.

## Classic interface artwork

`assets/classic-*` contain unmodified images from the classic Tcl/Tk edition
(github.com/SimHacker/micropolis, `micropolis-activity/`), pinned by commit
and SHA-256 in each `source.json`. Each directory carries the original
licence header (`COPYING`) and provenance (`NOTICE.txt`).

## Names

Micropolis is a registered trademark of Micropolis Corporation (Micropolis
GmbH), used here under the Micropolis Public Name License. SimCity is a
trademark of Electronic Arts Inc.

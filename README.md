# Micropolis for AROS

A native AROS port of **Micropolis**, the open-source release of the original
SimCity Classic simulation by Will Wright and Don Hopkins. The simulation is
the [MicropolisCore](https://github.com/SimHacker/MicropolisCore) C++ engine,
pinned and patched; the user interface is a new frontend written for AROS with
Intuition, CyberGraphics, GadTools, ASL, icon.library and AHI. It needs neither
SDL nor Tcl/Tk.

The interface follows the classic Tcl/Tk (OLPC) edition of Micropolis: its tool
palette and radial tool menus, startup and scenario screen, overview map with
overlays and R/C/I demand, history graphs, budget and evaluation windows,
located messages and menus, all adapted to native Amiga windows.

**Status:** `0.1.0-rc3`, a release candidate for **x86_64 AROS ABIv11** (AROS
One and other ABIv11 distributions). It is tested on AROS One 1.3 x86_64. A
mainline (ABIv1) build compiles and links but has not been run; the two ABIs
are not binary compatible.

## Installing

Download `micropolis.x86_64-aros-v11.lha` from the
[releases](https://github.com/tomaszstaniak/aros-micropolis/releases), extract
it to an installed writable disk and keep the `Micropolis` drawer and
`Micropolis.info` together. Double-click the Micropolis icon. No installer and
no extra game data are needed.

Double-clicking a city icon (for example `CITY.CTY`) opens that city directly.
Every city you save gets its own icon. Save to an installed disk; the default
folder is `SYS:Micropolis/`. RAM: is lost at reboot.

## Playing

| Input | Action |
|---|---|
| LMB | build or paint with the selected tool |
| Shift+LMB | classic radial tool menu (also the "Pie" line in Tools) |
| RMB | application menu, in every Micropolis window |
| Middle mouse drag, cursor keys | move the map |
| `+` / `-` / mouse wheel, `0` | zoom 50/100/200%, back to 100% |
| Space, N | run/pause, single step |
| F1-F7 | commands, budget, evaluation, messages, graphs, speed, overview |
| F9, F10 | choose a screen mode, switch between Workbench and own screen |
| S, L, Esc | save, load, quit |

**Chalk** draws notes on the map and **Eraser** removes whole chalk strokes,
as in the original; chalk is free and is not saved with the city. Micropolis >
Choose City (Amiga+N) returns to the city selection during play. Quitting,
loading or choosing another city asks first whenever the current city has
unsaved changes. Window positions are remembered separately for the Workbench
and for the private screen.

There is no music: the original game has sound effects only, which are played
through AHI when available.

## Building

See [docs/BUILDING.md](docs/BUILDING.md). In short, with an x86_64 AROS ABIv11
cross toolchain and SDK configured in `local.env`:

```sh
bash scripts/bootstrap.sh      # pinned engine + patch series
bash scripts/build-game.sh     # build/one/micropolis
bash scripts/package-game.sh   # runnable ZIP of the whole drawer
```

Host tests (macOS or Linux, clang with sanitizers) live under `tests/`.
[docs/DESIGN.md](docs/DESIGN.md) describes the frontend and every deliberate
difference from the classic interface. Changes are listed in
[CHANGELOG.md](CHANGELOG.md).

## Licence

The engine and the original artwork are GPL-3.0-or-later with additional terms
from Electronic Arts under GPL section 7; the AROS frontend in this repository
is GPL-3.0-or-later. See [NOTICE.md](NOTICE.md) and [LICENSE](LICENSE).

Micropolis is a registered trademark of Micropolis Corporation (Micropolis GmbH)
and is used under the Micropolis Public Name License. SimCity is a trademark of
Electronic Arts Inc.; this modified port is not affiliated with or endorsed by
Electronic Arts.

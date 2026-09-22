#!/bin/bash
# A local runnable tree, not a public release/source distribution.
set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
source "$SCRIPT_DIR/env.sh"
[ -f "$BUILD_DIR/micropolis" ] || { echo "Run scripts/build-game.sh first" >&2; exit 1; }
DEST="${MICROPOLIS_STAGE_DIR:-$BUILD_DIR/package/Micropolis}"
mkdir -p "$DEST/sprites" "$DEST/Licenses" "$DEST/cities"
python3 "$SCRIPT_DIR/stage-audio.py" "$WORK_DIR/content/micropolis/sounds" "$DEST/sounds"
python3 "$SCRIPT_DIR/build-startup-art.py" "$DEST/startup"
cp "$WORK_DIR"/content/micropolis/cities/scenario_*.cty "$DEST/cities/"
cp "$WORK_DIR/content/micropolis/cities/about.cty" "$DEST/cities/"
cp "$PROJECT_ROOT/assets/classic-startup/COPYING" "$DEST/Licenses/ClassicStartup-COPYING"
cp "$PROJECT_ROOT/assets/classic-startup/NOTICE.txt" "$DEST/Licenses/ClassicStartup-NOTICE.txt"
cp "$BUILD_DIR/micropolis" "$DEST/Micropolis"
chmod +x "$DEST/Micropolis"
cp "$WORK_DIR/content/micropolis/tilesets/classic/tiles.bmp" "$DEST/tiles.bmp"
# This filename is a bundled sample, never an automatically overwritten user save.
cp "$WORK_DIR/content/micropolis/cities/haight.cty" "$DEST/CITY.CTY"
# After the cities exist: their project icons are written beside them.
python3 "$SCRIPT_DIR/build-workbench-icons.py" "$DEST"
cp "$WORK_DIR"/content/micropolis/images/sprite_*.png "$DEST/sprites/"
for name in LICENSE MicropolisGPLLicenseNotice.md MicropolisPublicNameLicense.md; do
    cp "$WORK_DIR/$name" "$DEST/Licenses/$name"
done
cp "$PROJECT_ROOT/assets/classic-tools/COPYING" "$DEST/Licenses/ClassicTools-COPYING"
cp "$PROJECT_ROOT/assets/classic-tools/NOTICE.txt" "$DEST/Licenses/ClassicTools-NOTICE.txt"
cp "$PROJECT_ROOT/assets/classic-graphs/COPYING" "$DEST/Licenses/ClassicGraphs-COPYING"
cp "$PROJECT_ROOT/assets/classic-graphs/NOTICE.txt" "$DEST/Licenses/ClassicGraphs-NOTICE.txt"
cp "$PROJECT_ROOT/assets/classic-overview/COPYING" "$DEST/Licenses/ClassicOverview-COPYING"
cp "$PROJECT_ROOT/assets/classic-overview/NOTICE.txt" "$DEST/Licenses/ClassicOverview-NOTICE.txt"
cat > "$DEST/ReadMe.txt" <<'README'
Micropolis for AROS - development build

Installing
Copy the entire Micropolis folder AND Micropolis.info to an installed disk.
Open the drawer and double-click the Micropolis application icon. No installer needed.
Double-clicking a city icon (for example CITY.CTY) opens that city directly.
Saved cities get their own icon, so they can be reopened the same way.
From a Shell: run the executable by its path; resources are found beside it.
Explicit relative city/tiles/sprite arguments are resolved from your current directory.

Starting
The classic startup screen selects/generates a city or one of eight scenarios.
Choose difficulty, edit the city name, then click Play. Load uses the native requester.
Micropolis CITY.CTY bypasses startup for direct launch.
Micropolis > Choose City (Amiga+N) returns to that screen during play.
If the running city has unsaved changes you are asked to Save, Discard or Cancel;
the same question protects Load City and Quit.

Playing
LMB builds/paints; Shift+LMB opens the classic tool pie menu.
RMB opens the application menu in every Micropolis window.
Middle mouse drags the map; arrows scroll. N pauses and steps 16 fast engine ticks;
Space runs/pauses at the selected speed.
Zoom: + and - (or the mouse wheel) switch between 50%, 100% and 200%; 0 restores 100%.
Chalk draws notes on the map, Eraser removes a whole chalk stroke. Chalk is free,
shows on the overview map, and is not saved with the city (as in the original).
Options > Chalk Overlay hides or shows it.
In a pie: hold, point, release to select; a short click leaves it open.
Left or right press/release selects a tool or submenu in the open pie. ESC cancels the pie.
Sound effects use ahi.device when available; missing audio does not prevent play.
There is no music: the original game has sound effects only.

Windows and keys
The menu bar contains Micropolis, Options, Disasters, Priority and Windows.
F1: commands; F2: budget; F3: evaluation; F4: messages; F5: graphs; F7: overview/R-C-I.
F6: classic Pause/Slow/Medium/Fast. Simulation uses an independent timer.
F9: mode; F10: screen/window. Window positions are remembered separately for the
Workbench and for the private screen. Graphs: six series, 10/120 years.
S/L: save/load. ESC: quit. Save on an installed writable disk, not RAM.

This folder includes a sample city and original sprite art from pinned MicropolisCore.
Licenses contains upstream notices. This is not a public release package.
README
printf 'stage-game: %s (%s)\n' "$DEST" "$AROS_TARGET"

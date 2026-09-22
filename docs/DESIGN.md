# Design notes

## Structure

The simulation is MicropolisCore, built natively without its Emscripten glue
(`emscripten.cpp` and the JS console callback are left out; the engine's own
`emscripten::val` stub covers the rest). Engine fixes are patches
(`patches/micropoliscore/`), never edits to a vendored copy.

| File | Role |
|---|---|
| `src/game.cpp` | event loop, commands, tools, map rendering and the engine callback |
| `src/display.cpp` | Workbench window or private screen, off-screen frame, resize |
| `src/view-geometry.h` | camera, hit testing and zoom (8/16/32 px per tile) |
| `src/classic-tools.h`, `src/classic-tool-ui.cpp` | original palette geometry and radial menus |
| `src/chalk-overlay.h` | Chalk/Eraser strokes and drawing |
| `src/startup-window.cpp`, `src/startup-model.h` | classic startup and scenario screen |
| `src/city-windows.cpp`, `src/budget-model.h` | budget, evaluation, commands, speed |
| `src/graph-window.cpp`, `src/graph-model.h` | history graphs |
| `src/overview-window.cpp`, `src/overview-model.h`, `src/small-map.h` | overview map and R/C/I |
| `src/message-window.cpp`, `src/messages.cpp` | message history and located pictures |
| `src/game-menu.cpp`, `src/game-command.h` | menu strip shared by all windows; one command dispatcher |
| `src/save-replace.cpp`, `src/unsaved-city.h` | crash-safe saving and the unsaved-changes question |
| `src/launch-directory.h` | Shell and Workbench (project icon) arguments |
| `src/game-audio.cpp` | AHI sound effects |
| `src/window-placement.h` | remembered window positions |

The reference for the interface is the **classic Tcl/Tk (OLPC) edition**
(github.com/SimHacker/micropolis, `micropolis-activity/`), not the web app.
Its images are reused unchanged; its behaviour is adapted to Intuition.

## Input

LMB builds; dragging paints roads, rails, wires and bulldozes. Crossing a tile
boundary starts a stroke even under the four-pixel click threshold; small
motion inside one tile, or with a building tool, stays a click. Middle mouse
pans; a middle press or an opening menu cancels a pending LMB action. RMB is
the Amiga menu button, so the radial tool menu moved to Shift+LMB (and the
clickable "Pie" line). Inside an open pie, ordinary LMB also selects after the
opening click: a usability addition to the original bindings.

## Rendering and zoom

Tiles come from the classic 512x480 8-bit `tiles.bmp` (960 tiles). A frame is
composed at 16 px per tile (tiles, sprites, chalk, tool footprint, earthquake
shift), then scaled once: doubled for 200%, averaged over 2x2 for 50%, so thin
road and rail lines survive. Zoom keeps the tile at the view centre. Only whole
tiles are shown; partial margins are cleared, never the map under them, which
avoids flicker. The classic editor has no zoom; power-of-two steps keep the
pixel art undistorted.

## Chalk and Eraser

As in `w_tool.c`: strokes are polylines in world pixels drawn 3 px wide; a
click leaves a dot. The eraser deletes every whole stroke that touches the
17x17 box around the pointer. Chalk costs nothing, is drawn on the overview at
3/16 scale and is not saved with the city (the original kept it in memory
only). Options > Chalk Overlay hides it. Loading or choosing a city clears it.

## Cities, saving and the unsaved question

Starting without arguments shows the classic startup screen (Generate, Load,
difficulty, eight scenarios, Play). Micropolis > Choose City opens it during
play; Quit/Esc there returns to the running city. A city argument or a
Workbench project icon skips it.

A save writes a temporary file on the destination volume, checks that it is
not empty, renames the previous file to an unused `.bak` and promotes the new
one; failures leave both files and name them. Save reuses the last path, Save
As always asks. Each save writes a project icon beside the city, unless one
exists, from `icons/def_city.info` with this program's absolute path as the
default tool.

"Unsaved" means that saving now would produce a different file than the last
save or load: the engine writes the city to a scratch file in RAM: and its
hash is compared. This covers edits, time, budget and saved options without
tracking each engine write. Pausing alone does not count. A freshly
generated map that was never saved counts as unsaved, because it exists nowhere
else; a loaded file or a built-in scenario starts out saved. Quit (menu, Esc,
close gadget), Load City and Choose City then ask Save / Discard / Cancel; a
cancelled or failed save keeps the city.

## Windows, menus and screens

Tools, Messages, Graphs and Overview are separate native windows rather than
the docked Tk layout. All of them carry the same menu strip (`Micropolis`,
`Options`, `Disasters`, `Priority`, `Windows`); picks reach the same
dispatcher as keys. Options show live engine flags; Messages and Notices are
frontend switches as in the classic Tcl code. Priority offers the port's four
real speeds (Pause/Slow/Medium/Fast) instead of the classic six labels, which
the timer model cannot distinguish. Air Crash is absent because the pinned
engine removed it.

F10 switches between a resizable Workbench window and a private screen; F9
picks any 24/32-bit mode of at least 640x480. Replacement windows are opened
before the old ones are closed, so a failure keeps the current display.
Window positions are remembered separately for the Workbench and for the
private screen and are clamped to keep each window fully visible.

## Other adaptations

- Prices come from the engine: Park $10 and Network $100, not the classic
  labels ($20, $1,000).
- Graphs: fixed 590x300 window; grid lines and year labels follow
  `w_graph.c` (every January over 10 years, every decade over 120). The three
  population series share a scale; cash flow is the engine's index (128 means
  zero), not the treasury.
- Messages show month and year; engine IDs stay internal. Located picture
  messages show a 128x128 crop of the city with its sprites.
- Startup previews sample tile centres instead of the Tk 3 px renderer. The
  original Tokyo card says 1967 while the engine starts in 1957; the footer
  shows the engine date.
- The classic $5 reward for fast pie selection is not reproduced.
- The simulation is paced by a 100 ms timer.device request re-armed after each
  batch; it does not emulate the Tcl timer exactly and never catches up after
  a dialog.
- Sound effects are the original samples through AHI, four voices; a fifth
  simultaneous effect is dropped. There is no music in the original.

## Verification scope

Host suites cover the models, the real engine and the real window code
against recording stand-ins. The native build is tested on AROS One 1.3
x86_64 (ABIv11) under QEMU with the std VGA driver at 1024x768x24. Not covered:
mainline runtime, other display drivers and resolutions, real hardware and
performance, multi-hour ordinary play.

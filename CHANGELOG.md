# Changelog

## 0.1.0-rc4 (2026-09-23)

- Give the map, toolbar, Messages, Graphs and Overview windows separate
  Intuition `Menu` objects. Attaching one menu strip to several windows at
  once violated the Intuition ownership contract and could leave Wanderer's
  application-menu state damaged after Micropolis exited.
- Add `IDCMP_MENUPICK` when each menu is attached, keep checked state in sync
  between the independent menus, and detach every menu before closing its
  window.

## 0.1.0-rc3 (2026-09-23)

First public release candidate, x86_64 AROS ABIv11.

- Native Intuition/CyberGraphics frontend over the pinned MicropolisCore
  engine with nine engine patches (uninitialised members, an off-by-one in
  problem voting, sprite construction and cleanup, checked saves, service
  funding, packed city fields, scenario load errors).
- Classic tool palette and radial Tool/Zone/Build menus; Chalk and Eraser
  annotations as in the original (free, not saved with the city).
- Map zoom 50/100/200% with `+`/`-`, the mouse wheel and the Windows menu.
- Classic startup and scenario screen, also available during play
  (Micropolis > Choose City); quitting, loading or choosing a city asks
  before unsaved changes are lost.
- Workbench project icons: double-click a city to open it; saved cities get
  their own icons.
- Application menu (Micropolis, Options, Disasters, Priority, Windows) in
  every window; Options mirror live engine settings.
- Budget, evaluation, history graphs with calendar ticks, overview map with
  fourteen views and R/C/I demand, message history with located pictures.
- Random and menu disasters, scenario win/loss, original sprites and AHI
  sound effects. No music: the original has none.
- Resizable Workbench window or a private screen in any 24/32-bit mode;
  window positions are remembered per display.
- Crash-safe saves: temporary file, then rename, keeping the previous file as
  a `.bak`.

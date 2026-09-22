#!/usr/bin/env python3
"""Write copy/paste fields for the current AROS Archives upload form."""
from pathlib import Path
import sys

out, version, binary_name, source_name = Path(sys.argv[1]), sys.argv[2], sys.argv[3], sys.argv[4]

text = f"""AROS Archives submission fields
===============================

Name: Micropolis
Description: Classic city-building simulation for AROS
Version: {version}
Author: Will Wright, Don Hopkins and Micropolis contributors; AROS port contributors
Submitter: (fill in on upload)
Email: (fill in on upload)
URL: https://github.com/tomaszstaniak/aros-micropolis
Category: game/strategy
Filename: {binary_name}
Requirements: x86_64 AROS ABIv11 (AROS One)
License: GPL
License notes: GPL-3.0-or-later with EA Section 7 terms; Micropolis Public Name License
Distribute: yes
Corresponding source archive: {source_name}

Readme text
-----------

Short:        Classic city-building simulation for AROS
Author:       Will Wright, Don Hopkins and Micropolis contributors;
              native AROS port by the AROS port contributors
Type:         game/strategy
Version:      {version}
Architecture: x86_64-aros-v11

Micropolis is the open-source city-building simulation descended from the
original SimCity Classic code. This is a modified native AROS frontend using
Intuition, CyberGraphics, ASL and AHI; it does not require SDL or Tcl/Tk.

This build targets x86_64 AROS ABIv11 and was tested on AROS One. It is not a
mainline-v1 binary and must not be presented as compatible with that ABI.

Install: extract the archive to an installed writable disk, keeping the
Micropolis drawer and Micropolis.info together, then double-click the icon.
No installer or external game data is required. Save cities to an installed
disk, not RAM:. Double-clicking a city icon opens that city; saved cities
get their own icons.

Features include the classic city editor with its original tool palette and
radial tool menus, chalk and eraser annotations, 50/100/200% map zoom, the
classic startup and scenario selection (also during play, with a prompt
before unsaved changes are lost), persistent save/load, budget, evaluation,
history graphs, overview map with R/C/I demand, located messages, disasters,
sprites, AHI sound effects, the application menu in every window, a
resizable Workbench window and an optional private screen that keeps your
window layout. There is no music: the original game has sound effects only.

The matching corresponding source is distributed as {source_name}. It contains
the native frontend, all AROS engine patches and the exact patched engine tree
used for this binary. The same source is published at
https://github.com/tomaszstaniak/aros-micropolis (tag v{version}).

Micropolis is a registered trademark of Micropolis Corporation (Micropolis
GmbH) and is licensed here as a courtesy of the owner under the Micropolis
Public Name License. SimCity is a trademark of Electronic Arts Inc.; this
modified port is not affiliated with or endorsed by Electronic Arts.
"""

out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(text)

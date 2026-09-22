#!/usr/bin/env python3
"""Write the live-catalogue overlay draft for the exact release LHA."""
from pathlib import Path
import hashlib
import sys

out, version, archive = Path(sys.argv[1]), sys.argv[2], Path(sys.argv[3])
archive_hash = hashlib.sha256(archive.read_bytes()).hexdigest()

# The GitHub release asset is the first public copy of the exact archive; an
# AROS Archives upload of the same bytes can replace url without other edits.
url = ("https://github.com/tomaszstaniak/aros-micropolis/releases/download/"
       f"v{version}/{archive.name}")

text = f'''# Draft for arospkg index/manifests/micropolis.x86_64.toml.
# Keep skeleton until the archive exists at url and the index checklist passes.
status          = "skeleton"
id              = "micropolis"
version         = "{version}"
revision        = 1
summary         = "Classic city-building simulation for AROS"
arch            = "x86_64"
abi             = "v11"
category        = "game/strategy"
url             = "{url}"
size            = {archive.stat().st_size}
sha256          = "{archive_hash}"
kind            = "app"

depends         = []
depends_checked = true
subdir          = "Micropolis"
icon            = "Micropolis.info"
installs_on     = ["aros-one"]
runs_on         = ["aros-one"]

[[requires_system]]
type = "library"
id   = "crt.library"

[[requires_system]]
type = "library"
id   = "m.library"

[[requires_system]]
type = "library"
id   = "stdlib.library"
'''
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(text)

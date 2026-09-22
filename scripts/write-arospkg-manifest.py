#!/usr/bin/env python3
"""Write the proposed author-supplied arospkg manifest."""
from pathlib import Path
import hashlib
import sys

out, version, source_archive, port_commit = Path(sys.argv[1]), sys.argv[2], Path(sys.argv[3]), sys.argv[4]
source_hash = hashlib.sha256(source_archive.read_bytes()).hexdigest()

text = f'''schema   = 0
id       = "micropolis"
name     = "Micropolis"
version  = "{version}"
revision = 1
summary  = "Classic city-building simulation for AROS"
category = "game/strategy"
license  = "GPL-3.0-or-later with EA Section 7 terms; Micropolis Public Name License"

depends = []
conflicts = []

[[requires_system]]
type = "library"
id   = "crt.library"

[[requires_system]]
type = "library"
id   = "m.library"

[[requires_system]]
type = "library"
id   = "stdlib.library"

[[targets]]
os   = "aros"
arch = "x86_64"
abi  = "v11"

[install]
subdir = "Micropolis"
icon   = "Micropolis.info"

[source]
archive    = "{source_archive.name}"
sha256     = "{source_hash}"
revision   = "{port_commit}"
built_with = "AROS One ABIv11 x86_64 SDK / GCC 10.5.0"
# built_on is deliberately absent: the local ABIv11 SDK snapshot does not
# contain a reliable SDK build date, and the proposed field means that date,
# not the date on which this package happened to be compiled.
'''
out.parent.mkdir(parents=True, exist_ok=True)
out.write_text(text)

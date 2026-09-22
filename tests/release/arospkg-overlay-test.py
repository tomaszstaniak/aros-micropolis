#!/usr/bin/env python3
from pathlib import Path
import hashlib
import sys
import tomllib

manifest_path, archive_path, version = Path(sys.argv[1]), Path(sys.argv[2]), sys.argv[3]
manifest = tomllib.loads(manifest_path.read_text())

assert manifest["status"] == "skeleton"  # approval follows the real public upload
assert manifest["id"] == "micropolis"
assert manifest["version"] == version
assert manifest["revision"] == 1
assert manifest["summary"] == "Classic city-building simulation for AROS"
assert manifest["arch"] == "x86_64"
assert manifest["abi"] == "v11"
assert manifest["category"] == "game/strategy"
assert manifest["url"] == (
    "https://github.com/tomaszstaniak/aros-micropolis/releases/download/"
    f"v{version}/micropolis.x86_64-aros-v11.lha"
)
assert manifest["size"] == archive_path.stat().st_size
assert manifest["sha256"] == hashlib.sha256(archive_path.read_bytes()).hexdigest()
assert manifest["kind"] == "app"
assert manifest["depends"] == []
assert manifest["depends_checked"] is True
assert manifest["subdir"] == "Micropolis"
assert manifest["icon"] == "Micropolis.info"
assert manifest["installs_on"] == ["aros-one"]
assert manifest["runs_on"] == ["aros-one"]
assert manifest["requires_system"] == [
    {"type": "library", "id": "crt.library"},
    {"type": "library", "id": "m.library"},
    {"type": "library", "id": "stdlib.library"},
]
print("PASS arospkg overlay draft")

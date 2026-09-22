#!/usr/bin/env python3
"""Validate the proposed author-supplied arospkg manifest in the LHA."""
from pathlib import Path
import hashlib
import sys
import tomllib

manifest_path, source_path, binary_path = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3])
commit, version = sys.argv[4], sys.argv[5]
assert manifest_path.as_posix().endswith("/.arospkg/manifest.toml")
manifest = tomllib.loads(manifest_path.read_text())

assert manifest["schema"] == 0
assert manifest["id"] == "micropolis"
assert manifest["name"] == "Micropolis"
assert manifest["version"] == version
assert manifest["revision"] == 1
assert manifest["summary"] == "Classic city-building simulation for AROS"
assert manifest["category"] == "game/strategy"
assert manifest["license"] == "GPL-3.0-or-later with EA Section 7 terms; Micropolis Public Name License"
assert manifest["targets"] == [{"os": "aros", "arch": "x86_64", "abi": "v11"}]
assert manifest["depends"] == []
assert manifest["requires_system"] == [
    {"type": "library", "id": "crt.library"},
    {"type": "library", "id": "m.library"},
    {"type": "library", "id": "stdlib.library"},
]
assert manifest["conflicts"] == []
assert manifest["install"] == {"subdir": "Micropolis", "icon": "Micropolis.info"}
assert "files" not in manifest  # Bundled scenarios are program data; saves live in SYS:Micropolis.

binary = binary_path.read_bytes()
for requirement in manifest["requires_system"]:
    assert requirement["id"].encode() in binary, requirement

source = manifest["source"]
assert source["archive"] == source_path.name
assert source["sha256"] == hashlib.sha256(source_path.read_bytes()).hexdigest()
assert source["revision"] == commit
assert source["built_with"] == "AROS One ABIv11 x86_64 SDK / GCC 10.5.0"
assert "built_on" not in source  # This SDK snapshot carries no reliable build date.
print("PASS embedded arospkg manifest")

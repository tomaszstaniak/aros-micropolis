#!/usr/bin/env python3
"""Validate the copy/paste metadata for the current AROS Archives form."""
from pathlib import Path
import re
import sys

path, version, binary, source = Path(sys.argv[1]), sys.argv[2], sys.argv[3], sys.argv[4]
text = path.read_text()

required = {
    "Name": "Micropolis",
    "Description": "Classic city-building simulation for AROS",
    "Version": version,
    "Category": "game/strategy",
    "Filename": binary,
    "Requirements": "x86_64 AROS ABIv11 (AROS One)",
    "License": "GPL",
    "License notes": "GPL-3.0-or-later with EA Section 7 terms; Micropolis Public Name License",
    "Distribute": "yes",
    "Corresponding source archive": source,
}
for key, expected in required.items():
    match = re.search(rf"^{re.escape(key)}:\s*(.*)$", text, re.MULTILINE)
    assert match, f"missing field: {key}"
    assert match.group(1) == expected, (key, match.group(1), expected)

assert re.fullmatch(r"[a-z0-9_-]+\.x86_64-aros-v11\.lha", binary), binary
assert "Short:        Classic city-building simulation for AROS" in text
assert "Architecture: x86_64-aros-v11" in text
assert "ABIv11" in text
assert "mainline" in text
assert "Micropolis is a registered trademark of Micropolis Corporation" in text
assert "Submitter: (fill in on upload)" in text
assert "Email: (fill in on upload)" in text
assert "URL: https://github.com/tomaszstaniak/aros-micropolis" in text
assert f"(tag v{version})" in text

limits = {
    "Name": 20,
    "Description": 50,
    "Version": 10,
    "Author": 90,
    "Submitter": 30,
    "Email": 40,
    "URL": 60,
    "Requirements": 200,
}
for key, limit in limits.items():
    value = re.search(rf"^{re.escape(key)}:\s*(.*)$", text, re.MULTILINE).group(1)
    assert len(value) <= limit, (key, len(value), limit)
print("PASS release submission metadata")

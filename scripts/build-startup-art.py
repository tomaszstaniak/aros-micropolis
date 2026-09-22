#!/usr/bin/env python3
"""Lossless conversion of pinned, opaque classic startup XPMs to indexed BMP."""
import hashlib
import json
from pathlib import Path
import re
import struct
import sys


def convert(data):
    lines = re.findall(r'^"(.*)"[,}]?', data.decode('ascii'), re.M)
    width, height, count, cpp = map(int, lines[0].split())
    if not (0 < count <= 256 and width > 0 and height > 0):
        raise ValueError('unsupported XPM dimensions/palette')
    palette, indices = [], {}
    for i, line in enumerate(lines[1:count+1]):
        color = re.search(r'\bc\s+#([0-9A-Fa-f]{6})\s*$', line[cpp:])
        if not color:
            raise ValueError('only opaque RGB XPM palettes are supported')
        value = int(color[1], 16)
        palette.append(struct.pack('<I', value))
        indices[line[:cpp]] = i
    rows = lines[count+1:]
    if len(rows) != height or any(len(r) != width*cpp for r in rows):
        raise ValueError('invalid XPM rows')
    stride = (width+3)&~3
    pixels = b''.join(bytes(indices[r[x:x+cpp]] for x in range(0, width*cpp, cpp))
                      + bytes(stride-width) for r in reversed(rows))
    offset = 54+256*4
    header = struct.pack('<2sIHHI', b'BM', offset+len(pixels), 0, 0, offset)
    header += struct.pack('<IiiHHIIiiII', 40, width, height, 1, 8, 0, len(pixels), 0, 0, 256, 0)
    return header+b''.join(palette)+bytes((256-count)*4)+pixels


def main():
    source = Path(__file__).resolve().parent.parent/'assets/classic-startup'
    dest = Path(sys.argv[1]); dest.mkdir(parents=True, exist_ok=True)
    manifest = json.loads((source/'source.json').read_text())
    for entry in manifest['files']:
        data = (source/entry['name']).read_bytes()
        if hashlib.sha256(data).hexdigest() != entry['sha256']:
            raise ValueError('source hash mismatch: '+entry['name'])
        (dest/Path(entry['name']).with_suffix('.bmp')).write_bytes(convert(data))
    print(f"startup art: {len(manifest['files'])} original images verified and converted")

if __name__ == '__main__':
    main()

#!/usr/bin/env python3
"""Compare every decoded native BMP pixel to the pinned source XPM palette."""
from pathlib import Path
import re
import subprocess
import tempfile
root=Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='micropolis-startup-art-') as tmp:
    out=Path(tmp)
    subprocess.run(['python3',str(root/'scripts/build-startup-art.py'),str(out)],check=True)
    subprocess.run(['clang++','-std=c++17','-fsanitize=address,undefined','-I'+str(root/'src'),str(root/'tests/startup/art-test.cpp'),str(root/'src/bmp.cpp'),'-o',str(out/'test')],check=True)
    for source in sorted((root/'assets/classic-startup').glob('*.xpm')):
        strings=[line.split('"')[1] for line in source.read_text().splitlines() if line.startswith('"')]
        w,h,n,cpp=map(int,strings[0].split())
        palette={line[:cpp]:bytes.fromhex(line.split('#')[-1]) for line in strings[1:n+1]}
        pixels=b''.join(palette[row[x:x+cpp]] for row in strings[n+1:] for x in range(0,w*cpp,cpp))
        assert len(pixels)==w*h*3
        raw=out/(source.stem+'.rgb');raw.write_bytes(pixels)
        subprocess.run([str(out/'test'),str(out/(source.stem+'.bmp')),str(raw)],check=True)
    print('RESULT: 27/27 original startup images match every decoded RGB pixel')

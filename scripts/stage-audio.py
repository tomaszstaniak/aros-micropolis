#!/usr/bin/env python3
"""Convert original upstream MP3 effects to the native PCM WAV contract."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

NAMES = ('ExplosionHigh', 'ExplosionLow', 'FogHornLow', 'HeavyTraffic',
         'HonkHonkHigh', 'HonkHonkLow', 'HonkHonkMed', 'Monster', 'Siren', 'Sorry', 'UhUh')

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path, help='upstream content/micropolis/sounds')
    parser.add_argument('destination', type=Path, help='staged sounds directory')
    args = parser.parse_args()
    ffmpeg = shutil.which('ffmpeg')
    if not ffmpeg:
        parser.error('ffmpeg is required to decode the original MP3 assets; install it first')
    missing = [name for name in NAMES if not (args.source / (name + '.mp3')).is_file()]
    if missing:
        parser.error('missing original upstream sounds: ' + ', '.join(missing))
    args.destination.mkdir(parents=True, exist_ok=True)
    manifest = []
    for name in NAMES:
        source = args.source / (name + '.mp3')
        output = args.destination / (name + '.wav')
        temporary = output.with_suffix('.tmp.wav')
        try:
            subprocess.run([ffmpeg, '-nostdin', '-v', 'error', '-y', '-i', str(source),
                            '-map_metadata', '-1', '-ac', '1', '-ar', '22050',
                            '-c:a', 'pcm_s16le', '-fflags', '+bitexact',
                            '-flags:a', '+bitexact', str(temporary)], check=True)
            temporary.replace(output)
        finally:
            temporary.unlink(missing_ok=True)
        manifest.append({'source': source.name, 'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
                         'output': output.name, 'output_sha256': hashlib.sha256(output.read_bytes()).hexdigest()})
    (args.destination / 'SOURCE.json').write_text(json.dumps({
        'origin': 'Pinned MicropolisCore content/micropolis/sounds; see upstreams.json and Licenses/',
        'conversion': 'ffmpeg decoded original MP3 to mono 22050 Hz signed 16-bit PCM WAV; no synthesized audio',
        'files': manifest}, indent=2) + '\n')
    print(f'Staged {len(manifest)} original sound effects in {args.destination}')

if __name__ == '__main__':
    main()

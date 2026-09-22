#!/usr/bin/env python3
"""Create the exact corresponding-source bundle for a binary release."""
from pathlib import Path
import json
import os
import subprocess
import sys
import zipfile


def git_files(repo: Path):
    raw = subprocess.check_output(["git", "-C", str(repo), "ls-files", "-z"])
    return [Path(item.decode()) for item in raw.split(b"\0") if item]


def add_file(archive, name, data, executable=False):
    info = zipfile.ZipInfo(name, (2026, 1, 1, 0, 0, 0))
    info.create_system = 3
    info.external_attr = (0o100755 if executable else 0o100644) << 16
    info.compress_type = zipfile.ZIP_DEFLATED
    archive.writestr(info, data)


def main():
    root, work, output, version = Path(sys.argv[1]), Path(sys.argv[2]), Path(sys.argv[3]), sys.argv[4]
    commit = subprocess.check_output(["git", "-C", str(root), "rev-parse", "HEAD"], text=True).strip()
    pin = json.loads((root / "upstreams.json").read_text())["repositories"]["micropoliscore"]["commit"]
    prefix = f"Micropolis-AROS-{version}-source/"
    output.parent.mkdir(parents=True, exist_ok=True)
    temporary = output.with_suffix(".zip.tmp")
    source_note = f"""Micropolis for AROS corresponding source

Version: {version}
Port commit: {commit}
MicropolisCore pin: {pin}

The aros-micropolis directory contains the native frontend, build/package
scripts, tests, notices and the complete nine-patch engine series. The exact
patched engine and runtime content used by this binary are also included at
aros-micropolis/work/micropoliscore, so the archive can be built offline once
the matching AROS SDK/toolchain is installed:

  cd aros-micropolis
  bash scripts/build-game.sh

The default target is AROS One ABIv11. The clean upstream pin and patch series
remain available for independent reproduction with scripts/bootstrap.sh.
"""
    try:
        with zipfile.ZipFile(temporary, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as archive:
            add_file(archive, prefix + "SOURCE-BUILD.txt", source_note.encode())
            for relative in git_files(root):
                path = root / relative
                executable = bool(path.stat().st_mode & 0o111)
                add_file(archive, prefix + "aros-micropolis/" + relative.as_posix(),
                         path.read_bytes(), executable)
            allowed = ("packages/micropolis-engine/", "content/micropolis/")
            exact = {"LICENSE", "MicropolisGPLLicenseNotice.md", "MicropolisPublicNameLicense.md"}
            for relative in git_files(work):
                name = relative.as_posix()
                if name not in exact and not name.startswith(allowed):
                    continue
                path = work / relative
                if not path.is_file():
                    continue
                executable = bool(path.stat().st_mode & 0o111)
                add_file(archive, prefix + "aros-micropolis/work/micropoliscore/" + name,
                         path.read_bytes(), executable)
        os.replace(temporary, output)
    finally:
        if temporary.exists():
            temporary.unlink()
    print(output)


if __name__ == "__main__":
    main()

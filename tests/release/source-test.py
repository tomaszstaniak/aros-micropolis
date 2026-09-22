#!/usr/bin/env python3
import json
import sys
import zipfile

archive, expected_commit, expected_version = sys.argv[1:]
with zipfile.ZipFile(archive) as source:
    assert source.testzip() is None
    names = set(source.namelist())
    root = f"Micropolis-AROS-{expected_version}-source/"
    required = {
        root + "SOURCE-BUILD.txt",
        root + "aros-micropolis/src/game.cpp",
        root + "aros-micropolis/scripts/build-game.sh",
        root + "aros-micropolis/scripts/package-release.sh",
        root + "aros-micropolis/scripts/write-archive-submission.py",
        root + "aros-micropolis/scripts/write-arospkg-manifest.py",
        root + "aros-micropolis/tests/release/submission-test.py",
        root + "aros-micropolis/tests/release/arospkg-manifest-test.py",
        root + "aros-micropolis/upstreams.json",
        root + "aros-micropolis/patches/micropoliscore/series",
        root + "aros-micropolis/work/micropoliscore/LICENSE",
        root + "aros-micropolis/work/micropoliscore/packages/micropolis-engine/src/micropolis.cpp",
        root + "aros-micropolis/work/micropoliscore/content/micropolis/tilesets/classic/tiles.bmp",
    }
    assert required <= names, sorted(required - names)
    assert all(root + f"aros-micropolis/patches/micropoliscore/{i:04d}" in "\n".join(names)
               for i in range(1, 10))
    assert not any("/.git/" in name or "/build/" in name or "/local/" in name or
                   name.endswith("/local.env") for name in names)
    build = source.read(root + "SOURCE-BUILD.txt").decode()
    assert f"Port commit: {expected_commit}" in build
    manifest = json.loads(source.read(root + "aros-micropolis/upstreams.json"))
    assert manifest["repositories"]["micropoliscore"]["commit"] in build
print("PASS source archive: exact identity, build inputs, nine patches and patched engine source")

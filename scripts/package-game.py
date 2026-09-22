#!/usr/bin/env python3
"""Archive a fresh staging tree, with target identity and full payload hashes."""
from pathlib import Path
import hashlib, os, subprocess, sys, zipfile

def main():
    stage,output,abi,repo=Path(sys.argv[1]),Path(sys.argv[2]),sys.argv[3],Path(sys.argv[4])
    commit=subprocess.check_output(["git","-C",str(repo),"rev-parse","HEAD"],text=True).strip()
    dirty=bool(subprocess.check_output(["git","-C",str(repo),"status","--porcelain"],text=True).strip())
    (stage/"Micropolis/BUILD.txt").write_text(f"Micropolis for AROS — local development package\nArchitecture: x86_64\nABI: {abi}\nSource baseline: {commit}\nUncommitted changes at packaging: {dirty}\nDo not run a package built for another ABI.\n")
    paths=sorted(p for p in stage.rglob("*") if p.is_file())
    (stage/"Micropolis/SHA256SUMS").write_text("".join(hashlib.sha256(p.read_bytes()).hexdigest()+"  "+p.relative_to(stage).as_posix()+"\n" for p in paths))
    output.parent.mkdir(parents=True,exist_ok=True)
    temporary=output.with_suffix(".zip.tmp")
    try:
        with zipfile.ZipFile(temporary,"w",zipfile.ZIP_DEFLATED,compresslevel=9) as z:
            for p in sorted(stage.rglob("*")):
                if not p.is_file():continue
                name=p.relative_to(stage).as_posix()
                info=zipfile.ZipInfo(name,(2026,1,1,0,0,0))
                info.create_system=3
                info.external_attr=(0o100755 if name=="Micropolis/Micropolis" else 0o100644)<<16
                info.compress_type=zipfile.ZIP_DEFLATED
                z.writestr(info,p.read_bytes())
        os.replace(temporary,output)
    finally:
        if temporary.exists():temporary.unlink()
    print(output)
if __name__=="__main__":main()

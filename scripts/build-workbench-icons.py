#!/usr/bin/env python3
"""AROS PNG icons from unchanged classic R/C/I art; no image library needed.

Metadata format follows AROS workbench/libs/icon/diskobjPNGio.c. Two PNGs
encode normal/selected images; icOn integer tags are big endian.
"""
import hashlib, importlib.util, json, struct, sys, zlib
from pathlib import Path
spec=importlib.util.spec_from_file_location("classic",Path(__file__).with_name("build-classic-art.py"))
classic=importlib.util.module_from_spec(spec);spec.loader.exec_module(classic)

def chunk(name,data):
    return struct.pack(">I",len(data))+name+data+struct.pack(">I",zlib.crc32(name+data)&0xffffffff)

def icon_image(selected,kind,tools=("res","com","ind"),default_tool=None):
    assets=Path(__file__).resolve().parents[1]/"assets/classic-tools"
    hashes={x["name"]:x["sha256"] for x in json.loads((assets/"source.json").read_text())["files"]}
    images=[]
    for tool in tools:
        p=assets/("ic"+tool+("hi" if selected else "")+".xpm")
        if hashlib.sha256(p.read_bytes()).hexdigest()!=hashes[p.name]:raise ValueError("Art hash mismatch")
        images.append(classic.decode(p))
    width=sum(w for w,h,p in images);height=images[0][1]
    if any(h!=height for w,h,p in images):raise ValueError("Mismatched icon heights")
    raw=bytearray()
    for y in range(height):
        raw.append(0)
        for w,h,pixels in images:
            for pixel in pixels[y*w:(y+1)*w]:raw.extend(((pixel>>16)&255,(pixel>>8)&255,pixel&255,255))
    tags=[(0x8000100f,kind),(0x80001009,1048576)]
    if kind==2:tags.extend([(0x80001005,540),(0x80001006,300)])
    metadata=b"".join(struct.pack(">II",*t) for t in tags)
    # String attributes are the tag followed by a NUL-terminated string
    # (diskobjPNGio.c, ATTR_DEFAULTTOOL).
    if default_tool:metadata+=struct.pack(">I",0x8000100a)+default_tool.encode("latin-1")+b"\0"
    return (b"\x89PNG\r\n\x1a\n"+chunk(b"IHDR",struct.pack(">IIBBBBB",width,height,8,6,0,0,0))+
            chunk(b"icOn",metadata)+chunk(b"IDAT",zlib.compress(raw))+chunk(b"IEND",b""))

def project(tool):
    """City project icon (WBPROJECT=4): one residential zone, default tool."""
    return icon_image(False,4,("res",),tool)+icon_image(True,4,("res",),tool)

def main():
    dest=Path(sys.argv[1]);dest.mkdir(parents=True,exist_ok=True)
    (dest/"Micropolis.info").write_bytes(icon_image(False,3)+icon_image(True,3))
    dest.with_suffix(".info").write_bytes(icon_image(False,2)+icon_image(True,2))
    # The game copies this template beside saved cities and replaces the
    # default tool with its own absolute path.
    (dest/"icons").mkdir(exist_ok=True)
    (dest/"icons/def_city.info").write_bytes(project("Micropolis"))
    # Bundled cities: Workbench resolves a relative default tool from the
    # project's drawer.
    if (dest/"CITY.CTY").exists():(dest/"CITY.CTY.info").write_bytes(project("Micropolis"))
    if (dest/"cities/about.cty").exists():(dest/"cities/about.cty.info").write_bytes(project("/Micropolis"))
if __name__=="__main__":main()

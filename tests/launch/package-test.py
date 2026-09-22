#!/usr/bin/env python3
"""Inspect the delivered archive, rather than a separate hand-built fixture."""
import hashlib,struct,sys,zipfile,zlib,io,json,wave
from pathlib import Path

STRING_TAGS={0x80001008,0x8000100a,0x8000100b}  # diskobjPNGio.c string attributes
def icon_tags(data):
    tags={};i=0
    while i<len(data):
        tag=struct.unpack_from(">I",data,i)[0];i+=4
        if tag in STRING_TAGS:
            end=data.index(b"\0",i);tags[tag]=data[i:end].decode("latin-1");i=end+1
        else:
            tags[tag]=struct.unpack_from(">I",data,i)[0];i+=4
    return tags

def pngs(blob,kind,default_tool=None):
    offset=0
    for selected in range(2):
        assert blob[offset:offset+8]==b"\x89PNG\r\n\x1a\n";offset+=8
        tags={};pixels=b"";width=height=0
        while True:
            n=struct.unpack_from(">I",blob,offset)[0];name=blob[offset+4:offset+8]
            data=blob[offset+8:offset+8+n];crc=struct.unpack_from(">I",blob,offset+8+n)[0]
            assert zlib.crc32(name+data)&0xffffffff==crc
            if name==b"IHDR":width,height=struct.unpack_from(">II",data)
            if name==b"icOn":tags=icon_tags(data)
            if name==b"IDAT":pixels+=data
            offset+=12+n
            if name==b"IEND":break
        assert tags[0x8000100f]==kind and tags[0x80001009]>=1048576
        assert tags.get(0x8000100a)==default_tool
        assert len(zlib.decompress(pixels))==height*(1+width*4)
        assert width>0 and height>0
    assert offset==len(blob)

with zipfile.ZipFile(sys.argv[1]) as z:
    assert z.testzip() is None
    names=set(z.namelist())
    for path in ['Micropolis.info','Micropolis/Micropolis.info','Micropolis/Micropolis','Micropolis/tiles.bmp','Micropolis/BUILD.txt','Micropolis/Licenses/LICENSE']:assert path in names,path
    assert len([n for n in names if n.startswith('Micropolis/startup/')])==27
    assert len([n for n in names if n.startswith('Micropolis/cities/scenario_')])==8
    assert any(n.startswith('Micropolis/sprites/sprite_') for n in names)
    sounds=json.loads(z.read('Micropolis/sounds/SOURCE.json'))['files']
    assert len(sounds)==11
    for sound in sounds:
        data=z.read('Micropolis/sounds/'+sound['output'])
        assert hashlib.sha256(data).hexdigest()==sound['output_sha256']
        with wave.open(io.BytesIO(data),'rb') as wav:
            assert (wav.getnchannels(),wav.getsampwidth(),wav.getframerate())==(1,2,22050)
            assert wav.getnframes()>0
    assert not any('smoke' in n for n in names), 'test-only executables must not ship'
    listed=set()
    for line in z.read('Micropolis/SHA256SUMS').decode().splitlines():
        digest,path=line.split('  ',1);assert hashlib.sha256(z.read(path)).hexdigest()==digest;listed.add(path)
    assert listed==names-{'Micropolis/SHA256SUMS'}
    assert z.getinfo('Micropolis/Micropolis').external_attr>>16&0o111
    pngs(z.read('Micropolis.info'),2);pngs(z.read('Micropolis/Micropolis.info'),3)
    # City project icons: relative tools resolve from the project's drawer;
    # the template's tool is replaced at save time with an absolute path.
    pngs(z.read('Micropolis/CITY.CTY.info'),4,'Micropolis')
    pngs(z.read('Micropolis/cities/about.cty.info'),4,'/Micropolis')
    pngs(z.read('Micropolis/icons/def_city.info'),4,'Micropolis')
    print('PASS archive: all payload hashes, executable permissions, assets, two-state drawer/tool/project PNG icons')

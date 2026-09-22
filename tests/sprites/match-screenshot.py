import struct,zlib,sys
from pathlib import Path

def png(p):
 b=p.read_bytes();at=8;data=b'';w=h=0
 while at<len(b):
  n=struct.unpack('>I',b[at:at+4])[0];tag=b[at+4:at+8];chunk=b[at+8:at+8+n];at+=12+n
  if tag==b'IHDR':w,h,depth,color,*_=struct.unpack('>IIBBBBB',chunk);assert depth==8 and color==6
  if tag==b'IDAT':data+=chunk
 raw=zlib.decompress(data);rows=[];prior=bytearray(w*4);i=0
 for y in range(h):
  kind=raw[i];i+=1;row=bytearray(raw[i:i+w*4]);i+=w*4
  for x in range(w*4):
   a=row[x-4] if x>=4 else 0;c=prior[x-4] if x>=4 else 0;b=prior[x]
   if kind==1:v=a
   elif kind==2:v=b
   elif kind==3:v=(a+b)//2
   elif kind==4:
    p=a+b-c;pa,pb,pc=abs(p-a),abs(p-b),abs(p-c);v=a if pa<=pb and pa<=pc else (b if pb<=pc else c)
   else:v=0
   row[x]=(row[x]+v)&255
  rows.append(row);prior=row
 return w,h,b''.join(rows)

import argparse
ap=argparse.ArgumentParser(description='Match visible opaque pixels of pinned sprite frames in a QEMU P6 screenshot. Positive evidence only; clipping may hide the candidate anchor.')
ap.add_argument('screenshot',type=Path);ap.add_argument('art',type=Path)
ap.add_argument('--viewport',nargs=4,type=int,required=True,metavar=('X','Y','W','H'))
a=ap.parse_args();vx,vy,vw,vh=a.viewport
b=a.screenshot.read_bytes();head=b.split(b'\n',3);assert head[0]==b'P6';sw,sh=map(int,head[1].split());screen=head[3]
art=a.art
assert head[2]==b'255' and len(screen)==sw*sh*3
assert 0<=vx<vx+vw<=sw and 0<=vy<vy+vh<=sh
for f in sorted(art.glob('sprite_*.png')):
 w,h,data=png(f)
 opaque=[(x,y,data[(y*w+x)*4:(y*w+x)*4+3]) for y in range(h) for x in range(w) if data[(y*w+x)*4+3]==255]
 if not opaque:continue
 # Find candidates using a rare opaque art colour, then verify every opaque pixel.
 from collections import Counter
 counts=Counter(c for x,y,c in opaque);px,py,color=min(opaque,key=lambda p:counts[p[2]])
 pos=-1
 while True:
  pos=screen.find(color,pos+1)
  if pos<0:break
  if pos%3:continue
  xx=(pos//3)%sw-px;yy=(pos//3)//sw-py
  if not (vx-w<xx<vx+vw and vy-h<yy<vy+vh):continue
  visible=[(x,y,c) for x,y,c in opaque if vx<=xx+x<vx+vw and vy<=yy+y<vy+vh]
  if len(visible)<30:continue
  if all(screen[((yy+y)*sw+xx+x)*3:((yy+y)*sw+xx+x)*3+3]==c for x,y,c in visible):
   print(f.name,'at',xx,yy,'visible opaque pixels matched',len(visible),'of',len(opaque))

#!/usr/bin/env python3
"""Trace the actual native presentation block: no clear may erase the map.

The graphics stubs record intermediate damage before the final blit, which
a final screenshot alone cannot reveal. No alternate renderer is tested.
"""
from pathlib import Path
import os, subprocess, tempfile
root=Path(__file__).resolve().parents[2]
source=(root/'src/game.cpp').read_text()
start=source.index('            // Clear the partial-tile margins')
end=source.index('            needRender = false;',start)
body=source[start:end]
fixture=r'''
#include <algorithm>
#include <cstdio>
#include <vector>
struct Rect {int l,t,r,b;};
static std::vector<Rect> clears;
static int blits;
struct Window {int BorderLeft=5,BorderRight=7,BorderTop=23,BorderBottom=9;
 int Width,Height;void *RPort=nullptr;};
static void SetAPen(void*,int) {}
static void RectFill(void*,int l,int t,int r,int b){clears.push_back({l,t,r,b});}
static void BltBitMapRastPort(void*,int,int,void*,int,int,int,int,int){++blits;}
static void present(Window *win,int VIEW_W,int VIEW_H) {
 void *offBm=nullptr;
BODY
}
int main(){int n=0,failed=0;
 auto check=[&](bool ok,const char *s){++n;failed+=!ok;printf("%s %s\n",ok?"PASS":"FAIL",s);};
 for(auto inner:std::vector<Rect>{{0,0,640,432},{0,0,647,439},{0,0,1932,1612},{0,0,320,199},{0,0,327,192}}){
  Window w;w.Width=inner.r+w.BorderLeft+w.BorderRight;w.Height=inner.b+w.BorderTop+w.BorderBottom;
  const int width=std::min(inner.r/16,120)*16,height=std::min(inner.b/16,100)*16;
  clears.clear();blits=0;present(&w,width,height);
  bool inside=true,overlap=false,covered=true;
  for(auto r:clears){
   inside &= r.l>=w.BorderLeft && r.t>=w.BorderTop && r.r<w.Width-w.BorderRight && r.b<w.Height-w.BorderBottom && r.l<=r.r && r.t<=r.b;
   overlap |= r.l<w.BorderLeft+width && r.t<w.BorderTop+height && r.r>=w.BorderLeft && r.b>=w.BorderTop;
  }
  for(int y=0;y<inner.b;y++)for(int x=0;x<inner.r;x++)if(x>=width || y>=height){
   bool hit=false;for(auto r:clears)hit|=x+w.BorderLeft>=r.l && x+w.BorderLeft<=r.r && y+w.BorderTop>=r.t && y+w.BorderTop<=r.b;
   covered &= hit;
  }
  check(!overlap,"no intermediate background clear overlaps visible map");
  check(inside,"margin clears stay inside client area with valid rectangles");
  check(covered,"all partial-tile or beyond-world margins are cleared");
  check(blits==1,"completed frame is presented once");
 }
 printf("RESULT: %d/%d passed\n",n-failed,n);return failed?1:0;
}
'''.replace('BODY',body)
with tempfile.TemporaryDirectory(prefix='micropolis-present-') as d:
    cpp=Path(d)/'test.cpp';cpp.write_text(fixture);exe=Path(d)/'test'
    subprocess.run([os.environ.get('CXX','clang++'),'-std=c++17','-Wall','-Wextra',
                    '-fsanitize=address,undefined',str(cpp),'-o',str(exe)],check=True)
    raise SystemExit(subprocess.run([str(exe)]).returncode)

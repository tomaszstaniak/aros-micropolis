#!/usr/bin/env python3
"""Compile the actual frontend mouse switch against a recording engine.
Only native window/event transport is omitted: assertions concern engine calls.
"""
from pathlib import Path
import os, subprocess, tempfile
root=Path(os.environ.get('MICROPOLIS_TEST_ROOT',Path(__file__).resolve().parents[2]))
source=(root/'src/game.cpp').read_text()
start=source.index('                case IDCMP_MOUSEBUTTONS:')
end=source.index('                case IDCMP_INTUITICKS:',start)
handler=source[start:end]
fixture=r'''
#include <cstdio>
#include <cstdlib>
#include "view-geometry.h"
#include "chalk-overlay.h"
#define TILE_SIZE 16
#define WORLD_W 120
#define WORLD_H 100
#define VIEW_W (view.width)
#define VIEW_H (view.height)
#define VIEW_TILES_W (view.columns())
#define VIEW_TILES_H (view.rows())
enum {IDCMP_MOUSEBUTTONS,IDCMP_MOUSEMOVE};
enum {SELECTDOWN,SELECTUP,MENUDOWN,MENUUP,MIDDLEDOWN,MIDDLEUP};
enum {IEQUALIFIER_LSHIFT=1,IEQUALIFIER_RSHIFT=2};
enum {TOOL_ROAD,TOOL_RAILROAD,TOOL_WIRE,TOOL_BULLDOZER,TOOL_RESIDENTIAL};
struct Engine { int calls=0; int x0=-1,y0=-1,x1=-1,y1=-1; long totalFunds=1000000;
 void toolDrag(int,short ax,short ay,short bx,short by) {
  ++calls;x0=ax;y0=ay;x1=bx;y1=by;
 } };
static int clicks;
struct Window {int LeftEdge=10,TopEdge=20,BorderLeft=4,BorderTop=12;};
struct Callback {bool dialogShown=false;ChalkOverlay chalk;};
enum {ANNOTATE_NONE,ANNOTATE_CHALK,ANNOTATE_ERASER};
static int annotation=ANNOTATE_NONE;
static int currentAnnotation(){return annotation;}
static int pieCalls,pieResult=-1,pieX,pieY,selections,selected;
static bool pieLeft;
static int showClassicPie(Window*,int x,int y,bool left) {
 ++pieCalls;pieX=x;pieY=y;pieLeft=left;return pieResult;
}
static void selectSlot(int slot,void*,Engine*,void*,bool,const char**,char*,int*,int*) {
 ++selections;selected=slot;
}
static int currentTool(){return TOOL_ROAD;}
static void buildAt(int,int,int,void*,Engine*,void*,bool,const char**,char*) { ++clicks; }
static void updateTitle(void*,Engine*,void*,bool,const char*,char*) {}
struct Input {
 ViewGeometry view{640,432};
 int camX=40,camY=30,dragLastX=0,dragLastY=0,paintTileX=-1,paintTileY=-1;
 int hoverVX=-1,hoverVY=-1,tool=TOOL_ROAD;
 bool dragging=false,dragMoved=false,panDragging=false,buildCancelled=false;
 bool needRender=false,running=false,shiftHeld=false;
 Engine engine;Engine *micropolis=&engine;
 Window window;Window *win=&window;Callback callback;Callback *cb=&callback;
 int footW=1,footH=1;
 char feedback[2048]={}; const char *toolNames[5]={"road","rail","wire","dozer","zone"};
 void event(int cls,int code,int mx,int my,int qualifier=0) {switch(cls) {
HANDLER
 }}
 void down(int x,int y){event(IDCMP_MOUSEBUTTONS,SELECTDOWN,x,y);}
 void up(int x,int y){event(IDCMP_MOUSEBUTTONS,SELECTUP,x,y);}
 void move(int x,int y){event(IDCMP_MOUSEMOVE,0,x,y);}
 void rdown(int x,int y){event(IDCMP_MOUSEBUTTONS,MENUDOWN,x,y);}
 void rup(int x,int y){event(IDCMP_MOUSEBUTTONS,MENUUP,x,y);}
 void mdown(int x,int y){event(IDCMP_MOUSEBUTTONS,MIDDLEDOWN,x,y);}
 void mup(int x,int y){event(IDCMP_MOUSEBUTTONS,MIDDLEUP,x,y);}
};
int main(){
 int n=0,failed=0;
 auto check=[&](bool ok,const char*s){++n;failed+=!ok;printf("%s %s\n",ok?"PASS":"FAIL",s);};
 for(int tool=0;tool<4;++tool){
  clicks=0;Input a;a.tool=tool;a.down(644,200);a.move(620,200);a.up(620,200);
  check(a.engine.calls==0 && clicks==0,"press in right margin cannot paint or click into city");
  clicks=0;Input b;b.tool=tool;b.down(100,440);b.move(100,410);b.up(100,410);
  check(b.engine.calls==0 && clicks==0,"press in bottom margin cannot paint or click into city");
  clicks=0;Input c;c.tool=tool;c.down(100,100);c.move(132,100);c.up(132,100);
  check(c.engine.calls==1 && clicks==0,"normal in-map painting still calls the engine");
  clicks=0;Input d;d.tool=tool;d.down(100,100);d.rdown(100,100);d.rup(100,100);d.move(132,100);d.up(132,100);
  check(d.engine.calls==0 && clicks==0,"RMB cancels existing LMB gesture through release");
  clicks=0;Input e;e.tool=tool;e.mdown(100,100);e.down(100,100);e.mup(100,100);e.move(132,100);e.up(132,100);
  check(e.engine.calls==0 && clicks==0,"LMB starting during middle pan stays cancelled");
  clicks=0;Input f;f.tool=tool;f.down(100,100);f.mdown(100,100);f.mup(100,100);f.move(132,100);f.up(132,100);
  check(f.engine.calls==0 && clicks==0,"middle pan cancels pending click and paint");
 }
 clicks=0;Input a;a.down(100,100);a.up(100,100);check(clicks==1,"normal click builds once");
 clicks=0;Input b;b.up(100,100);check(clicks==0,"unpaired release cannot build");
 clicks=0;Input edge;edge.down(111,100);edge.move(113,100);edge.up(113,100);
 check(edge.engine.calls==1 && clicks==0 &&
       edge.engine.x0==46 && edge.engine.x1==47,
       "crossing a tile boundary by two pixels paints both line tiles");
 clicks=0;Input same;same.down(100,100);same.move(103,100);same.up(103,100);
 check(same.engine.calls==0 && clicks==1,
       "small movement inside one tile remains a click");
 clicks=0;Input building;building.tool=TOOL_RESIDENTIAL;
 building.down(111,100);building.move(113,100);building.up(113,100);
 check(building.engine.calls==0 && clicks==1,
       "small boundary crossing with a building tool remains a click");
 Input pan;pan.mdown(100,100);
 for(int x=101;x<=260;++x)pan.move(x,100);
 pan.mup(260,100);
 check(pan.camX==30 && pan.engine.calls==0,"160 one-pixel middle moves pan ten tiles without painting");
 // RMB belongs to the native Intuition menu (2026-09-21): no pie, no build.
 clicks=pieCalls=0;{Input rmb;rmb.rdown(100,80);rmb.rup(100,80);
  check(pieCalls==0 && clicks==0 && rmb.engine.calls==0,"RMB is left to the native menu");}
 for(int shift : {int(IEQUALIFIER_LSHIFT),int(IEQUALIFIER_RSHIFT)}) {
  clicks=pieCalls=selections=0;pieResult=-1;Input menu;
  menu.event(IDCMP_MOUSEBUTTONS,SELECTDOWN,100,80,shift);
  menu.move(164,80);menu.up(164,80);
  check(pieCalls==1 && pieLeft,"either Shift+LMB opens the pie with the left-button trigger");
  check(pieX==114 && pieY==112,"pie receives absolute screen coordinates including borders");
  check(clicks==0 && menu.engine.calls==0 && menu.camX==40,"cancelled pie neither builds nor pans");
  check(menu.callback.dialogShown && selections==0,"pie requests stale input drain; cancellation preserves selection");
 }
 clicks=pieCalls=0;{Input held;held.shiftHeld=true;held.down(100,80);
  check(pieCalls==1 && clicks==0,"tracked Shift key opens the pie when the qualifier is missing");}
 pieResult=9;selections=0;Input chosen;chosen.event(IDCMP_MOUSEBUTTONS,SELECTDOWN,100,80,IEQUALIFIER_LSHIFT);
 check(selections==1 && selected==9,"pie choice uses shared palette selection path");
 pieResult=-1;
 // Chalk and eraser (classic w_tool.c): world pixels, press and drag only.
 annotation=ANNOTATE_CHALK;clicks=0;
 {Input c;c.down(100,100);c.move(120,110);c.move(140,110);c.up(140,110);
  const auto &s=c.callback.chalk.strokes();
  check(s.size()==1 && s[0].length()==3 && s[0].points[0]==40*16+100 && s[0].points[1]==30*16+100 &&
        s[0].points[4]==40*16+140,"chalk stroke records world pixels from press through drag");
  check(clicks==0 && c.engine.calls==0 && !c.callback.chalk.drawing(),"chalk never builds and release ends the stroke");}
 {Input z;z.view=ViewGeometry::fromInner(640,432,32);z.down(64,32);z.move(96,32);z.up(96,32);
  const auto &s=z.callback.chalk.strokes();
  check(s.size()==1 && s[0].points[0]==40*16+32 && s[0].points[2]==40*16+48,"chalk at 200% maps screen pixels to world pixels");}
 {Input m;m.down(644,200);m.up(644,200);check(m.callback.chalk.strokes().empty(),"chalk press in the margin draws nothing");}
 {Input p;p.down(100,100);p.mdown(100,100);p.move(160,100);p.mup(160,100);p.move(170,100);p.up(170,100);
  check(p.callback.chalk.strokes().size()==1 && p.callback.chalk.strokes()[0].length()==1,
        "middle pan ends the chalk stroke; later motion does not extend it");}
 {Input e;e.down(100,100);e.move(140,100);e.up(140,100);
  annotation=ANNOTATE_ERASER;e.down(300,300);e.up(300,300);
  check(e.callback.chalk.strokes().size()==1,"eraser far from a stroke keeps it");
  e.down(300,300);e.move(120,104);e.up(120,104);
  check(e.callback.chalk.strokes().empty() && clicks==0,"eraser drag removes the whole touched stroke");}
 annotation=ANNOTATE_NONE;
 {Input zp;zp.view=ViewGeometry::fromInner(640,432,32);zp.mdown(100,100);
  for(int x=101;x<=260;++x)zp.move(x,100);zp.mup(260,100);
  check(zp.camX==35,"middle pan at 200% moves one tile per 32 screen pixels");}
 printf("RESULT: %d/%d passed\n",n-failed,n);return failed?1:0;
}
'''.replace('HANDLER',handler)
with tempfile.TemporaryDirectory(prefix='micropolis-gestures-') as d:
 cpp=Path(d)/'test.cpp';cpp.write_text(fixture)
 exe=Path(d)/'test'
 subprocess.run([os.environ.get('CXX','clang++'),'-std=c++17','-fsanitize=address,undefined','-I',str(root/'src'),str(cpp),'-o',str(exe)],check=True)
 raise SystemExit(subprocess.run([str(exe)]).returncode)

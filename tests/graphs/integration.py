#!/usr/bin/env python3
"""Compile the actual native graph window against a recording Intuition/RTG shim.

Only transport is simulated: use real graph code, icon decoder and model.
The shared native shim is read as a literal, never import/run the message test.
"""
import ast
import os
from pathlib import Path
import subprocess
import tempfile

root=Path(__file__).resolve().parents[2]
tree=ast.parse((root/'tests/messages/integration.py').read_text())
stub=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign)
          and any(isinstance(t,ast.Name) and t.id=='stub' for t in n.targets))
stub+=r'''
#include <cassert>
#include <algorithm>
using APTR=void *;
constexpr int RECTFMT_BGRA32=1;
struct Blit {int x,y,w,h;std::vector<uint32_t> pixels;};
inline std::vector<Blit> blits;
inline unsigned WritePixelArray(APTR data,int sx,int sy,int pitch,RastPort *,int x,int y,int w,int h,int format) {
    assert(format==RECTFMT_BGRA32 && sx==0 && sy==0 && pitch==w*4);
    const auto *p=(const uint32_t *)data;
    blits.push_back({x,y,w,h,{p,p+w*h}});
    return w*h;
}
'''
fixture=r'''
#include "native-stub.h"
#include "graph-window.h"
#include "micropolis.h"
#include <cstdio>
int checks=0,failures=0;
void check(bool ok,const char *label){++checks;if(!ok)++failures;printf("%s %s\n",ok?"PASS":"FAIL",label);}
void event(Window *w,int cls,int code=0,int x=0,int y=0){
    auto *m=new IntuiMessage;m->Class=cls;m->Code=code;
    m->MouseX=x+w->BorderLeft;m->MouseY=y+w->BorderTop;w->UserPort->queue.push_back(m);
}
bool text(Window *w,const char *s){for(auto &v:w->RPort->output)if(v.find(s)!=std::string::npos)return true;return false;}
int main(){
    TextAttr font={(STRPTR)"topaz.font",8,0,0};Screen screen;screen.Font=&font;
    Window parent;parent.WScreen=&screen;Micropolis city;GraphWindow ui;
    check(!ui.open(nullptr,city),"missing parent refuses without resources");
    screen.Width=320;check(!ui.open(&parent,city) && opens==0 && fonts==0,"small screen refuses without leaking");
    screen.Width=640;screen.Height=480;
    city.history[0][0][0]=100;city.history[0][0][119]=20;
    city.history[1][0][0]=30;city.history[1][0][119]=60;
    check(ui.open(&parent,city),"graph fits native 640x480 screen");
    auto *w=ui.window();
    check(text(w,"10-year history") && text(w,"Cash flow: index 128"),"monthly default and correct money semantics visible");
    check(blits.size()==15 && blits[0].w==466 && blits[0].h==180,"single complete plot followed by eight icons and six swatches");
    auto monthly=blits[0].pixels;
    const auto selected=blits[1].pixels;
    auto before=blits.size();ui.refresh(city);
    check(blits.size()==before,"unchanged snapshot does not redraw");
    event(w,IDCMP_MOUSEBUTTONS,SELECTDOWN,20,90);ui.poll(city);
    check(blits[before+1].pixels!=selected,"series click changes selected original icon");
    before=blits.size();event(w,IDCMP_MOUSEBUTTONS,SELECTDOWN,20,90);ui.poll(city);
    check(blits[before].pixels==monthly,"retoggling series restores exact graph");
    before=blits.size();event(w,IDCMP_MOUSEBUTTONS,SELECTDOWN,20,50);ui.poll(city);
    check(text(w,"120-year history") && blits[before].pixels!=monthly,"annual selector uses the other history bank");
    before=blits.size();city.history[1][0][20]=90;ui.refresh(city);
    check(blits.size()==before+15,"loaded or simulated history refreshes visible chart");
    before=blits.size();city.cityTime+=4;ui.refresh(city);
    check(blits.size()==before+15 && text(w,"2/1900"),"date refreshes even if sample values are unchanged");
    const int savedFocus=focus;
    event(w,IDCMP_INTUITICKS);check(ui.poll(city)==0,"window ticks cannot advance independent simulation");
    event(w,IDCMP_RAWKEY,0x50);check(ui.poll(city)==0x50,"F1 commands forwarded distinctly from ticks");
    event(w,IDCMP_RAWKEY,0x53);check(ui.poll(city)==0x53,"F4 opens messages from graphs");
    event(w,IDCMP_RAWKEY,0x40);check(ui.poll(city)==0x40,"Space pauses or runs while graph has focus");
    check(focus==savedFocus,"redraw and forwarding never steal focus");
    before=blits.size();event(w,IDCMP_RAWKEY,0x40);event(w,IDCMP_MOUSEBUTTONS,SELECTDOWN,20,18);
    event(w,IDCMP_INTUITICKS);check(ui.poll(city,true)==0 && blits.size()==before,"modal drain discards stale controls and simulation ticks");
    event(w,IDCMP_REFRESHWINDOW);ui.poll(city,true);
    check(blits.size()==before+15,"modal drain still redraws exposed native window");
    event(w,IDCMP_RAWKEY,0x45);check(ui.poll(city)==0 && !ui.window(),"Escape closes graph only");
    check(ui.open(&parent,city) && text(ui.window(),"120-year history"),"reopen after display change retains selected range");
    // Shared application menu and remembered placement (2026-09-23).
    Menu strip;ui.close();ui.setMenu(&strip);
    check(ui.open(&parent,city) && ui.window()->MenuStrip==&strip && (ui.window()->IDCMPFlags&IDCMP_MENUPICK),
          "graph window carries the shared application menu");
    event(ui.window(),IDCMP_MENUPICK,0x0021);
    check(ui.poll(city)==(0x10000|0x0021),"menu pick is forwarded as a menu code, not a key");
    event(ui.window(),IDCMP_MENUPICK,MENUNULL);check(ui.poll(city)==0,"MENUNULL is ignored");
    event(ui.window(),IDCMP_MENUPICK,0x0021);check(ui.poll(city,true)==0,"menu pick queued behind a modal is discarded");
    ui.window()->LeftEdge=30;ui.window()->TopEdge=40;ui.close();
    check(menuStrips==0,"menu strip cleared before the window closes");
    check(ui.open(&parent,city) && ui.window()->LeftEdge==30 && ui.window()->TopEdge==40,
          "reopening restores the player's position");
    ui.window()->LeftEdge=900;ui.window()->TopEdge=700;ui.close();
    const int wide=screen.Width;screen.Width=640;screen.Height=480;
    check(ui.open(&parent,city) && ui.window()->LeftEdge+ui.window()->Width<=640 &&
          ui.window()->LeftEdge>=0 && ui.window()->TopEdge>=screen.BarHeight,
          "remembered position is clamped onto a smaller screen");
    screen.Width=wide;
    event(ui.window(),IDCMP_CLOSEWINDOW);event(ui.window(),IDCMP_INTUITICKS);
    check(ui.poll(city,true)==0 && !ui.window(),"native close honored during modal drain and queued messages replied");
    check(opens==closes && fonts==0,"all native resources released");
    printf("RESULT: %d/%d passed (real graph UI, simulated native transport)\n",checks-failures,checks);
    return failures?1:0;
}
'''
with tempfile.TemporaryDirectory(prefix='micropolis-graph-integration-') as tmp:
    out=Path(tmp)
    (out/'native-stub.h').write_text(stub)
    for name in ('intuition/intuition.h','intuition/screens.h','graphics/text.h',
                 'proto/exec.h','proto/intuition.h','proto/graphics.h',
                 'proto/cybergraphics.h','cybergraphx/cybergraphics.h'):
        header=out/name;header.parent.mkdir(exist_ok=True);header.write_text('#include "native-stub.h"\n')
    (out/'micropolis.h').write_text('''#pragma once
#include "graph-model.h"
class Micropolis {public:GraphHistory history[2]{};int cityTime=0,startingYear=1900;
short getHistory(int s,int scale,int i){return history[scale][s][i];}};
''')
    subprocess.run(['python3',str(root/'scripts/build-classic-art.py'),'--family','graphs',str(out/'classic-graph-art.h')],check=True)
    (out/'test.cpp').write_text(fixture)
    subprocess.run([os.environ.get('CXX','clang++'),'-std=c++17','-Wall','-Wextra','-Werror',
                    '-g','-fsanitize=address,undefined','-fno-sanitize-recover=all',
                    '-I',str(out),'-I',str(root/'src'),str(out/'test.cpp'),
                    str(root/'src/graph-window.cpp'),'-o',str(out/'test')],check=True)
    subprocess.run([str(out/'test')],check=True)

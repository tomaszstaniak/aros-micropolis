#!/usr/bin/env python3
"""Exercise the production native popup against a recording Intuition transport."""
from pathlib import Path
import os
import subprocess
import tempfile
root = Path(__file__).resolve().parents[2]
stub = r'''
#pragma once
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <deque>
#include <vector>
using ULONG=unsigned long; using UWORD=unsigned short; using IPTR=uintptr_t; using APTR=void*;
constexpr int TRUE=1,JAM1=1,RECTFMT_BGRA32=1;
enum {TAG_DONE,WA_CustomScreen,WA_Left,WA_Top,WA_Width,WA_Height,WA_Borderless,WA_Activate,WA_RMBTrap,WA_ReportMouse,WA_IDCMP};
enum {IDCMP_MOUSEBUTTONS=1,IDCMP_MOUSEMOVE=2,IDCMP_RAWKEY=4,IDCMP_REFRESHWINDOW=8,IDCMP_INACTIVEWINDOW=16};
enum {SELECTDOWN=1,SELECTUP=2,MENUDOWN=3,MENUUP=4,MIDDLEDOWN=5};
struct Screen {int Width=1024,Height=768,BarHeight=12;};
struct RastPort {};
struct Message {virtual ~Message()=default;};
struct IntuiMessage:Message {ULONG Class=0;UWORD Code=0;int MouseX=0,MouseY=0;};
struct MsgPort {int mp_SigBit=1;};
struct Window {Screen* WScreen=nullptr; RastPort* RPort=nullptr;MsgPort* UserPort=nullptr;int LeftEdge=0,TopEdge=0;};
struct Event {ULONG cls;UWORD code;int x,y;};
inline std::deque<Event> events;
inline std::vector<int> drawsAtEvent;
inline int draws=0,opens=0,closes=0,focus=0,moves=0,waits=0;
inline void ActivateWindow(Window*) {++focus;}
inline void collect(std::vector<IPTR>& tags){tags.push_back(TAG_DONE);}
template<class T,class... Rest>void collect(std::vector<IPTR>& tags,T v,Rest... rest){tags.push_back(static_cast<IPTR>(v));if constexpr(sizeof...(rest))collect(tags,rest...);}
template<class... Args>Window* OpenWindowTags(void*,Args... args){
 std::vector<IPTR> tags;collect(tags,args...);auto* w=new Window;w->RPort=new RastPort;w->UserPort=new MsgPort;++opens;
 for(size_t i=0;i+1<tags.size()&&tags[i]!=TAG_DONE;i+=2){if(tags[i]==WA_CustomScreen)w->WScreen=reinterpret_cast<Screen*>(tags[i+1]);if(tags[i]==WA_Left)w->LeftEdge=tags[i+1];if(tags[i]==WA_Top)w->TopEdge=tags[i+1];}
 return w;
}
inline void Wait(ULONG){assert(++waits<100);}
inline Message* GetMsg(MsgPort*){if(events.empty())return nullptr;auto e=events.front();events.pop_front();drawsAtEvent.push_back(draws);auto* m=new IntuiMessage;m->Class=e.cls;m->Code=e.code;m->MouseX=e.x;m->MouseY=e.y;return m;}
inline void ReplyMsg(Message* m){delete m;}
inline void CloseWindow(Window* w){++closes;delete w->RPort;delete w->UserPort;delete w;}
inline void MoveWindow(Window* w,int x,int y){w->LeftEdge+=x;w->TopEdge+=y;++moves;}
inline void WritePixelArray(APTR,int,int,int,RastPort*,int,int,int,int,int){}
inline void SetAPen(RastPort*,int){} inline void Move(RastPort*,int,int){} inline void Draw(RastPort*,int,int){}
inline int TextLength(RastPort*,const char*,int n){return n*8;}
inline void Text(RastPort*,const char*,int){} inline void SetDrMd(RastPort*,int){}
inline void RectFill(RastPort*,int x,int y,int w,int h){if(x==0&&y==0&&w==299&&h==299)++draws;}
inline void BeginRefresh(Window*){} inline void EndRefresh(Window*,int){}
'''
fixture = r'''
#include "native-stub.h"
#include "classic-tool-ui.h"
#include <cstdio>
int checks=0,failures=0;
void check(bool ok,const char* name){++checks;if(!ok)++failures;std::printf("%s %s\n",ok?"PASS":"FAIL",name);}
Event button(int code,int x=150,int y=150){return {IDCMP_MOUSEBUTTONS,UWORD(code),x,y};}
Event hover(int x,int y){return {IDCMP_MOUSEMOVE,0,x,y};}
const Event esc={IDCMP_RAWKEY,0x45,0,0};
int run(std::initializer_list<Event> input,bool left=false,int x=400,int y=300){
 events=input;events.push_back(esc);draws=waits=moves=0;drawsAtEvent.clear();Screen screen;Window parent;parent.WScreen=&screen;
 int result=showClassicPie(&parent,x,y,left);events.clear();return result;
}
int main(){
 check(run({button(MENUUP),button(SELECTDOWN,246,150),button(SELECTUP,246,150)})==9,"RMB opening click latches, LMB selects road");
 events={button(SELECTDOWN,246,150),button(SELECTUP,246,150)};draws=waits=moves=0;drawsAtEvent.clear();Screen latchedScreen;Window latchedParent;latchedParent.WScreen=&latchedScreen;
 check(showClassicPieLatched(&latchedParent,400,300)==9,"toolbar-opened latched pie selects on first click");events.clear();
 check(run({button(MENUUP),hover(246,150),esc})==-1 && drawsAtEvent.size()==3 && drawsAtEvent[2]>drawsAtEvent[1],"latched hover redraws highlighted sector");
 check(run({button(MENUUP),button(SELECTDOWN,150,54),button(SELECTUP,150,54),hover(67,102),button(SELECTDOWN,67,102),button(SELECTUP,67,102)})==0 && moves==1,"LMB enters Zone and selects residential with hover");
 check(run({button(MENUUP),button(SELECTDOWN,150,246),button(SELECTUP,150,246),button(MENUDOWN,150,246),button(MENUUP,150,246)})==17 && moves==1,"LMB enters Build then RMB selects airport");
 check(run({button(MENUUP),button(MENUDOWN,246,150),button(MENUUP,246,150)})==9,"latched RMB selection remains supported");
 check(run({hover(246,150),button(MENUUP,246,150)})==9,"opening held RMB release selects");
 check(run({hover(246,150),button(SELECTUP,246,150)},true)==9,"opening held Shift-LMB release selects");
 check(run({button(SELECTUP),button(MENUDOWN,246,150),button(MENUUP,246,150)},true)==9,"Shift-LMB latch accepts RMB selection");
 check(run({button(MENUUP,0,-13),button(SELECTDOWN,246,150),button(SELECTUP,246,150)},false,0,0)==9,"clamped unmoved opening click latches before LMB selection");
 check(run({button(SELECTUP,246,150),button(MENUUP),button(SELECTDOWN,246,150),button(SELECTUP,246,150)})==9,"wrong opening release cannot select or consume initial latch");
 check(run({button(MENUUP),button(SELECTUP,246,150),button(MENUDOWN,246,150),button(SELECTUP,246,150),button(MENUUP,246,150)})==9,"unpaired and wrong releases do not select or drop matching press");
 check(run({button(MENUUP),button(SELECTDOWN,246,150),button(MENUDOWN,246,150),button(SELECTUP,246,150)})==-1,"mixed simultaneous buttons cancel safely");
 check(run({button(SELECTDOWN,246,150),button(MENUUP,246,150)})==-1,"opposite down during opening hold cancels");
 check(run({button(MENUUP),button(MIDDLEDOWN)})==-1,"middle button cancels latched popup");
 check(run({button(MENUUP),button(SELECTDOWN),button(SELECTUP)})==-1,"second center click cancels");
 int before=focus;check(run({{IDCMP_INACTIVEWINDOW,0,0,0}})==-1 && focus==before,"focus loss cancels without stealing focus back");
 check(run({button(MENUUP),esc})==-1,"Escape cancels latch");
 check(opens==closes,"native popup resources balanced");
 std::printf("RESULT: %d/%d passed (production popup, recording native transport)\n",checks-failures,checks);return failures?1:0;
}
'''
with tempfile.TemporaryDirectory(prefix='micropolis-pie-integration-') as tmp:
    out=Path(tmp)
    (out/'native-stub.h').write_text(stub)
    for name in ('intuition/intuition.h','intuition/screens.h','cybergraphx/cybergraphics.h','proto/exec.h','proto/intuition.h','proto/graphics.h','proto/cybergraphics.h'):
        header=out/name;header.parent.mkdir(exist_ok=True);header.write_text('#include "native-stub.h"\n')
    subprocess.run(['python3',str(root/'scripts/build-classic-art.py'),str(out/'classic-art.h')],check=True)
    (out/'test.cpp').write_text(fixture)
    subprocess.run([os.environ.get('CXX','clang++'),'-std=c++17','-Wall','-Wextra','-Werror','-g','-fsanitize=address,undefined','-fno-sanitize-recover=all','-I',str(out),'-I',str(root/'src'),str(out/'test.cpp'),str(root/'src/classic-tool-ui.cpp'),'-o',str(out/'test')],check=True)
    subprocess.run([str(out/'test')],check=True)

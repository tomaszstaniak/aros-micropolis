#include "classic-tool-ui.h"
#include "classic-tools.h"
#include "classic-art.h"
#include <algorithm>
#include <cstring>
#include <vector>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>

void drawClassicTool(RastPort *rp,int slot,bool selected,bool enabled,int x,int y) {
    if(slot<0 || slot>=18)return;
    const auto &image=classicArt[slot][selected?1:0];
    const uint32_t *pixels=image.pixels;
    std::vector<uint32_t> disabled;
    if(!enabled) {
        disabled.assign(pixels,pixels+image.width*image.height);
        for(size_t i=0;i<disabled.size();i++) {
            const uint32_t p=disabled[i];
            const unsigned grey=(((p>>16)&255)+((p>>8)&255)+(p&255))/3;
            disabled[i]=0xff000000u|grey*0x010101u;
        }
        pixels=disabled.data();
    }
    WritePixelArray((APTR)pixels,0,0,image.width*4,rp,x+1,y+1,
                    image.width,image.height,RECTFMT_BGRA32);
    SetAPen(rp,selected?2:0);
    Move(rp,x,y+image.height+1);Draw(rp,x,y);Draw(rp,x+image.width+1,y);
    SetAPen(rp,selected?1:0);
    Draw(rp,x+image.width+1,y+image.height+1);Draw(rp,x,y+image.height+1);
    if(!enabled) {
        SetAPen(rp,1);Move(rp,x+2,y+image.height);Draw(rp,x+image.width,y+2);
    }
}

namespace {
constexpr int extent=300,center=extent/2,radius=96;
void textAt(RastPort *rp,const char *text,int x,int baseline) {
    const int len=std::strlen(text);
    Move(rp,x-TextLength(rp,text,len)/2,baseline);Text(rp,text,len);
}
void drawPie(Window *w,const ClassicPie &pie,int active) {
    auto *rp=w->RPort;SetDrMd(rp,JAM1);SetAPen(rp,0);
    RectFill(rp,0,0,extent-1,extent-1);
    SetAPen(rp,1);textAt(rp,pie.title(),center,16);
    for(int i=0;i<pie.size();i++) {
        const double a=pie.angle(i);
        int x=center+std::lround(std::cos(a)*radius);
        int y=center-std::lround(std::sin(a)*radius);
        int slot=pie.slot(i);
        // The source uses highlighted icons in pies; selection relief is native.
        if(slot>=0) {
            const auto &art=classicTools[slot];
            drawClassicTool(rp,slot,true,slot!=10 && slot!=11,x-art.w/2-1,y-art.h/2-1);
            if(i==active) {
                SetAPen(rp,1);int l=x-art.w/2-4,t=y-art.h/2-4;
                Move(rp,l,t);Draw(rp,x+art.w/2+4,t);Draw(rp,x+art.w/2+4,y+art.h/2+4);
                Draw(rp,l,y+art.h/2+4);Draw(rp,l,t);
            }
        } else {
            SetAPen(rp,i==active?1:0);RectFill(rp,x-28,y-12,x+28,y+12);
            SetAPen(rp,i==active?2:1);textAt(rp,slot==-1?"Zone":"Build",x,y+4);
        }
        const double edge=a-ClassicPie::pi/pie.size();
        SetAPen(rp,1);
        Move(rp,center+std::lround(14*std::cos(edge)),center-std::lround(14*std::sin(edge)));
        Draw(rp,center+std::lround(50*std::cos(edge)),center-std::lround(50*std::sin(edge)));
    }
}
}

static int showClassicPieImpl(Window *parent,int screenX,int screenY,UWORD openingButton) {
    Screen *screen=parent->WScreen;
    if(screen->Width<extent || screen->Height<extent+screen->BarHeight+1)return -1;
    auto left=[&](int x){return std::clamp(x-center,0,int(screen->Width)-extent);};
    auto top=[&](int y){return std::clamp(y-center,int(screen->BarHeight)+1,int(screen->Height)-extent);};
    Window *w=OpenWindowTags(nullptr,WA_CustomScreen,(IPTR)screen,
        WA_Left,left(screenX),WA_Top,top(screenY),WA_Width,extent,WA_Height,extent,
        WA_Borderless,TRUE,WA_Activate,TRUE,WA_RMBTrap,TRUE,WA_ReportMouse,TRUE,
        WA_IDCMP,IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE|IDCMP_RAWKEY|
                 IDCMP_REFRESHWINDOW|IDCMP_INACTIVEWINDOW,TAG_DONE);
    if(!w)return -1;
    ClassicPie pie;int active=-1,result=-1;
    bool done=false,moved=false,initial=openingButton!=0,restoreFocus=true;
    // The opening button is already held when this window is posted. Once
    // latched, either normal mouse button may start the next selection.
    UWORD heldButton=openingButton;
    drawPie(w,pie,active);
    while(!done) {
        Wait(1UL<<w->UserPort->mp_SigBit);
        while(auto *msg=(IntuiMessage *)GetMsg(w->UserPort)) {
            ULONG cls=msg->Class;UWORD code=msg->Code;
            const int mx=msg->MouseX,my=msg->MouseY;
            ReplyMsg((Message *)msg);
            if(cls==IDCMP_INACTIVEWINDOW) {restoreFocus=false;done=true;break;}
            if(cls==IDCMP_RAWKEY && code==0x45) {done=true;break;}
            if(cls==IDCMP_REFRESHWINDOW) {
                BeginRefresh(w);drawPie(w,pie,active);EndRefresh(w,TRUE);
            } else if(cls==IDCMP_MOUSEMOVE) {
                if(pie.held) moved=moved || std::abs(w->LeftEdge+mx-screenX)>4 || std::abs(w->TopEdge+my-screenY)>4;
                int next=pie.pick(mx-center,my-center);
                if(next!=active){active=next;drawPie(w,pie,active);}
            } else if(cls==IDCMP_MOUSEBUTTONS) {
                if(code==SELECTDOWN || code==MENUDOWN) {
                    // A second different button during a drag is ambiguous;
                    // cancel instead of treating its release as a selection.
                    if(heldButton) {
                        if(code!=heldButton){done=true;break;}
                        continue;
                    }
                    heldButton=code;pie.press();
                    active=pie.pick(mx-center,my-center);drawPie(w,pie,active);
                } else if(code==SELECTUP || code==MENUUP) {
                    const UWORD releasedButton=code==SELECTUP?SELECTDOWN:MENUDOWN;
                    if(heldButton!=releasedButton)continue;
                    heldButton=0;
                    // At screen edges the popup is clamped. An unmoved opening
                    // click still latches, never selects an unrelated sector.
                    moved=moved || std::abs(w->LeftEdge+mx-screenX)>4 || std::abs(w->TopEdge+my-screenY)>4;
                    int selection=pie.release(initial && !moved?0:mx-center,initial && !moved?0:my-center);
                    initial=false;
                    if(selection>=0){result=selection;done=true;break;}
                    if(selection==ClassicPie::cancel){done=true;break;}
                    if(selection==ClassicPie::submenu) {
                        screenX=w->LeftEdge+mx;screenY=w->TopEdge+my;
                        MoveWindow(w,left(screenX)-w->LeftEdge,top(screenY)-w->TopEdge);
                    }
                    active=-1;drawPie(w,pie,active);
                } else if(code==MIDDLEDOWN) {
                    done=true;break;
                }
            }
        }
    }
    CloseWindow(w);
    if(restoreFocus)ActivateWindow(parent);
    return result;
}

int showClassicPie(Window *parent,int screenX,int screenY,bool leftButton) {
    return showClassicPieImpl(parent,screenX,screenY,
                              leftButton?SELECTDOWN:MENUDOWN);
}

int showClassicPieLatched(Window *parent,int screenX,int screenY) {
    return showClassicPieImpl(parent,screenX,screenY,0);
}

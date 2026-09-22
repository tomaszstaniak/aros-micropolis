#include "game-menu.h"
#include "graph-window.h"
#include "micropolis.h"
#include "classic-graph-art.h"
#include <cstdio>
#include <cstring>
#include <vector>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/text.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>

namespace {
constexpr int width=590,height=300;
constexpr int plotX=108,plotY=32,plotW=466,plotH=180;
constexpr int iconX[]={12,12,12,49,49,49,12,12};
constexpr int iconY[]={84,119,154,84,119,154,12,44};
constexpr uint32_t paper=0xffbfbfbf,grid=0xff808080;
bool gameKey(unsigned code) {
    return (code>=0x50 && code<=0x55) || code==0x58 || code==0x59 ||
        code==0x40 || code==0x36;
}
}

GraphWindow::~GraphWindow(){close();}

bool GraphWindow::open(Window *parent,Micropolis &city) {
    if(win_){refresh(city);return true;}
    if(!parent || !parent->WScreen)return false;
    auto *screen=parent->WScreen;
    const int outerW=width+screen->WBorLeft+screen->WBorRight;
    const int outerH=height+screen->WBorTop+screen->WBorBottom+screen->Font->ta_YSize+1;
    const int top=screen->BarHeight+4;
    if(outerW>screen->Width || outerH>screen->Height-top)return false;
    TextAttr attr={(STRPTR)"topaz.font",8,FS_NORMAL,FPF_ROMFONT};
    font_=OpenFont(&attr);
    if(!font_)return false;
    int left=(screen->Width-outerW)/2,topEdge=std::max(top,(screen->Height-outerH)/2);
    placement.place(screen->Width,screen->Height,top,outerW,outerH,left,topEdge);
    win_=OpenWindowTags(NULL,WA_CustomScreen,(IPTR)screen,
        WA_Title,(IPTR)"Micropolis - Graphs",WA_InnerWidth,width,WA_InnerHeight,height,
        WA_Left,left,WA_Top,topEdge,
        WA_Activate,TRUE,WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,
        WA_SimpleRefresh,TRUE,WA_RMBTrap,menu_?FALSE:TRUE,
        WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_RAWKEY|IDCMP_REFRESHWINDOW|IDCMP_MOUSEBUTTONS,
        TAG_DONE);
    if(!win_){close();return false;}
    SetFont(win_->RPort,font_);
    shareMenu(win_,menu_);
    if(auto *info=GetScreenDrawInfo(screen)) {
        background_=info->dri_Pens[BACKGROUNDPEN];foreground_=info->dri_Pens[TEXTPEN];
        FreeScreenDrawInfo(screen,info);
    }
    model_.fresh=false;
    refresh(city);
    return true;
}

void GraphWindow::close() {
    if(win_){
        while(auto *m=GetMsg(win_->UserPort))ReplyMsg(m);
        placement.remember(win_->LeftEdge,win_->TopEdge,win_->Width,win_->Height);
        unshareMenu(win_);CloseWindow(win_);win_=nullptr;
    }
    if(font_){CloseFont(font_);font_=nullptr;}
    model_.fresh=false;
}

void GraphWindow::text(int x,int y,const char *value) {
    Move(win_->RPort,win_->BorderLeft+x,win_->BorderTop+y);
    Text(win_->RPort,value,strlen(value));
}

void GraphWindow::draw() {
    auto *rp=win_->RPort;
    SetDrMd(rp,JAM1);
    // Compose the plot privately, then publish it once. Never erase the live
    // plot to a background color between history updates.
    std::vector<uint32_t> pixels(plotW*plotH,paper);
    auto point=[&](int x,int y,uint32_t color){
        if(x>=0 && x<plotW && y>=0 && y<plotH)pixels[y*plotW+x]=color;
    };
    const auto ticks=graphTicks(model_.scale,cityTime_,plotW);
    for(const auto &tick:ticks)for(int y=0;y<plotH;++y)point(tick.x,y,grid);
    for(int i=0;i<=4;++i)for(int x=0;x<plotW;++x)point(x,i*(plotH-1)/4,grid);
    for(int s=0;s<6;++s)if(model_.mask&(1u<<s)) {
        for(int i=1;i<120;++i) {
            int x0=(i-1)*(plotW-1)/119,y0=graphY(model_.value(s,i-1),plotH);
            const int x1=i*(plotW-1)/119,y1=graphY(model_.value(s,i),plotH);
            const int dx=x1-x0,dy=-std::abs(y1-y0),sy=y0<y1?1:-1;
            int error=dx+dy;
            for(;;) {
                point(x0,y0,graphColors[s]);point(x0,y0-1,graphColors[s]);point(x0,y0+1,graphColors[s]);
                if(x0==x1 && y0==y1)break;
                const int e2=2*error;
                if(e2>=dy){error+=dy;++x0;}
                if(e2<=dx){error+=dx;y0+=sy;}
            }
        }
    }
    WritePixelArray(pixels.data(),0,0,plotW*4,rp,win_->BorderLeft+plotX,
                    win_->BorderTop+plotY,plotW,plotH,RECTFMT_BGRA32);
    // Only margins/controls may be cleared; the chart rectangle is excluded.
    SetAPen(rp,background_);
    const int l=win_->BorderLeft,t=win_->BorderTop;
    RectFill(rp,l,t,l+plotX-1,t+height-1);
    RectFill(rp,l+plotX,t,l+width-1,t+plotY-1);
    RectFill(rp,l+plotX,t+plotY+plotH,l+width-1,t+height-1);
    RectFill(rp,l+plotX+plotW,t+plotY,l+width-1,t+plotY+plotH-1);
    for(int i=0;i<8;++i) {
        const auto &art=classicGraphArt[i][i<6?!!(model_.mask&(1u<<i)):model_.scale==i-6];
        WritePixelArray((APTR)art.pixels,0,0,art.width*4,rp,l+iconX[i],t+iconY[i],
                        art.width,art.height,RECTFMT_BGRA32);
    }
    SetAPen(rp,foreground_);
    char label[96];
    snprintf(label,sizeof label,"%s history    %d/%d",model_.scale?"120-year":"10-year",month_+1,year_);
    text(plotX,18,label);
    // Year (or decade) labels at the classic ticks; skip one that would
    // overlap its right-hand neighbour or leave the plot.
    int nextLeft=plotX+plotW;
    for(const auto &tick:ticks) {
        snprintf(label,sizeof label,"%d",tick.year);
        const int w=TextLength(rp,label,strlen(label));
        const int x=std::min(plotX+tick.x+2,plotX+plotW-w);
        if(x+w+4>nextLeft || x<plotX)continue;
        text(x,224,label);nextLeft=x;
    }
    for(int s=0;s<6;++s) {
        const int x=12+(s%3)*190,y=240+(s/3)*16;
        uint32_t swatch[64];std::fill(swatch,swatch+64,(model_.mask&(1u<<s))?graphColors[s]:paper);
        WritePixelArray(swatch,0,0,32,rp,l+x,t+y-7,8,8,RECTFMT_BGRA32);
        text(x+12,y,graphNames[s]);
    }
    text(12,286,"Cash flow: index 128 = zero.  Esc: dismiss");
}

void GraphWindow::refresh(Micropolis &city) {
    if(!win_)return;
    const int year=city.startingYear+city.cityTime/48;
    const int month=(city.cityTime%48)/4;
    cityTime_=city.cityTime;
    const bool changed=model_.update(city);
    if(changed || year!=year_ || month!=month_){year_=year;month_=month;draw();}
}

int GraphWindow::poll(Micropolis &city,bool discardInput) {
    if(!win_)return 0;
    refresh(city);

    while(auto *msg=(IntuiMessage *)GetMsg(win_->UserPort)) {
        const ULONG cls=msg->Class;const UWORD code=msg->Code;
        const int x=msg->MouseX-win_->BorderLeft,y=msg->MouseY-win_->BorderTop;
        ReplyMsg((Message *)msg);
        if(cls==IDCMP_CLOSEWINDOW || (!discardInput && cls==IDCMP_RAWKEY && code==0x45)){close();return 0;}
        if(discardInput && cls!=IDCMP_REFRESHWINDOW)continue;
        if(cls==IDCMP_REFRESHWINDOW){BeginRefresh(win_);draw();EndRefresh(win_,TRUE);}
        else if(cls==IDCMP_INTUITICKS)continue;
        else if(cls==IDCMP_MENUPICK){if(code!=MENUNULL)return menuPickResult(code);}
        else if(cls==IDCMP_RAWKEY && gameKey(code))return code;
        else if(cls==IDCMP_MOUSEBUTTONS && code==SELECTDOWN) {
            for(int i=0;i<8;++i) {
                const auto &art=classicGraphArt[i][0];
                if(x<iconX[i] || x>=iconX[i]+art.width || y<iconY[i] || y>=iconY[i]+art.height)continue;
                if(i<6)model_.toggle(i);else model_.setScale(i-6);
                model_.update(city);draw();break;
            }
        }
    }
    return 0;
}

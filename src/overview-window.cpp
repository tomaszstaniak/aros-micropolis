#include "game-menu.h"
#include "overview-window.h"
#include "small-map.h"
#include "demand-model.h"
#include "micropolis.h"
#include "classic-overview-art.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/text.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <cybergraphx/cybergraphics.h>

namespace {
constexpr int mapW=360,mapH=300,panelX=372,width=590,height=380;
constexpr int layerTop=12,layerStep=20,demandX=466,demandY=306;
bool gameKey(unsigned code){return (code>=0x50&&code<=0x56)||code==0x58||code==0x59||code==0x40||code==0x36;}
int sample(Micropolis &city,OverviewLayer layer,int x,int y) {
    const int b=overviewLayerBlockSize(layer),sx=x/b,sy=y/b;
    switch(layer) {
        case OverviewLayer::PopulationDensity:return city.getPopulationDensity(sx,sy);
        case OverviewLayer::RateOfGrowth:return city.getRateOfGrowth(sx,sy);
        case OverviewLayer::LandValue:return city.getLandValue(sx,sy);
        case OverviewLayer::CrimeRate:return city.getCrimeRate(sx,sy);
        case OverviewLayer::PollutionDensity:return city.getPollutionDensity(sx,sy);
        case OverviewLayer::TrafficDensity:return city.getTrafficDensity(sx,sy);
        case OverviewLayer::FireCoverage:return city.getFireCoverage(sx,sy);
        case OverviewLayer::PoliceCoverage:return city.getPoliceCoverage(sx,sy);
        default:return 0;
    }
}
}

OverviewWindow::~OverviewWindow(){close();}
bool OverviewWindow::open(Window *parent,Micropolis &city,const DemandModel &demand,
                          int cx,int cy,int columns,int rows) {
    if(win_){refresh(city,demand,cx,cy,columns,rows);return true;}
    if(!parent||!parent->WScreen)return false;
    auto *screen=parent->WScreen;
    const int outerW=width+screen->WBorLeft+screen->WBorRight;
    const int outerH=height+screen->WBorTop+screen->WBorBottom+screen->Font->ta_YSize+1;
    if(outerW>screen->Width||outerH>screen->Height-screen->BarHeight)return false;
    TextAttr attr={(STRPTR)"topaz.font",8,FS_NORMAL,FPF_ROMFONT};font_=OpenFont(&attr);
    if(!font_)return false;
    int left=(screen->Width-outerW)/2,top=std::max((int)screen->BarHeight+2,(screen->Height-outerH)/2);
    placement.place(screen->Width,screen->Height,screen->BarHeight+2,outerW,outerH,left,top);
    win_=OpenWindowTags(nullptr,WA_CustomScreen,(IPTR)screen,WA_Title,(IPTR)"Micropolis - Overview",
        WA_InnerWidth,width,WA_InnerHeight,height,WA_Left,left,WA_Top,top,
        WA_Activate,TRUE,WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,
        WA_SimpleRefresh,TRUE,WA_RMBTrap,FALSE,WA_ReportMouse,TRUE,
        WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_RAWKEY|IDCMP_REFRESHWINDOW|IDCMP_MOUSEBUTTONS|IDCMP_MOUSEMOVE,TAG_DONE);
    if(!win_){close();return false;}SetFont(win_->RPort,font_);
    if(!menu_.attach(win_)){close();return false;}
    if(auto *i=GetScreenDrawInfo(screen)){background_=i->dri_Pens[BACKGROUNDPEN];foreground_=i->dri_Pens[TEXTPEN];shine_=i->dri_Pens[SHINEPEN];shadow_=i->dri_Pens[SHADOWPEN];FreeScreenDrawInfo(screen,i);}
    draw(city,demand,cx,cy,columns,rows);return true;
}
void OverviewWindow::close(){panning_=false;if(win_){while(auto *m=GetMsg(win_->UserPort))ReplyMsg(m);placement.remember(win_->LeftEdge,win_->TopEdge,win_->Width,win_->Height);menu_.detach();CloseWindow(win_);win_=nullptr;}if(font_){CloseFont(font_);font_=nullptr;}}
void OverviewWindow::text(int x,int y,const char *s){Move(win_->RPort,win_->BorderLeft+x,win_->BorderTop+y);Text(win_->RPort,s,strlen(s));}
void OverviewWindow::draw(Micropolis &city,const DemandModel &demand,int cx,int cy,int columns,int rows) {
    if(!win_)return;tiles_.resize(overviewWorldW*overviewWorldH);samples_.resize(tiles_.size());
    for(int y=0;y<overviewWorldH;++y)for(int x=0;x<overviewWorldW;++x){size_t i=(size_t)y*overviewWorldW+x;tiles_[i]=city.map[x][y];samples_[i]=sample(city,layer_,x,y);}
    renderSmallMap(classic_tilessm.pixels,classic_tilessm.height/3,tiles_.data(),samples_.data(),overviewWorldW,overviewWorldH,layer_,pixels_);
    if(chalk_)drawChalkOverview(*chalk_,pixels_.data(),mapW,mapH);
    auto *rp=win_->RPort;const int l=win_->BorderLeft,t=win_->BorderTop;
    SetDrMd(rp,JAM1);SetAPen(rp,background_);RectFill(rp,l,t,l+width-1,t+height-1);
    WritePixelArray(pixels_.data(),0,0,mapW*4,rp,l,t,mapW,mapH,RECTFMT_BGRA32);
    int rx,ry,rw,rh;OverviewCamera::rect(cx,cy,columns,rows,3,rx,ry,rw,rh);
    SetAPen(rp,shine_);Move(rp,l+rx,t+ry);Draw(rp,l+rx+rw-1,t+ry);Draw(rp,l+rx+rw-1,t+ry+rh-1);Draw(rp,l+rx,t+ry+rh-1);Draw(rp,l+rx,t+ry);
    SetAPen(rp,foreground_);text(panelX,10,"Zones / overlays");
    for(int i=0;i<(int)OverviewLayer::Count;++i){char line[40];snprintf(line,sizeof line,"%c %s",i==(int)layer_?'*':' ',overviewLayerName((OverviewLayer)i));text(panelX,layerTop+16+i*layerStep,line);}
    text(8,322,"Click/drag map: pan main view   Esc: close");
    text(8,340,"Overlay: low grey, mid yellow, high orange, top red");
    text(8,356,"Growth: green +, orange/yellow -   Power: red/blue/grey");
    // Original demand background plus native signed R/C/I bars.
    WritePixelArray((APTR)classic_demandg.pixels,0,0,classic_demandg.width*4,rp,l+demandX,t+demandY,classic_demandg.width,classic_demandg.height,RECTFMT_BGRA32);
    const float values[]={demand.residential,demand.commercial,demand.industrial};
    const uint32_t colors[]={0xff00e600,0xff0000e6,0xffffff00};
    for(int i=0;i<3;++i){int h=demandBarHeight(values[i],17),x=demandX+9+i*10,y=demandY+23;uint32_t bar[6*17];std::fill(bar,bar+6*17,colors[i]);if(h>0)WritePixelArray(bar,0,0,6*4,rp,l+x,t+y-h,6,h,RECTFMT_BGRA32);else if(h<0)WritePixelArray(bar,0,0,6*4,rp,l+x,t+y,6,-h,RECTFMT_BGRA32);}
    text(demandX-1,demandY+58,"R/C/I -> Eval");
}
void OverviewWindow::refresh(Micropolis &city,const DemandModel &demand,int cx,int cy,int columns,int rows){if(win_)draw(city,demand,cx,cy,columns,rows);}
int OverviewWindow::poll(Micropolis &city,const DemandModel &demand,int &cx,int &cy,int columns,int rows,bool discard) {
    if(!win_)return 0;
    while(auto *msg=(IntuiMessage*)GetMsg(win_->UserPort)){
        ULONG cls=msg->Class;UWORD code=msg->Code;int x=msg->MouseX-win_->BorderLeft,y=msg->MouseY-win_->BorderTop;ReplyMsg((Message*)msg);
        if(cls==IDCMP_CLOSEWINDOW||(!discard&&cls==IDCMP_RAWKEY&&code==0x45)){close();return 0;}
        if(discard&&cls!=IDCMP_REFRESHWINDOW)continue;
        if(cls==IDCMP_REFRESHWINDOW){BeginRefresh(win_);draw(city,demand,cx,cy,columns,rows);EndRefresh(win_,TRUE);}
        else if(cls==IDCMP_MENUPICK){if(code!=MENUNULL)return menuPickResult(code);}
        else if(cls==IDCMP_RAWKEY&&gameKey(code))return code;
        else if(cls==IDCMP_MOUSEBUTTONS&&code==SELECTDOWN){
            if(x<mapW&&y<mapH){OverviewCamera::panTo(x,y,3,columns,rows,cx,cy);panning_=true;draw(city,demand,cx,cy,columns,rows);}
            else if(x>=panelX&&y>=layerTop+8&&y<layerTop+8+(int)OverviewLayer::Count*layerStep){int i=(y-(layerTop+8))/layerStep;layer_=(OverviewLayer)i;draw(city,demand,cx,cy,columns,rows);}
            else if(x>=demandX&&y>=demandY&&y<height)return 2;
        } else if(cls==IDCMP_MOUSEBUTTONS&&code==SELECTUP)panning_=false;
        else if(cls==IDCMP_MOUSEMOVE&&panning_&&x>=0&&x<mapW&&y>=0&&y<mapH){OverviewCamera::panTo(x,y,3,columns,rows,cx,cy);draw(city,demand,cx,cy,columns,rows);}
    }
    return 0;
}

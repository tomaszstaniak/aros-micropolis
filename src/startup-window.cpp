#include "startup-window.h"
#include <cstdio>
#include <cstring>
#include <ctime>
#include <map>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>
#include <proto/cybergraphics.h>

namespace {
struct StartupWindow {
    Library *GadToolsBase=nullptr;
    Window *win=nullptr;
    APTR visual=nullptr;
    Gadget *head=nullptr,*name=nullptr;
    ~StartupWindow(){
        if(win)CloseWindow(win);
        if(head)FreeGadgets(head);
        if(visual)FreeVisualInfo(visual);
        if(GadToolsBase)CloseLibrary(GadToolsBase);
    }
    bool open(Window *parent,StartupLayout layout){
        GadToolsBase=OpenLibrary((CONST_STRPTR)"gadtools.library",37);
        if(!GadToolsBase)return false;
        TagItem end[]={{TAG_DONE,0}};
        visual=GetVisualInfoA(parent->WScreen,end);
        if(!visual)return false;
        win=OpenWindowTags(nullptr,WA_CustomScreen,(IPTR)parent->WScreen,
            WA_Title,(IPTR)"Micropolis - Choose a city",WA_InnerWidth,layout.width,
            WA_InnerHeight,layout.height+24,WA_Left,0,WA_Top,parent->WScreen->BarHeight+1,
            WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,WA_Activate,TRUE,
            WA_ReportMouse,TRUE,WA_RMBTrap,TRUE,
            WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_RAWKEY|IDCMP_REFRESHWINDOW|
                     IDCMP_GADGETUP|IDCMP_MOUSEMOVE|IDCMP_MOUSEBUTTONS,TAG_DONE);
        if(!win)return false;
        auto *context=CreateContext(&head);
        if(!context)return false;
        auto r=layout.rect({534,5,360,28});
        NewGadget ng={};ng.ng_LeftEdge=win->BorderLeft+r.x;ng.ng_TopEdge=win->BorderTop+r.y;
        ng.ng_Width=r.w;ng.ng_Height=std::max(16,r.h);ng.ng_TextAttr=win->WScreen->Font;
        ng.ng_GadgetID=1;ng.ng_VisualInfo=visual;
        TagItem tags[]={{GTST_MaxChars,63},{GTST_String,(IPTR)"NowHere"},{TAG_DONE,0}};
        name=CreateGadgetA(STRING_KIND,context,&ng,tags);
        if(!name)return false;
        AddGList(win,head,(UWORD)-1,-1,nullptr);
        ActivateWindow(win);return true;
    }
    void readName(Micropolis *m){
        auto *info=(StringInfo *)name->SpecialInfo;
        if(m && info && info->Buffer[0])m->setCityName(reinterpret_cast<const char *>(info->Buffer));
    }
    void setName(Micropolis *m){
        TagItem tags[]={{GTST_String,(IPTR)m->cityName.c_str()},{TAG_DONE,0}};
        GT_SetGadgetAttrsA(name,win,nullptr,tags);
    }
};
const char *baseNames[]={"button1hilite","button2hilite","button3hilite","button4hilite",
 "checkbox1hilite","checkbox2hilite","checkbox3hilite","lefthilite","righthilite","playhilite",
 "scenario1hilite","scenario2hilite","scenario3hilite","scenario4hilite",
 "scenario5hilite","scenario6hilite","scenario7hilite","scenario8hilite"};
}

std::unique_ptr<Micropolis> showStartup(Window *parent,const BmpImage &tiles,
        StartupModel::Factory factory,StartupFilePicker picker,std::string &error,
        bool returnToCity){
    StartupModel model(factory);
    unsigned seed=(unsigned)time(nullptr);
    if(!model.generate((int)seed,0)){error="Cannot create city preview";return nullptr;}
    std::map<std::string,BmpImage> art;
    auto load=[&](const std::string &name){return loadBmp8("startup/"+name+".bmp",art[name],error);};
    if(!load("background-micropolis"))return nullptr;
    for(auto name:baseNames)if(!load(name))return nullptr;
    for(int i=1;i<=3;i++)for(auto suffix:{"checked","hilitechecked"})
        if(!load("checkbox"+std::to_string(i)+suffix))return nullptr;
    if(!load("leftdisabled") || !load("rightdisabled"))return nullptr;
    auto layout=StartupLayout::fit(parent->WScreen->Width,parent->WScreen->Height);
    StartupWindow d;
    if(layout.width<400 || !d.open(parent,layout)){error="Cannot open startup window on this screen";return nullptr;}
    std::vector<uint32_t> canvas(layout.width*layout.height);
    auto blit=[&](const BmpImage &image,StartupRect r){
        for(int y=0;y<r.h;y++)for(int x=0;x<r.w;x++)
            canvas[(r.y+y)*layout.width+r.x+x]=image.pixels[(y*image.height/r.h)*image.width+x*image.width/r.w];
    };
    int hover=-1,pressed=-1;
    auto draw=[&]{
        auto *m=model.city();
        blit(art["background-micropolis"],{0,0,layout.width,layout.height});
        // Mini-map samples actual classic tile art; selection never advances simulation.
        auto r=layout.rect({534,48,360,300});
        for(int y=0;y<r.h;y++)for(int x=0;x<r.w;x++){
            int mx=x*WORLD_W/r.w,my=y*WORLD_H/r.h;
            int tile=m->map[mx][my]&LOMASK;
            int sx=(tile%32)*16+8,sy=(tile/32)*16+8;
            canvas[(r.y+y)*layout.width+r.x+x]=sy<tiles.height?tiles.pixels[sy*tiles.width+sx]:0xff000000;
        }
        if(hover>=0)blit(art[baseNames[hover]],layout.button(hover));
        const int level=std::clamp((int)m->gameLevel,0,2);
        blit(art["checkbox"+std::to_string(level+1)+(hover==4+level?"hilitechecked":"checked")],layout.button(4+level));
        if(!model.canPrevious())blit(art["leftdisabled"],layout.button(7));
        if(!model.canNext())blit(art["rightdisabled"],layout.button(8));
        if(m->scenario!=SC_NONE)for(int i=10;i<18;i++)if(startupScenarioId(i)==m->scenario)
            blit(art[baseNames[i]],layout.button(i));
        WritePixelArray(canvas.data(),0,0,layout.width*4,d.win->RPort,
            d.win->BorderLeft,d.win->BorderTop,layout.width,layout.height,RECTFMT_BGRA32);
        SetAPen(d.win->RPort,0);
        RectFill(d.win->RPort,d.win->BorderLeft,d.win->BorderTop+layout.height,
            d.win->BorderLeft+layout.width-1,d.win->BorderTop+layout.height+23);
        // During play, Quit/Esc/close return to the running city unchanged.
        char footer[256];snprintf(footer,sizeof footer,"%s - %d - $%ld | Choose a city, then Play. %s",
            m->cityName.c_str(),(int)(1900+m->cityTime/48),(long)m->totalFunds,
            returnToCity?"Quit/ESC: back to your city":"ESC: quit");
        size_t length=strlen(footer);
        while(length && TextLength(d.win->RPort,footer,length)>layout.width-12)--length;
        SetAPen(d.win->RPort,1);SetDrMd(d.win->RPort,JAM1);
        Move(d.win->RPort,d.win->BorderLeft+6,d.win->BorderTop+layout.height+16);
        Text(d.win->RPort,footer,length);
        RefreshGList(d.head,d.win,nullptr,-1);GT_RefreshWindow(d.win,nullptr);
    };
    d.setName(model.city());draw();
    bool done=false,play=false;
    while(!done){
        Wait((1UL<<d.win->UserPort->mp_SigBit)|(1UL<<parent->UserPort->mp_SigBit));
        // The inactive game window exists only to own the target screen.
        while(auto *msg=(IntuiMessage *)GetMsg(parent->UserPort)){
            auto cls=msg->Class;ReplyMsg((Message *)msg);
            if(cls==IDCMP_REFRESHWINDOW){BeginRefresh(parent);EndRefresh(parent,TRUE);}
        }
        while(auto *msg=GT_GetIMsg(d.win->UserPort)){
            ULONG cls=msg->Class;UWORD code=msg->Code;
            int hit=layout.hit(msg->MouseX-d.win->BorderLeft,msg->MouseY-d.win->BorderTop);
            GT_ReplyIMsg(msg);int action=-1;
            if(cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_RAWKEY && code==0x45)){done=true;break;}
            if(cls==IDCMP_REFRESHWINDOW){GT_BeginRefresh(d.win);draw();GT_EndRefresh(d.win,TRUE);}
            else if(cls==IDCMP_GADGETUP){d.readName(model.city());d.setName(model.city());draw();}
            else if(cls==IDCMP_MOUSEMOVE && hit!=hover){hover=hit;draw();}
            else if(cls==IDCMP_MOUSEBUTTONS){
                if(code==SELECTDOWN)pressed=hit;
                else if(code==SELECTUP){if(hit==pressed)action=hit;pressed=-1;}
            }
            if(action<0)continue;
            d.readName(model.city());bool success=true;
            if(action==2){done=true;break;}
            if(action==9){play=true;done=true;break;}
            if(action==0){char path[512];if(picker(d.win,false,path,sizeof path))success=model.load(path);}
            else if(action==1)success=model.generate((int)++seed,(int)model.city()->gameLevel);
            else if(action==3)success=model.load("cities/about.cty");
            else if(action>=4 && action<=6)model.level(action-4);
            else if(action==7)model.previous();
            else if(action==8)model.next();
            else if(action>=10)success=model.scenario(startupScenarioId(action));
            if(!success){
                EasyStruct e={sizeof e,0,(STRPTR)"Micropolis",(STRPTR)"Cannot load this city.\nThe previous selection is unchanged.",(STRPTR)"OK"};
                EasyRequestArgs(d.win,&e,nullptr,nullptr);
            }
            d.setName(model.city());hover=-1;pressed=-1;draw();
        }
    }
    return play?model.take():nullptr;
}

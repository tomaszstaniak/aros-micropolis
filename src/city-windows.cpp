#include "city-windows.h"
#include "budget-model.h"
#include <cstdio>
#include <cstring>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <libraries/gadtools.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/gadtools.h>

namespace {
// GadTools owns the gadget chain; the window must close before it is freed.
struct Dialog {
    Library *GadToolsBase=nullptr;
    Window *win=nullptr;
    APTR visual=nullptr;
    Gadget *head=nullptr,*last=nullptr;
    bool valid=true;
    ~Dialog() {
        if(win) CloseWindow(win);
        if(head) FreeGadgets(head);
        if(visual) FreeVisualInfo(visual);
        if(GadToolsBase) CloseLibrary(GadToolsBase);
    }
    bool open(Window *parent,const char *title,int width,int height) {
        GadToolsBase=OpenLibrary((CONST_STRPTR)"gadtools.library",37);
        if(!GadToolsBase)return false;
        TagItem end[]={{TAG_DONE,0}};
        visual=GetVisualInfoA(parent->WScreen,end);
        if(!visual)return false;
        win=OpenWindowTags(NULL,WA_CustomScreen,(IPTR)parent->WScreen,
            WA_Title,(IPTR)title,WA_InnerWidth,width,WA_InnerHeight,height,
            WA_Left,std::max(0,(parent->WScreen->Width-width-20)/2),
            WA_Top,std::max((int)parent->WScreen->BarHeight+4,(parent->WScreen->Height-height-30)/2),
            WA_Activate,TRUE,WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,
            WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_RAWKEY|IDCMP_REFRESHWINDOW|
                     IDCMP_GADGETUP|IDCMP_GADGETDOWN|IDCMP_MOUSEMOVE,
            TAG_DONE);
        if(!win)return false;
        last=CreateContext(&head);
        return last!=nullptr;
    }
    Gadget *add(int kind,int id,int x,int y,int width,int height,const char *label,TagItem *tags) {
        if(!valid)return nullptr;
        NewGadget ng={};
        ng.ng_LeftEdge=win->BorderLeft+x;ng.ng_TopEdge=win->BorderTop+y;
        ng.ng_Width=width;ng.ng_Height=height;ng.ng_GadgetText=(STRPTR)label;
        ng.ng_TextAttr=win->WScreen->Font;ng.ng_GadgetID=id;
        ng.ng_Flags=PLACETEXT_RIGHT;ng.ng_VisualInfo=visual;
        last=CreateGadgetA(kind,last,&ng,tags);
        valid=last!=nullptr;return last;
    }
    void attach() {
        AddGList(win,head,(UWORD)-1,-1,NULL);
        RefreshGList(head,win,NULL,-1);GT_RefreshWindow(win,NULL);
    }
    void text(int x,int y,const char *s) {
        SetAPen(win->RPort,1);SetDrMd(win->RPort,JAM1);
        Move(win->RPort,win->BorderLeft+x,win->BorderTop+y);
        Text(win->RPort,s,strlen(s));
    }
    void clear(int x,int y,int w,int h) {
        SetAPen(win->RPort,0);
        RectFill(win->RPort,win->BorderLeft+x,win->BorderTop+y,
                 win->BorderLeft+x+w-1,win->BorderTop+y+h-1);
    }
    void level(Gadget *g,int tag,int value) {
        TagItem tags[]={{(ULONG)tag,(IPTR)value},{TAG_DONE,0}};
        GT_SetGadgetAttrsA(g,win,NULL,tags);
    }
    IntuiMessage *message() {return GT_GetIMsg(win->UserPort);}
    void reply(IntuiMessage *m) {GT_ReplyIMsg(m);}
    void refresh() {GT_RefreshWindow(win,NULL);}
    void beginRefresh() {GT_BeginRefresh(win);}
    void endRefresh() {GT_EndRefresh(win,TRUE);}
};
void unavailable(Window *parent) {
    EasyStruct e={sizeof e,0,(STRPTR)"Micropolis",(STRPTR)"Cannot open city window.",(STRPTR)"OK"};
    EasyRequestArgs(parent,&e,NULL,NULL);
}
}

bool showCityBudget(Window *parent,Micropolis &m) {
    Dialog d;
    if(!d.open(parent,"Micropolis - Budget (simulation paused)",590,340)) {unavailable(parent);return false;}
    const BudgetDraft original=BudgetDraft::read(m);
    BudgetDraft draft=original;
    Gadget *sliders[4]={};
    const char *names[]={"Road fund","Fire fund","Police fund","Tax rate"};
    for(int i=0;i<4;i++) {
        const int v=i==3?draft.tax:(int)((i==0?draft.road:i==1?draft.fire:draft.police)*100+0.5f);
        TagItem tags[]={{GTSL_Min,0},{GTSL_Max,(IPTR)(i==3?20:100)},
            {GTSL_Level,(IPTR)v},{PGA_Freedom,LORIENT_HORIZ},{GA_RelVerify,TRUE},
            {GA_Immediate,TRUE},{TAG_DONE,0}};
        sliders[i]=d.add(SLIDER_KIND,10+i,270,34+i*58,300,14,nullptr,tags);
    }
    TagItem check[]={{GTCB_Checked,(IPTR)draft.automatic},{TAG_DONE,0}};
    Gadget *automatic=d.add(CHECKBOX_KIND,20,12,250,18,14,"Auto budget",check);
    TagItem end[]={{TAG_DONE,0}};
    d.add(BUTTON_KIND,1,12,286,200,24,"Continue",end);
    d.add(BUTTON_KIND,2,222,286,140,24,"Reset",end);
    d.add(BUTTON_KIND,3,372,286,200,24,"Cancel",end);
    if(!d.valid) {unavailable(parent);return false;}
    d.attach();
    auto draw=[&] {
        char text[128];
        d.clear(8,6,245,235);
        d.text(12,20,"Treasury (current)");
        snprintf(text,sizeof text,"$%ld",(long)m.totalFunds);d.text(12,38,text);
        d.text(12,68,"Taxes (last assessment)");
        snprintf(text,sizeof text,"$%ld",(long)m.taxFund);d.text(12,86,text);
        long expense=(long)(m.roadFund*draft.road)+(long)(m.fireFund*draft.fire)+(long)(m.policeFund*draft.police);
        d.text(12,116,"Selected service costs");
        snprintf(text,sizeof text,"$%ld",expense);d.text(12,134,text);
        d.text(12,164,"Balance at those figures");
        snprintf(text,sizeof text,"$%+ld",(long)m.taxFund-expense);d.text(12,182,text);
        d.text(12,212,"Tax changes: next assessment");
        d.text(12,228,"Engine limits unaffordable funds");
        for(int i=0;i<4;i++) {
            d.clear(266,4+i*58,312,26);
            if(i==3)snprintf(text,sizeof text,"%s: %d%%",names[i],draft.tax);
            else {
                float p=i==0?draft.road:i==1?draft.fire:draft.police;
                long fund=i==0?m.roadFund:i==1?m.fireFund:m.policeFund;
                snprintf(text,sizeof text,"%s: %.0f%% of $%ld = $%ld",names[i],p*100,fund,(long)(fund*p));
            }
            d.text(270,20+i*58,text);
        }
        d.text(12,328,"Cancel/ESC keeps settings; closing equals Cancel.");
    };
    draw();bool done=false,accepted=false;
    while(!done) {
        Wait(1UL<<d.win->UserPort->mp_SigBit);
        while(auto *msg=d.message()) {
            ULONG cls=msg->Class;UWORD code=msg->Code;
            int id=(cls==IDCMP_GADGETUP || cls==IDCMP_GADGETDOWN) && msg->IAddress?
                ((Gadget *)msg->IAddress)->GadgetID:0;
            d.reply(msg);
            if(cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_RAWKEY && code==0x45))done=true;
            else if(cls==IDCMP_GADGETUP) {
                if(id==1){accepted=true;done=true;}
                else if(id==3)done=true;
                else if(id==2){draft=original;
                    for(int i=0;i<4;i++) d.level(sliders[i],GTSL_Level,i==3?draft.tax:
                        (int)((i==0?draft.road:i==1?draft.fire:draft.police)*100+0.5f));
                    d.level(automatic,GTCB_Checked,draft.automatic);draw();
                } else if(id==20){draft.automatic=(automatic->Flags&GFLG_SELECTED)!=0;}
                else if(id>=10 && id<=13){draft.set(id-10,code);draw();}
            } else if(cls==IDCMP_REFRESHWINDOW) {
                d.beginRefresh();draw();d.endRefresh();d.refresh();
            }
        }
    }
    if(accepted) draft.apply(m);
    ActivateWindow(parent);
    return accepted;
}

void showCityEvaluation(Window *parent,const Micropolis &m) {
    Dialog d;
    if(!d.open(parent,"Micropolis - Evaluation (simulation paused)",590,310)){unavailable(parent);return;}
    TagItem end[]={{TAG_DONE,0}};
    d.add(BUTTON_KIND,1,185,270,220,24,"Dismiss evaluation",end);
    if(!d.valid){unavailable(parent);return;}d.attach();
    auto draw=[&]{
        d.clear(6,6,578,250);char text[120];
        d.text(12,20,"PUBLIC OPINION");d.text(12,44,"Is the mayor doing a good job?");
        snprintf(text,sizeof text,"YES %d%%    NO %d%%",m.cityYes,100-m.cityYes);d.text(12,66,text);
        d.text(12,104,"Worst problems (votes)");
        static const char *problems[]={"Crime","Pollution","Housing","Taxes","Traffic","Unemployment","Fire"};
        for(int i=0;i<CVP_PROBLEM_COMPLAINTS;i++) {
            int p=m.problemOrder[i];
            if(p>=0 && p<CVP_NUMPROBLEMS)snprintf(text,sizeof text,"%s: %d%%",problems[p],m.problemVotes[p]);
            else snprintf(text,sizeof text,"-");
            d.text(12,128+i*22,text);
        }
        d.text(322,20,"STATISTICS");
        snprintf(text,sizeof text,"Population: %ld",(long)m.cityPop);d.text(322,44,text);
        snprintf(text,sizeof text,"Migration: %+ld",(long)m.cityPopDelta);d.text(322,66,text);
        snprintf(text,sizeof text,"Value: $%ld",(long)m.cityAssessedValue);d.text(322,88,text);
        static const char *classes[]={"Village","Town","City","Capital","Metropolis","Megalopolis"};
        snprintf(text,sizeof text,"Category: %s",m.cityClass>=0 && m.cityClass<CC_NUM_CITIES?classes[m.cityClass]:"Unknown");d.text(322,110,text);
        static const char *levels[]={"Easy","Medium","Hard"};
        snprintf(text,sizeof text,"Level: %s",m.gameLevel>=0 && m.gameLevel<LEVEL_COUNT?levels[m.gameLevel]:"Unknown");d.text(322,132,text);
        d.text(322,174,"CITY SCORE (0-1000)");
        snprintf(text,sizeof text,"Score: %d",m.cityScore);d.text(322,196,text);
        snprintf(text,sizeof text,"Annual change: %+d",m.cityScoreDelta);d.text(322,218,text);
        d.text(12,248,"Last engine evaluation; opening does not reroll opinion.");
    };draw();bool done=false;
    while(!done){Wait(1UL<<d.win->UserPort->mp_SigBit);while(auto *msg=d.message()){
        ULONG cls=msg->Class;UWORD code=msg->Code;d.reply(msg);
        if(cls==IDCMP_CLOSEWINDOW || cls==IDCMP_GADGETUP || (cls==IDCMP_RAWKEY && code==0x45))done=true;
        else if(cls==IDCMP_REFRESHWINDOW){d.beginRefresh();draw();d.endRefresh();d.refresh();}
    }}
    ActivateWindow(parent);
}


int showGameCommands(Window *parent,bool soundEnabled) {
    Dialog d;
    if(!d.open(parent,"Micropolis - Game menu",300,448)){unavailable(parent);return 0;}
    char soundLabel[32];
    snprintf(soundLabel,sizeof soundLabel,"Sound effects: %s",soundEnabled?"on":"off");
    const char *labels[]={"Open city (L)","Save city (S)","Budget (F2)","Evaluation (F3)",
        "Messages (F4)","Graphs (F5)","Screen / window (F10)","Choose screen mode (F9)","Pause / run (Space)","Simulation speed (F6)",soundLabel,"Quit game (Esc)"};
    static const int commands[]={0x28,0x21,0x51,0x52,0x53,0x54,0x59,0x58,0x40,0x55,0x100,0x45};
    TagItem end[]={{TAG_DONE,0}};
    for(int i=0;i<12;i++)d.add(BUTTON_KIND,i+1,12,10+i*32,276,25,labels[i],end);
    d.add(BUTTON_KIND,13,70,410,160,25,"Close menu",end);
    if(!d.valid){unavailable(parent);return 0;}d.attach();
    int command=0;bool done=false;
    while(!done){Wait(1UL<<d.win->UserPort->mp_SigBit);while(auto *msg=d.message()){
        ULONG cls=msg->Class;UWORD code=msg->Code;
        int id=cls==IDCMP_GADGETUP && msg->IAddress?((Gadget *)msg->IAddress)->GadgetID:0;
        d.reply(msg);
        if(cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_RAWKEY && code==0x45))done=true;
        else if(cls==IDCMP_GADGETUP && id>=1 && id<=13){if(id<=12)command=commands[id-1];done=true;}
        else if(cls==IDCMP_REFRESHWINDOW){d.beginRefresh();d.endRefresh();d.refresh();}
    }}
    ActivateWindow(parent);return command;
}


int showSimulationSpeed(Window *parent,int selected) {
    Dialog d;
    if(!d.open(parent,"Micropolis - Simulation speed",300,210)){unavailable(parent);return -1;}
    const char *names[]={"Pause","Slow","Medium","Fast"};
    char labels[4][32];TagItem end[]={{TAG_DONE,0}};
    for(int i=0;i<4;i++) {
        snprintf(labels[i],sizeof labels[i],"%s%s",i==selected?"> ":"",names[i]);
        d.add(BUTTON_KIND,i+1,12,10+i*34,276,26,labels[i],end);
    }
    d.add(BUTTON_KIND,5,70,164,160,26,"Cancel",end);
    if(!d.valid){unavailable(parent);return -1;}d.attach();
    bool done=false;int result=-1;
    while(!done){Wait(1UL<<d.win->UserPort->mp_SigBit);while(auto *msg=d.message()){
        ULONG cls=msg->Class;UWORD code=msg->Code;
        int id=cls==IDCMP_GADGETUP && msg->IAddress?((Gadget *)msg->IAddress)->GadgetID:0;
        d.reply(msg);
        if(cls==IDCMP_CLOSEWINDOW || (cls==IDCMP_RAWKEY && code==0x45))done=true;
        else if(cls==IDCMP_GADGETUP && id>=1 && id<=5){if(id<=4)result=id-1;done=true;}
        else if(cls==IDCMP_REFRESHWINDOW){d.beginRefresh();d.endRefresh();d.refresh();}
    }}
    ActivateWindow(parent);return result;
}

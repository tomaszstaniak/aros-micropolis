#include "game-menu.h"
#include <intuition/intuition.h>
#include <libraries/gadtools.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>

namespace {
#define CMD(x) (APTR)(IPTR)GameCommand::x
NewMenu entries[]={
 {NM_TITLE,(STRPTR)"Micropolis",nullptr,0,0,nullptr},
 {NM_ITEM,(STRPTR)"About",nullptr,0,0,CMD(About)},
 {NM_ITEM,(STRPTR)"Save",(STRPTR)"S",0,0,CMD(Save)},
 {NM_ITEM,(STRPTR)"Save As...",nullptr,0,0,CMD(SaveAs)},
 {NM_ITEM,(STRPTR)"Choose City...",(STRPTR)"N",0,0,CMD(ChooseCity)},
 {NM_ITEM,(STRPTR)"Load City...",(STRPTR)"L",0,0,CMD(Load)},
 {NM_ITEM,NM_BARLABEL,nullptr,0,0,nullptr},
 {NM_ITEM,(STRPTR)"Quit",(STRPTR)"Q",0,0,CMD(Quit)},
 {NM_TITLE,(STRPTR)"Options",nullptr,0,0,nullptr},
 {NM_ITEM,(STRPTR)"Auto Budget",nullptr,CHECKIT|MENUTOGGLE,0,CMD(AutoBudget)},
 {NM_ITEM,(STRPTR)"Auto Bulldoze",nullptr,CHECKIT|MENUTOGGLE,0,CMD(AutoBulldoze)},
 {NM_ITEM,(STRPTR)"Disasters",nullptr,CHECKIT|MENUTOGGLE,0,CMD(DisastersEnabled)},
 {NM_ITEM,(STRPTR)"Sound",nullptr,CHECKIT|MENUTOGGLE,0,CMD(Sound)},
 {NM_ITEM,(STRPTR)"Animation",nullptr,CHECKIT|MENUTOGGLE,0,CMD(Animation)},
 {NM_ITEM,(STRPTR)"Messages",nullptr,CHECKIT|MENUTOGGLE,0,CMD(Messages)},
 {NM_ITEM,(STRPTR)"Notices",nullptr,CHECKIT|MENUTOGGLE,0,CMD(Notices)},
 {NM_ITEM,(STRPTR)"Chalk Overlay",nullptr,CHECKIT|MENUTOGGLE,0,CMD(ChalkOverlay)},
 {NM_TITLE,(STRPTR)"Disasters",nullptr,0,0,nullptr},
 {NM_ITEM,(STRPTR)"Monster",nullptr,0,0,CMD(Monster)},
 {NM_ITEM,(STRPTR)"Fire",nullptr,0,0,CMD(Fire)},
 {NM_ITEM,(STRPTR)"Flood",nullptr,0,0,CMD(Flood)},
 {NM_ITEM,(STRPTR)"Meltdown",nullptr,0,0,CMD(Meltdown)},
 {NM_ITEM,(STRPTR)"Tornado",nullptr,0,0,CMD(Tornado)},
 {NM_ITEM,(STRPTR)"Earthquake",nullptr,0,0,CMD(Earthquake)},
 {NM_TITLE,(STRPTR)"Priority",nullptr,0,0,nullptr},
 {NM_ITEM,(STRPTR)"Pause",nullptr,CHECKIT,0,CMD(Pause)},
 {NM_ITEM,(STRPTR)"Slow",nullptr,CHECKIT,0,CMD(Slow)},
 {NM_ITEM,(STRPTR)"Medium",nullptr,CHECKIT,0,CMD(Medium)},
 {NM_ITEM,(STRPTR)"Fast",nullptr,CHECKIT,0,CMD(Fast)},
 {NM_TITLE,(STRPTR)"Windows",nullptr,0,0,nullptr},
 {NM_ITEM,(STRPTR)"Budget",nullptr,0,0,CMD(Budget)},
 {NM_ITEM,(STRPTR)"Evaluation",nullptr,0,0,CMD(Evaluation)},
 {NM_ITEM,(STRPTR)"Graphs",nullptr,0,0,CMD(Graphs)},
 {NM_ITEM,(STRPTR)"Overview",nullptr,0,0,CMD(Overview)},
 {NM_ITEM,(STRPTR)"Messages",nullptr,0,0,CMD(MessageHistory)},
 {NM_ITEM,NM_BARLABEL,nullptr,0,0,nullptr},
 {NM_ITEM,(STRPTR)"Zoom In (+)",nullptr,0,0,CMD(ZoomIn)},
 {NM_ITEM,(STRPTR)"Zoom Out (-)",nullptr,0,0,CMD(ZoomOut)},
 {NM_ITEM,(STRPTR)"Zoom 100% (0)",nullptr,0,0,CMD(ZoomNormal)},
 {NM_ITEM,NM_BARLABEL,nullptr,0,0,nullptr},
 {NM_ITEM,(STRPTR)"Screen / Workbench",nullptr,0,0,CMD(DisplayToggle)},
 {NM_ITEM,(STRPTR)"Choose Screen Mode...",nullptr,0,0,CMD(DisplayMode)},
 {NM_END,nullptr,nullptr,0,0,nullptr}
};
#undef CMD
MenuItem *findItem(Menu *menu,GameCommand command){for(Menu *m=menu;m;m=m->NextMenu)for(MenuItem *i=m->FirstItem;i;i=i->NextItem){if((GameCommand)(IPTR)GTMENUITEM_USERDATA(i)==command)return i;for(MenuItem*s=i->SubItem;s;s=s->NextItem)if((GameCommand)(IPTR)GTMENUITEM_USERDATA(s)==command)return s;}return nullptr;}
}
GameMenu::~GameMenu(){detach();}
bool GameMenu::attach(Window *window){detach();if(!window)return false;visual_=GetVisualInfoA(window->WScreen,nullptr);if(!visual_)return false;menu_=CreateMenusA(entries,nullptr);if(!menu_||!LayoutMenusA(menu_,visual_,nullptr)){detach();return false;}if(!SetMenuStrip(window,menu_)){detach();return false;}window_=window;if(!ModifyIDCMP(window,window->IDCMPFlags|IDCMP_MENUPICK)){detach();return false;}return true;}
void GameMenu::detach(){if(window_&&menu_)ClearMenuStrip(window_);if(menu_)FreeMenus(menu_);if(visual_)FreeVisualInfo(visual_);window_=nullptr;menu_=nullptr;visual_=nullptr;}
GameCommand GameMenu::pick(unsigned short code)const{if(!menu_||code==MENUNULL)return GameCommand::None;auto *i=ItemAddress(menu_,code);return i?(GameCommand)(IPTR)GTMENUITEM_USERDATA(i):GameCommand::None;}
void GameMenu::checked(GameCommand command,bool value){if(auto*i=findItem(menu_,command)){if(value)i->Flags|=CHECKED;else i->Flags&=~CHECKED;}}

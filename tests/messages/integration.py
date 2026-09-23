#!/usr/bin/env python3
from pathlib import Path
import os
import subprocess
import tempfile

root = Path(os.environ.get('MICROPOLIS_TEST_ROOT', Path(__file__).resolve().parents[2]))
work_dir = subprocess.check_output(
    ['bash', '-c', 'source "$1"; printf "%s" "$WORK_DIR"',
     'message-integration', str(root / 'scripts/env.sh')], text=True)
engine_headers = Path(work_dir) / 'packages/micropolis-engine/src'
if not (engine_headers / 'text.h').is_file():
    raise SystemExit('Missing engine headers; run bash scripts/bootstrap.sh first.')
source = (root / 'src/game.cpp').read_text()
start = source.index('class FrontendCallback : public Callback {')
end = source.index('\n};', start) + len('\n};')
callback = source[start:end]
# Exercise the real outer-loop routing, not a second hand-written dispatcher.
poll_line = next(line.strip() for line in source.splitlines() if 'int messageAction=' in line)
poll_line = poll_line.replace('messageWindow.', 'ui.').replace('cb->history', 'history').replace('gotoX','x').replace('gotoY','y')
routing_start = source.index('} else if(messageAction>1) {') + len('} else if(messageAction>1) {')
routing = source[routing_start:source.index('\n        }', routing_start)]


stub = r'''
#pragma once
#include <cstdint>
#include <deque>
#include <string>
#include <utility>
#include <vector>
using ULONG=unsigned long;
using UWORD=unsigned short;
using IPTR=uintptr_t;
using STRPTR=char *;
constexpr int TRUE=1,FALSE=0,FS_NORMAL=0,FPF_ROMFONT=0,JAM1=1,SELECTDOWN=1;
constexpr UWORD MENUNULL=0xffff;
enum {TAG_DONE,WA_CustomScreen,WA_Title,WA_InnerWidth,WA_InnerHeight,WA_Left,
      WA_Top,WA_Activate,WA_DragBar,WA_DepthGadget,WA_CloseGadget,WA_SimpleRefresh,
      WA_RMBTrap,WA_IDCMP};
enum {IDCMP_CLOSEWINDOW=1,IDCMP_RAWKEY=2,IDCMP_REFRESHWINDOW=4,IDCMP_MOUSEBUTTONS=8,
      IDCMP_INTUITICKS=16,IDCMP_MENUPICK=32};
struct Menu {};
enum {BACKGROUNDPEN,TEXTPEN,SHINEPEN,SHADOWPEN};
struct TextAttr {STRPTR ta_Name;int ta_YSize,style,flags;};
struct TextFont {int tf_YSize=8,tf_Baseline=6;};
struct Screen {
    int WBorLeft=4,WBorRight=4,WBorTop=4,WBorBottom=4;
    TextAttr *Font;
    int BarHeight=12,Width=1024,Height=768;
};
struct RastPort {std::vector<std::string> output;};
struct Message {virtual ~Message()=default;};
struct IntuiMessage : Message {ULONG Class=0;UWORD Code=0;int MouseX=0,MouseY=0;};
struct MsgPort {std::deque<Message *> queue;};
struct Window {
    Screen *WScreen=nullptr;
    RastPort *RPort=nullptr;
    MsgPort *UserPort=nullptr;
    ULONG IDCMPFlags=0;
    int BorderLeft=4,BorderTop=13;
    int LeftEdge=0,TopEdge=0,Width=0,Height=0;
    Menu *MenuStrip=nullptr;
};
struct DrawInfo {UWORD dri_Pens[4]={0,1,2,3};};
inline int opens=0,closes=0,focus=0,replies=0,fonts=0;
inline Window *active=nullptr;
inline void ActivateWindow(Window *win) {active=win;++focus;}
inline void WindowToFront(Window *) {++focus;}
inline void SetWindowTitles(Window *,const char *,const char *) {}
inline TextFont *OpenFont(TextAttr *) {++fonts;return new TextFont;}
inline void CloseFont(TextFont *font) {--fonts;delete font;}
inline void collectTags(std::vector<IPTR> &tags) {tags.push_back(TAG_DONE);}
template<class T,class... Rest>
void collectTags(std::vector<IPTR> &tags,T value,Rest... rest) {
    tags.push_back(static_cast<IPTR>(value));
    if constexpr(sizeof...(rest)) collectTags(tags,rest...);
}
template<class... Args> Window *OpenWindowTags(void *,Args... args) {
    std::vector<IPTR> tags;
    collectTags(tags,args...);
    auto *win=new Window;
    win->RPort=new RastPort;win->UserPort=new MsgPort;
    ++opens;
    for(size_t i=0;i+1<tags.size() && tags[i]!=TAG_DONE;i+=2) {
        if(tags[i]==WA_CustomScreen) win->WScreen=reinterpret_cast<Screen *>(tags[i+1]);
        if(tags[i]==WA_Activate && tags[i+1]) ActivateWindow(win);
        if(tags[i]==WA_IDCMP) win->IDCMPFlags=tags[i+1];
        if(tags[i]==WA_Left) win->LeftEdge=int(tags[i+1]);
        if(tags[i]==WA_Top) win->TopEdge=int(tags[i+1]);
        // Outer size as the default screen borders and topaz title imply.
        if(tags[i]==WA_InnerWidth) win->Width=int(tags[i+1])+8;
        if(tags[i]==WA_InnerHeight) win->Height=int(tags[i+1])+17;
    }
    return win;
}
inline Message *GetMsg(MsgPort *port) {
    if(port->queue.empty()) return nullptr;
    auto *msg=port->queue.front();port->queue.pop_front();return msg;
}
inline void ReplyMsg(Message *msg) {++replies;delete msg;}
inline void CloseWindow(Window *win) {
    if(active==win) active=nullptr;
    ++closes;delete win->RPort;delete win->UserPort;delete win;
}
inline DrawInfo *GetScreenDrawInfo(Screen *) {static DrawInfo info;return &info;}
inline void FreeScreenDrawInfo(Screen *,DrawInfo *) {}
inline void SetFont(RastPort *,TextFont *) {}
inline int TextLength(RastPort *,const char *,size_t length) {return int(length)*8;}
inline void Text(RastPort *rp,const char *value,size_t length) {rp->output.emplace_back(value,length);}
inline void RectFill(RastPort *rp,int,int,int,int) {rp->output.clear();}
inline void Move(RastPort *,int,int) {}
inline void Draw(RastPort *,int,int) {}
inline void SetAPen(RastPort *,int) {}
inline void SetDrMd(RastPort *,int) {}
inline void BeginRefresh(Window *) {}
// Application menus are modelled here; game-menu.cpp is not linked.
inline int menuStrips=0;
#define MICROPOLIS_SHARED_MENU_STUB 1
inline void EndRefresh(Window *,int) {}
'''
fixture = r'''
#include "native-stub.h"
#include "message-window.h"
#include <algorithm>
#include <cstdio>
#include <type_traits>

GameMenu::~GameMenu(){detach();}
bool GameMenu::attach(Window *window){
    detach();if(!window)return false;window_=window;menu_=new Menu;
    window_->MenuStrip=menu_;window_->IDCMPFlags|=IDCMP_MENUPICK;++menuStrips;return true;
}
void GameMenu::detach(){
    if(window_&&window_->MenuStrip==menu_)window_->MenuStrip=nullptr;
    if(menu_){delete menu_;--menuStrips;}window_=nullptr;menu_=nullptr;visual_=nullptr;
}
GameCommand GameMenu::pick(unsigned short code)const{return code==MENUNULL?GameCommand::None:GameCommand::About;}
void GameMenu::checked(GameCommand,bool){}
struct Micropolis {long cityTime=123;};
namespace emscripten {struct val {};}
struct Callback {
    virtual ~Callback()=default;
    virtual void sendMessage(Micropolis *,emscripten::val,int,int,int,bool,bool)=0;
    virtual void newGame(Micropolis *,emscripten::val)=0;
};
#include "demand-model.h"
#include "disaster-presentation.h"
#include "game-audio.h"
// Only the audio output boundary is recorded here. Decoder/AHI ownership are
// exercised separately by tests/audio; presentation models above are real.
static std::vector<std::string> requestedSounds;
bool GameAudio::play(const char *name) {requestedSounds.emplace_back(name);return true;}
void GameAudio::close() {}
static void showCityBudget(Window *,Micropolis &) {}
#include "chalk-overlay.h"
#include "unsaved-city.h"
// The real loop maps menu-code poll results through the menu; plain keys pass.
static int forwardAction(int action) {return action;}
CALLBACK
static int checks=0,failures=0;
static void check(bool ok,const char *name) {
    ++checks;failures+=!ok;
    std::printf("%s %s\n",ok?"PASS":"FAIL",name);
}
static bool rendered(MessageWindow &ui,const char *text) {
    if(!ui.window()) return false;
    const auto &output=ui.window()->RPort->output;
    return std::find(output.begin(),output.end(),text)!=output.end();
}
static void event(MessageWindow &ui,ULONG cls,UWORD code=0,int x=0,int y=0) {
    auto *msg=new IntuiMessage;
    msg->Class=cls;msg->Code=code;
    msg->MouseX=x+ui.window()->BorderLeft;msg->MouseY=y+ui.window()->BorderTop;
    ui.window()->UserPort->queue.push_back(msg);
}
template<class Send,class Reset>
void exercise(MessageHistory &history,Send send,Reset reset) {
    TextAttr attr{nullptr,8,0,0};Screen screen;screen.Font=&attr;
    Window parent;parent.WScreen=&screen;active=&parent;
    MessageWindow ui;
    const int before=opens,focusBefore=focus;
    send(20,0,0,true,false,123);
    check(history.entries().size()==1,"callback accepts engine ID 20");
    if(history.entries().size()!=1) return;
    const auto first=history.entries().back();
    check(first.id==20 && first.x==0 && first.y==0 && first.picture && !first.important &&
          first.time==123 && first.text=="Fire reported !","ID 20 preserves origin, flags, cityTime and text");
    ui.refresh(history);
    check(!ui.window() && opens==before && focus==focusBefore && active==&parent,
          "message arrival and closed refresh never open or focus a window");
    check(ui.open(&parent,history),"explicit open succeeds");
    if(!ui.window()) return;
    check(opens==before+1 && active==ui.window() && focus>focusBefore,"explicit open activates history window");
    check(rendered(ui,"Fire reported !") && rendered(ui,"Message 1 of 1    Jul 1902") &&
          rendered(ui,"Click the picture to go there.") && !rendered(ui,"IMPORTANT") &&
          !rendered(ui,"ID:") && !rendered(ui,"City time"),
           "actual MessageWindow draws ID 20 text with player-facing date and location hint");
    check((ui.window()->IDCMPFlags & IDCMP_INTUITICKS)==0,"history window does not subscribe to simulation ticks");
    int x=777,y=888;
    auto *const opened=ui.window();
    const int tickFocus=focus,tickReplies=replies;
    event(ui,IDCMP_INTUITICKS);
    event(ui,IDCMP_INTUITICKS);
    event(ui,IDCMP_RAWKEY,0x40);
    check(ui.poll(history,x,y)==0x40 && x==777 && y==888 && replies==tickReplies+3 &&
          ui.window()==opened && active==opened && focus==tickFocus && opens==before+1,
          "queued ticks do not starve Space and do not change focus or goto coordinates");
    event(ui,IDCMP_INTUITICKS);
    event(ui,IDCMP_INTUITICKS);
    check(ui.poll(history,x,y)==0 && x==777 && y==888 && replies==tickReplies+5,
          "window ticks return no simulation action without changing goto coordinates");
    check(ui.window()==opened && active==opened && focus==tickFocus && opens==before+1,
          "tick forwarding preserves active history window without changing focus");
    event(ui,IDCMP_INTUITICKS);
    event(ui,IDCMP_RAWKEY,0x53);
    check(ui.poll(history,x,y)==0x53 && x==777 && y==888 && replies==tickReplies+7 &&
          ui.window()==opened && active==opened && focus==tickFocus,
          "F4 forwards rawkey 0x53 distinctly from tick and goto without changing focus");
    check(ui.poll(history,x,y)==0,"forwarded tick and F4 events leave no pending action");
    int uiCommand=0x21; // Save chosen in F1; stale tick remains on history port.
    event(ui,IDCMP_INTUITICKS);
    ROUTE_POLL
    if(messageAction>1) { ROUTE_BODY }
    check(uiCommand==0x21,"pending F1 Save survives history-window tick");
    check(ui.poll(history,x,y)==0,"deferred native ticks are ignored after pending command");
    event(ui,IDCMP_INTUITICKS);
    event(ui,IDCMP_RAWKEY,0x40);
    event(ui,IDCMP_RAWKEY,0x45);
    event(ui,IDCMP_MOUSEBUTTONS,SELECTDOWN,200,250);
    event(ui,IDCMP_REFRESHWINDOW);
    x=777;y=888;
    check(ui.poll(history,x,y,true)==0 && ui.window()==opened && x==777 && y==888 &&
          rendered(ui,"Fire reported !") && ui.poll(history,x,y)==0,
          "post-modal drain drops ticks, keys and goto but refreshes without closing on stale Escape");
    active=&parent;
    const int afterOpen=focus;
    send(40,-1,-1,false,true,171);
    check(history.entries().size()==2,"callback accepts engine ID 40");
    const auto second=history.entries().back();
    check(second.id==40 && second.x==-1 && second.y==-1 && !second.picture && second.important &&
          second.time==171 && !second.hasLocation(),"ID 40 preserves unavailable coordinates and independent flags");
    ui.refresh(history);
    check(rendered(ui,"Brownouts, build another Power Plant.") && rendered(ui,"No map location for this message.") &&
          rendered(ui,"Message 2 of 2    Jul 1903    IMPORTANT"),"actual MessageWindow renders engine 40 as brownouts");
    x=777;y=888;
    event(ui,IDCMP_MOUSEBUTTONS,SELECTDOWN,200,250);
    check(ui.poll(history,x,y)==0 && x==777 && y==888,"unlocated message cannot emit goto");
    event(ui,IDCMP_MOUSEBUTTONS,SELECTDOWN,16,250);
    check(ui.poll(history,x,y)==0 && rendered(ui,"Fire reported !"),"Older navigates to fire");
    event(ui,IDCMP_MOUSEBUTTONS,SELECTDOWN,200,250);
    check(ui.poll(history,x,y)==1 && x==0 && y==0,"Go to location returns exact tile origin");
    event(ui,IDCMP_MOUSEBUTTONS,SELECTDOWN,108,250);
    check(ui.poll(history,x,y)==0 && rendered(ui,"Brownouts, build another Power Plant."),"Newer navigates to brownouts");
    event(ui,IDCMP_REFRESHWINDOW);
    ui.poll(history,x,y);
    check(focus==afterOpen && active==&parent && opens==before+1,
          "arrival, refresh event and navigation never steal focus or reopen");
    event(ui,IDCMP_CLOSEWINDOW);
    event(ui,IDCMP_MOUSEBUTTONS,SELECTDOWN,200,250);
    const int replied=replies;
    ui.poll(history,x,y);
    check(!ui.window() && replies==replied+2 && history.entries().size()==2 && fonts==0,
          "close drains queued events and retains history");
    check(ui.open(&parent,history) && rendered(ui,"Brownouts, build another Power Plant."),
          "reopen restores retained latest message");
    if(!ui.window()) return;
    event(ui,IDCMP_MOUSEBUTTONS,SELECTDOWN,16,250);
    ui.poll(history,x,y);
    check(rendered(ui,"Fire reported !"),"select located message before reset");
    event(ui,IDCMP_MOUSEBUTTONS,SELECTDOWN,200,250);
    const auto revision=history.revision();
    reset();
    check(history.entries().empty() && history.revision()>revision,"newGame clears history and invalidates UI revision");
    x=777;y=888;
    check(ui.poll(history,x,y)==0 && x==777 && y==888 && rendered(ui,"No messages yet."),
          "newGame before queued goto cannot return stale coordinates");
    send(20,119,99,false,true,900);
    ui.refresh(history);
    event(ui,IDCMP_MOUSEBUTTONS,SELECTDOWN,200,250);
    check(ui.poll(history,x,y)==1 && x==119 && y==99 && rendered(ui,"Press Go to location to see the place."),
          "post-reset message uses new identity and exact far-edge coordinates");
    event(ui,IDCMP_INTUITICKS);
    event(ui,IDCMP_CLOSEWINDOW);
    event(ui,IDCMP_RAWKEY,0x40);
    check(ui.poll(history,x,y,true)==0 && !ui.window() && fonts==0,
          "post-modal drain honors close and replies to trailing input");
    ui.close();active=nullptr;
}
template<class T,class=void> struct HasHistory : std::false_type {};
template<class T> struct HasHistory<T,std::void_t<decltype(std::declval<T &>().history)>> : std::true_type {};
template<class T> void integration(T &cb) {
    if constexpr(HasHistory<T>::value) {
        Micropolis engine;
        Callback &dispatch=cb;
        exercise(cb.history,[&](int id,int x,int y,bool picture,bool important,long time) {
            engine.cityTime=time;
            dispatch.sendMessage(&engine,{},id,x,y,picture,important);
        },[&] {dispatch.newGame(&engine,{});});
    } else {
        check(false,"actual FrontendCallback has no history: callback integration is missing");
    }
}
int main() {
    std::puts("INTEGRATION: extracted FrontendCallback -> MessageHistory -> actual MessageWindow");
    FrontendCallback cb;integration(cb);
    Micropolis engine;
    cb.startEarthquake(&engine,{},300);
    check(cb.earthquake.active() && cb.earthquake.x()!=0,"actual callback activates real earthquake presentation");
    cb.sendMessage(&engine,{},MESSAGE_SCENARIO_WON,-1,-1,true,true);
    check(cb.dialogShown && cb.scenario.take()==ScenarioPresentation::Won,
          "actual victory message queues result and interrupts batch without win callback");
    cb.dialogShown=false;
    cb.sendMessage(&engine,{},MESSAGE_SCENARIO_WON,-1,-1,true,true);
    check(!cb.dialogShown && cb.scenario.take()==ScenarioPresentation::None,"duplicate callback does not reopen scenario result");
    cb.didLoadCity(&engine,{},"city.cty");
    check(!cb.earthquake.active(),"successful load callback clears quake");
    cb.sendMessage(&engine,{},MESSAGE_SCENARIO_LOST,-1,-1,true,true);
    cb.didLoseGame(&engine,{});
    check(cb.dialogShown && cb.scenario.take()==ScenarioPresentation::Lost && cb.scenario.take()==ScenarioPresentation::None,
          "loss message and callback create exactly one pending result");
    cb.startEarthquake(&engine,{},1000);cb.newGame(&engine,{});
    check(!cb.earthquake.active() && cb.scenario.take()==ScenarioPresentation::None,"new game clears presentation state");
    cb.makeSound(&engine,{},"city","ExplosionLow",-1,-1);
    check(requestedSounds.empty(),"detached startup callback safely ignores audio");
    GameAudio audio;cb.audio=&audio;
    cb.makeSound(&engine,{},"city","ExplosionLow",-1,-1);
    check(requestedSounds==std::vector<std::string>{"ExplosionLow"},"actual callback forwards exact earthquake sound to audio boundary");
    cb.audio=nullptr;
    std::puts("UI ISOLATION: real history/window with stubbed native transport (not callback integration)");
    MessageHistory history;
    exercise(history,[&](int id,int x,int y,bool picture,bool important,long time) {
        history.receive(id,x,y,picture,important,time);
    },[&] {history.reset();});
    check(opens==closes && fonts==0,"all native stub resources released");
    std::printf("RESULT: %d/%d passed\n",checks-failures,checks);
    return failures?1:0;
}
'''.replace('CALLBACK', callback).replace('ROUTE_POLL', poll_line).replace('ROUTE_BODY', routing)

with tempfile.TemporaryDirectory(prefix='micropolis-message-integration-') as directory:
    out = Path(directory)
    (out / 'native-stub.h').write_text(stub)
    (out / 'micropolis.h').write_text('#pragma once\n#include <text.h>\n')
    for name in ('intuition/intuition.h', 'intuition/screens.h', 'graphics/text.h',
                 'proto/exec.h', 'proto/intuition.h', 'proto/graphics.h'):
        header = out / name
        header.parent.mkdir(exist_ok=True)
        header.write_text('#include "native-stub.h"\n')
    cpp = out / 'test.cpp'
    cpp.write_text(fixture)
    exe = out / 'test'
    subprocess.run([os.environ.get('CXX', 'clang++'), '-std=c++17', '-Wall', '-Wextra',
                    '-Werror', '-pedantic', '-g', '-fsanitize=address,undefined',
                    '-fno-sanitize-recover=all', '-I', str(out), '-I', str(root / 'src'),
                    '-I', str(engine_headers), str(cpp), str(root / 'src/messages.cpp'),
                    str(root / 'src/message-window.cpp'), '-o', str(exe)], check=True)
    env = os.environ.copy()
    env['ASAN_OPTIONS'] = env.get('ASAN_OPTIONS', '') + ':halt_on_error=1:abort_on_error=1'
    env['UBSAN_OPTIONS'] = env.get('UBSAN_OPTIONS', '') + ':halt_on_error=1'
    raise SystemExit(subprocess.run([str(exe)], env=env).returncode)

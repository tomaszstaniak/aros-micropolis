#include "game-menu.h"
#include "simulation-control.h"
#include "message-window.h"
#include "notice-preview.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/text.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>

namespace {
constexpr int width=580,height=280;
constexpr int bodyTop=76,lineHeight=12,rows=12;
constexpr int buttonTop=244,buttonHeight=24;
constexpr int buttonLeft[]={12,104,196,384,476};
constexpr int buttonWidth[]={80,80,176,80,92};

bool gameKey(unsigned int code) {
    return code==0x50 || code==0x51 || code==0x52 || code==0x53 || code==0x54 || code==0x55 || code==0x58 ||
           code==0x59 || code==0x40 || code==0x36;
}

}

void MessageWindow::configurePreview(void *context,MessagePreviewRenderer renderer,bool enabled) {
    bool changed=previewContext_!=context||previewRenderer_!=renderer||noticesEnabled_!=enabled;
    previewContext_=context;previewRenderer_=renderer;noticesEnabled_=enabled;
    if(changed&&win_){wrap();draw();}
}

MessageWindow::~MessageWindow() {
    close();
}

bool MessageWindow::open(Window *parent,const MessageHistory &history) {
    if(win_) {
        refresh(history);
        return true;
    }
    if(!parent || !parent->WScreen) return false;
    auto *screen=parent->WScreen;
    const int outerWidth=width+screen->WBorLeft+screen->WBorRight;
    const int outerHeight=height+screen->WBorTop+screen->WBorBottom+
        screen->Font->ta_YSize+1;
    const int top=screen->BarHeight+4;
    if(outerWidth>screen->Width || outerHeight>screen->Height-top) return false;
    TextAttr attr={(STRPTR)"topaz.font",8,FS_NORMAL,FPF_ROMFONT};
    font_=OpenFont(&attr);
    if(!font_) return false;
    int left=(screen->Width-outerWidth)/2,topEdge=std::max(top,(screen->Height-outerHeight)/2);
    placement.place(screen->Width,screen->Height,top,outerWidth,outerHeight,left,topEdge);
    win_=OpenWindowTags(NULL,WA_CustomScreen,(IPTR)screen,
        WA_Title,(IPTR)title_.c_str(),WA_InnerWidth,width,WA_InnerHeight,height,
        WA_Left,left,WA_Top,topEdge,
        WA_Activate,TRUE,WA_DragBar,TRUE,WA_DepthGadget,TRUE,WA_CloseGadget,TRUE,
        WA_SimpleRefresh,TRUE,WA_RMBTrap,FALSE,
        WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_RAWKEY|IDCMP_REFRESHWINDOW|IDCMP_MOUSEBUTTONS,
        TAG_DONE);
    if(!win_) {
        close();
        return false;
    }
    SetFont(win_->RPort,font_);
    if(!menu_.attach(win_)){close();return false;}
    if(auto *info=GetScreenDrawInfo(screen)) {
        background_=info->dri_Pens[BACKGROUNDPEN];
        foreground_=info->dri_Pens[TEXTPEN];
        shine_=info->dri_Pens[SHINEPEN];
        shadow_=info->dri_Pens[SHADOWPEN];
        FreeScreenDrawInfo(screen,info);
    }
    refresh(history);
    return true;
}

void MessageWindow::close() {
    if(win_) {
        while(auto *msg=GetMsg(win_->UserPort)) ReplyMsg(msg);
        placement.remember(win_->LeftEdge,win_->TopEdge,win_->Width,win_->Height);
        menu_.detach();
        CloseWindow(win_);
        win_=nullptr;
    }
    if(font_) {
        CloseFont(font_);
        font_=nullptr;
    }
    selected_=CityMessage{};
    revision_=latest_=0;
    synchronized_=false;
    index_=total_=page_=0;
    lines_.clear();
    background_=0;foreground_=1;shine_=2;shadow_=1;
}

Window *MessageWindow::window() const {
    return win_;
}

void MessageWindow::select(const CityMessage &message) {
    selected_=message;
    page_=0;
    wrap();
}

bool MessageWindow::synchronize(const MessageHistory &history) {
    if(synchronized_ && revision_==history.revision()) return false;
    const auto &entries=history.entries();
    const bool follow=total_==0 || selected_.serial==latest_;
    revision_=history.revision();
    synchronized_=true;
    total_=entries.size();
    if(entries.empty()) {
        selected_=CityMessage{};
        latest_=0;
        index_=page_=0;
        lines_.clear();
        return true;
    }
    index_=total_-1;
    if(!follow) {
        index_=0;
        while(index_<total_ && entries[index_].serial<selected_.serial) ++index_;
        if(index_==total_) index_=total_-1;
    }
    if(selected_.serial!=entries[index_].serial) select(entries[index_]);
    latest_=entries.back().serial;
    return true;
}

void MessageWindow::wrap() {
    lines_.clear();
    std::string value=selected_.text;
    for(char &c:value) if((unsigned char)c<32 && c!='\n') c=' ';
    size_t start=0;
    while(start<value.size()) {
        size_t end=start,space=std::string::npos;
        int pixels=0;
        while(end<value.size() && value[end]!='\n') {
            const int next=TextLength(win_->RPort,value.data()+end,1);
            const int available=noticePreviewVisible(selected_,noticesEnabled_)?width-176:width-24;
            if(pixels+next>available) break;
            pixels+=next;
            if(value[end]==' ') space=end;
            ++end;
        }
        if(end==start && value[end]!='\n') ++end;
        if(end<value.size() && value[end]!='\n' && space!=std::string::npos && space>start)
            end=space;
        lines_.push_back(value.substr(start,end-start));
        start=end;
        if(start<value.size() && value[start]=='\n') ++start;
        else while(start<value.size() && value[start]==' ') ++start;
    }
}

void MessageWindow::text(int x,int y,const char *value) {
    Move(win_->RPort,win_->BorderLeft+x,win_->BorderTop+y);
    Text(win_->RPort,value,strlen(value));
}

void MessageWindow::button(int index,const char *label,bool enabled) {
    auto *rp=win_->RPort;
    const int x=win_->BorderLeft+buttonLeft[index],y=win_->BorderTop+buttonTop;
    const int w=buttonWidth[index];
    SetAPen(rp,enabled?shine_:shadow_);
    Move(rp,x,y+buttonHeight-1);Draw(rp,x,y);Draw(rp,x+w-1,y);
    SetAPen(rp,shadow_);
    Draw(rp,x+w-1,y+buttonHeight-1);Draw(rp,x,y+buttonHeight-1);
    SetAPen(rp,enabled?foreground_:shadow_);
    text(buttonLeft[index]+(w-(int)TextLength(rp,label,strlen(label)))/2,
         buttonTop+(buttonHeight-font_->tf_YSize)/2+font_->tf_Baseline,label);
    if(!enabled) {
        for(int px=x+3;px<x+w-3;px+=3) {
            Move(rp,px,y+buttonHeight-4);Draw(rp,px,y+buttonHeight-4);
        }
    }
}

void MessageWindow::draw() {
    auto *rp=win_->RPort;
    SetDrMd(rp,JAM1);
    SetAPen(rp,background_);
    RectFill(rp,win_->BorderLeft,win_->BorderTop,
             win_->BorderLeft+width-1,win_->BorderTop+height-1);
    SetAPen(rp,foreground_);
    char value[128];
    if(total_) {
        // Player-facing labels; engine IDs and raw cityTime stay internal.
        static const char *const months[]={"Jan","Feb","Mar","Apr","May","Jun",
                                            "Jul","Aug","Sep","Oct","Nov","Dec"};
        const auto date=simulationDate(selected_.time);
        snprintf(value,sizeof value,"Message %lu of %lu    %s %d%s",
            (unsigned long)(index_+1),(unsigned long)total_,months[(date.month-1)%12],date.year,
            selected_.important?"    IMPORTANT":"");
        text(12,16,value);
        if(selected_.hasLocation())
            snprintf(value,sizeof value,"%s",
                noticePreviewVisible(selected_,noticesEnabled_)?"Click the picture to go there.":"Press Go to location to see the place.");
        else snprintf(value,sizeof value,"No map location for this message.");
        text(12,32,value);
        if(selected_.picture && !noticesEnabled_) text(12,48,"Pictures are off (Options > Notices).");
        const bool preview=noticePreviewVisible(selected_,noticesEnabled_)&&previewRenderer_;
        if(preview) {
            previewRenderer_(previewContext_,selected_,win_,12,bodyTop);
            SetAPen(rp,shine_);Move(rp,win_->BorderLeft+11,win_->BorderTop+bodyTop-1);Draw(rp,win_->BorderLeft+140,win_->BorderTop+bodyTop-1);Draw(rp,win_->BorderLeft+140,win_->BorderTop+bodyTop+128);Draw(rp,win_->BorderLeft+11,win_->BorderTop+bodyTop+128);Draw(rp,win_->BorderLeft+11,win_->BorderTop+bodyTop-1);
        }
        const int bodyLeft=preview?152:12;
        const size_t begin=page_*rows;
        for(size_t i=begin;i<lines_.size() && i<begin+rows;++i)
            text(bodyLeft,bodyTop+(int)(i-begin)*lineHeight+font_->tf_Baseline,lines_[i].c_str());
    } else {
        text(12,16,"Message 0 / 0");
        text(12,bodyTop+font_->tf_Baseline,"No messages yet.");
    }
    const size_t pages=std::max(size_t(1),(lines_.size()+rows-1)/rows);
    snprintf(value,sizeof value,"Text page %lu / %lu    Esc: close",
        (unsigned long)(page_+1),(unsigned long)pages);
    text(12,232,value);
    button(0,"Older",total_ && index_>0);
    button(1,"Newer",total_ && index_+1<total_);
    button(2,"Go to location",total_ && selected_.hasLocation());
    button(3,"Text <",page_>0);
    button(4,"Text >",page_+1<pages);
}

void MessageWindow::refresh(const MessageHistory &history) {
    if(win_ && synchronize(history)) draw();
}

int MessageWindow::poll(const MessageHistory &history,int &gotoX,int &gotoY,bool discardInput) {
    if(!win_) return 0;
    refresh(history);

    while(auto *msg=(IntuiMessage *)GetMsg(win_->UserPort)) {
        const ULONG cls=msg->Class;
        const UWORD code=msg->Code;
        const int x=msg->MouseX-win_->BorderLeft,y=msg->MouseY-win_->BorderTop;
        ReplyMsg((Message *)msg);
        if(cls==IDCMP_CLOSEWINDOW || (!discardInput && cls==IDCMP_RAWKEY && code==0x45)) {
            close();
            return 0;
        }
        if(discardInput && cls!=IDCMP_REFRESHWINDOW) continue;
        if(cls==IDCMP_INTUITICKS) {
            continue;
        }
        if(cls==IDCMP_MENUPICK) {
            if(code!=MENUNULL) return menuPickResult(code);
            continue;
        }
        if(cls==IDCMP_REFRESHWINDOW) {
            BeginRefresh(win_);
            draw();
            EndRefresh(win_,TRUE);
        } else if(cls==IDCMP_RAWKEY && gameKey(code)) {
            return code;
        } else if(cls==IDCMP_MOUSEBUTTONS && code==SELECTDOWN &&
                  x>=12&&x<140&&y>=bodyTop&&y<bodyTop+128&&
                  noticePreviewVisible(selected_,noticesEnabled_)) {
            gotoX=selected_.x;gotoY=selected_.y;return 1;
        } else if(cls==IDCMP_MOUSEBUTTONS && code==SELECTDOWN &&
                  y>=buttonTop && y<buttonTop+buttonHeight) {
            int hit=-1;
            for(int i=0;i<5;++i)
                if(x>=buttonLeft[i] && x<buttonLeft[i]+buttonWidth[i]) hit=i;
            if(hit==0 && total_ && index_>0) {
                select(history.entries()[--index_]);
                draw();
            } else if(hit==1 && total_ && index_+1<total_) {
                select(history.entries()[++index_]);
                draw();
            } else if(hit==2 && total_ && selected_.hasLocation()) {
                gotoX=selected_.x;
                gotoY=selected_.y;
                return 1;
            } else if(hit==3 && page_>0) {
                --page_;
                draw();
            } else if(hit==4 && (page_+1)*rows<lines_.size()) {
                ++page_;
                draw();
            }
        }
    }
    return 0;
}

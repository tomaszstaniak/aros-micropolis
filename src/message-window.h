#ifndef MICROPOLIS_MESSAGE_WINDOW_H
#define MICROPOLIS_MESSAGE_WINDOW_H

#include "messages.h"
#include <vector>

#include "window-placement.h"
struct Window;
struct Menu;
struct TextFont;
using MessagePreviewRenderer=void(*)(void *,const CityMessage &,Window *,int,int);

class MessageWindow {
public:
    MessageWindow() = default;
    ~MessageWindow();
    MessageWindow(const MessageWindow &) = delete;
    MessageWindow &operator=(const MessageWindow &) = delete;
    bool open(Window *parent,const MessageHistory &history);
    void close();
    // Shared application menu, attached on every open.
    void setMenu(Menu *menu){menu_=menu;}
    WindowPlacement placement;
    Window *window() const;
    void configurePreview(void *context,MessagePreviewRenderer renderer,
                          bool noticesEnabled);
    void refresh(const MessageHistory &history);
    // After a modal dialog, discard stale input but honor close/refresh.
    int poll(const MessageHistory &history,int &gotoX,int &gotoY,bool discardInput=false);

private:
    bool synchronize(const MessageHistory &history);
    void select(const CityMessage &message);
    void wrap();
    void draw();
    void text(int x,int y,const char *value);
    void button(int index,const char *label,bool enabled);

    Window *win_=nullptr;
    Menu *menu_=nullptr;
    TextFont *font_=nullptr;
    std::string title_="Micropolis - Messages";
    CityMessage selected_{};
    uint64_t revision_=0,latest_=0;
    bool synchronized_=false;
    size_t index_=0,total_=0,page_=0;
    std::vector<std::string> lines_;
    unsigned short background_=0,foreground_=1,shine_=2,shadow_=1;
    void *previewContext_=nullptr;
    MessagePreviewRenderer previewRenderer_=nullptr;
    bool noticesEnabled_=true;
};

#endif

#ifndef MICROPOLIS_GRAPH_WINDOW_H
#define MICROPOLIS_GRAPH_WINDOW_H
#include "graph-model.h"
#include "window-placement.h"
struct Window;
struct Menu;
struct TextFont;
class Micropolis;

// An independent native window, owned by the same event loop as the map.
class GraphWindow {
public:
    ~GraphWindow();
    GraphWindow()=default;
    GraphWindow(const GraphWindow&)=delete;
    GraphWindow& operator=(const GraphWindow&)=delete;
    bool open(Window *parent,Micropolis &city);
    void close();
    // Shared application menu, attached on every open.
    void setMenu(Menu *menu){menu_=menu;}
    WindowPlacement placement;
    Window *window() const{return win_;}
    void refresh(Micropolis &city);
    int poll(Micropolis &city,bool discardInput=false);
private:
    void draw();
    void text(int x,int y,const char *value);
    Window *win_=nullptr;
    Menu *menu_=nullptr;
    TextFont *font_=nullptr;
    GraphModel model_;
    int year_=0,month_=0;
    long cityTime_=0;
    unsigned short background_=0,foreground_=1;
};
#endif

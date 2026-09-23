#ifndef MICROPOLIS_GRAPH_WINDOW_H
#define MICROPOLIS_GRAPH_WINDOW_H
#include "graph-model.h"
#include "window-placement.h"
#include "game-menu.h"
struct Window;
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
    // Every window owns its MenuStrip; Intuition forbids attaching one Menu
    // object to several windows at once.
    void checked(GameCommand command,bool value){menu_.checked(command,value);}
    WindowPlacement placement;
    Window *window() const{return win_;}
    void refresh(Micropolis &city);
    int poll(Micropolis &city,bool discardInput=false);
private:
    void draw();
    void text(int x,int y,const char *value);
    Window *win_=nullptr;
    GameMenu menu_;
    TextFont *font_=nullptr;
    GraphModel model_;
    int year_=0,month_=0;
    long cityTime_=0;
    unsigned short background_=0,foreground_=1;
};
#endif

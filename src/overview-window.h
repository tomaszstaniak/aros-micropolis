#ifndef MICROPOLIS_OVERVIEW_WINDOW_H
#define MICROPOLIS_OVERVIEW_WINDOW_H

#include "overview-model.h"
#include "chalk-overlay.h"
#include <vector>
#include "window-placement.h"
struct Window;
struct Menu;
struct TextFont;
class Micropolis;
struct DemandModel;

class OverviewWindow {
public:
    ~OverviewWindow();
    bool open(Window *parent,Micropolis &city,const DemandModel &demand,
              int cameraX,int cameraY,int columns,int rows);
    void close();
    // Shared application menu, attached on every open.
    void setMenu(Menu *menu){menu_=menu;}
    WindowPlacement placement;
    Window *window() const{return win_;}
    void refresh(Micropolis &city,const DemandModel &demand,
                 int cameraX,int cameraY,int columns,int rows);
    // 1: camera changed; 2: open Evaluation; >2: forwarded game rawkey.
    int poll(Micropolis &city,const DemandModel &demand,int &cameraX,int &cameraY,
             int columns,int rows,bool discardInput=false);
    OverviewLayer layer() const{return layer_;}
    // Classic DrawMapInk: the city's chalk also appears on the small map.
    void setChalk(const ChalkOverlay *chalk){chalk_=chalk;}
private:
    void draw(Micropolis &city,const DemandModel &demand,
              int cameraX,int cameraY,int columns,int rows);
    void text(int x,int y,const char *value);
    Window *win_=nullptr;
    Menu *menu_=nullptr;
    const ChalkOverlay *chalk_=nullptr;
    TextFont *font_=nullptr;
    OverviewLayer layer_=OverviewLayer::All;
    std::vector<int> tiles_,samples_;
    std::vector<unsigned int> pixels_;
    bool panning_=false;
    unsigned short background_=0,foreground_=1,shine_=2,shadow_=1;
};
#endif

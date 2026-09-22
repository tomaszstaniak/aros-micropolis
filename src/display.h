#ifndef MICROPOLIS_DISPLAY_H
#define MICROPOLIS_DISPLAY_H
#include "view-geometry.h"
#include "window-placement.h"
#include <intuition/intuition.h>
#include <intuition/screens.h>
#include <graphics/rastport.h>
#include <cstdint>
#include <vector>
struct RequestedDisplayMode { ULONG id=0; int width=0,height=0,depth=0; };
// 1 = selected, 0 = cancelled, -1 = requester unavailable.
int chooseRequestedDisplayMode(Window *parent, RequestedDisplayMode &mode);
struct GameDisplay {
    Screen *screen=nullptr;
    Window *window=nullptr;
    BitMap *bitmap=nullptr;
    RastPort raster{};
    bool custom=false;
    ViewGeometry view;
    int tile=16; // requested zoom; resize() applies it
    std::vector<uint32_t> world;  // 16 px per tile, see view-geometry.h
    std::vector<uint32_t> pixels; // screen frame, scaled from world
    ~GameDisplay();
    // placement: where the player last left the map window on this kind of display.
    bool open(const RequestedDisplayMode *mode,const WindowPlacement *placement=nullptr);
    bool resize();
    GameDisplay() = default;
    GameDisplay(const GameDisplay &) = delete;
    GameDisplay &operator=(const GameDisplay &) = delete;
};
#endif

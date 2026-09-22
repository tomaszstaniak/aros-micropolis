#ifndef MICROPOLIS_DEMAND_MODEL_H
#define MICROPOLIS_DEMAND_MODEL_H
#include <algorithm>

// Residential/commercial/industrial demand ("valve") indicator.
//
// Engine side: Micropolis::drawValve() (update.cpp) clamps r/c/i to
// [-1500,1500] and calls callback->updateDemand(this,val,r,c,i) only when a
// value actually changed (update.cpp:229-238). FrontendCallback::updateDemand
// in src/game.cpp is currently `{}` (a no-op) - this model is what a real
// implementation stores.
//
// Classic side: whead.tcl:428-468 draws three canvas rectangles (r/c/i) from
// a shared baseline; UISetDemand (micropolis.tcl:2703-2725) sets each bar's
// far edge at `baseline - value`, with the baseline itself flipped between
// two fixed y-coordinates depending on the value's sign - i.e. a
// zero-crossing, signed bar, not a value clamped to always be positive.
//
// This header only keeps the latest values and maps them to a signed pixel
// height; it draws nothing and knows nothing about Intuition.
struct DemandModel {
    float residential=0, commercial=0, industrial=0;
    bool has=false; // false until the first updateDemand callback arrives
    void update(float r,float c,float i) {
        residential=r; commercial=c; industrial=i; has=true;
    }
    void reset() { residential=commercial=industrial=0; has=false; }
};

// Signed bar half-height in pixels for one demand value, clamped to the
// engine's own [-1500,1500] valve range and scaled linearly so the zero
// valve is always the baseline (barHeight(0,h)==0), matching the classic
// zero-crossing behaviour at a native pixel scale rather than whead.tcl's
// fixed 8px-tall canvas rectangle.
inline int demandBarHeight(float value,int maxHeight) {
    float v=std::clamp(value,-1500.0f,1500.0f);
    if(maxHeight<0)maxHeight=0;
    return (int)(v*maxHeight/1500.0f);
}

#endif

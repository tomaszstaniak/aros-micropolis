#ifndef MICROPOLIS_WINDOW_PLACEMENT_H
#define MICROPOLIS_WINDOW_PLACEMENT_H
#include <algorithm>
// Where the player last put a window. A different screen may be smaller,
// so a remembered position is clamped to keep the whole window visible.
struct WindowPlacement {
    int left=-1,top=-1,width=0,height=0;
    bool known() const { return left>=0 && top>=0; }
    void remember(int l,int t,int w,int h) { left=l;top=t;width=w;height=h; }
    // outerW/outerH: size about to be opened; minTop keeps the title bar
    // below the screen bar.
    void place(int screenW,int screenH,int minTop,int outerW,int outerH,
               int &l,int &t) const {
        if(!known()) return;
        l=std::max(0,std::min(left,screenW-outerW));
        t=std::max(minTop,std::min(top,screenH-outerH));
    }
    // Resizable windows also keep their size, within the screen.
    void size(int screenW,int screenH,int minTop,int minW,int minH,int &w,int &h) const {
        if(!known() || width<=0 || height<=0) return;
        w=std::max(minW,std::min(width,screenW));
        h=std::max(minH,std::min(height,screenH-minTop));
    }
};
#endif

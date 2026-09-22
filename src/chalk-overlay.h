#ifndef MICROPOLIS_CHALK_OVERLAY_H
#define MICROPOLIS_CHALK_OVERLAY_H
// Classic annotation tools (micropolis-activity src/sim/w_tool.c). Strokes
// live in 16 px/tile world pixels. As in the original, the overlay belongs
// to the running city and is not written into the city file.
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <vector>

struct ChalkStroke {
    std::vector<int> points; // x0,y0,x1,y1,...
    int left=0,top=0,right=0,bottom=0;
    size_t length() const { return points.size()/2; }
};

class ChalkOverlay {
public:
    void start(int x,int y) {
        strokes_.push_back({});
        drawing_=true;
        add(x,y);
    }
    void extend(int x,int y) {
        if(!drawing_) return;
        auto &s=strokes_.back();
        if(s.points[s.points.size()-2]==x && s.points.back()==y) return;
        add(x,y);
    }
    void finish() { drawing_=false; }
    bool drawing() const { return drawing_; }
    // EraserTo: any stroke touching the 17x17 box around the pointer goes.
    bool eraseAt(int x,int y) {
        const size_t before=strokes_.size();
        strokes_.erase(std::remove_if(strokes_.begin(),strokes_.end(),
            [&](const ChalkStroke &s){return inBox(s,x-8,y-8,x+8,y+8);}),strokes_.end());
        if(strokes_.size()!=before){++revision_;drawing_=false;return true;}
        return false;
    }
    void clear() { if(!strokes_.empty())++revision_; strokes_.clear(); drawing_=false; }
    const std::vector<ChalkStroke> &strokes() const { return strokes_; }
    uint64_t revision() const { return revision_; }

    // InkInBox: bounding box first, then every segment's bounding box.
    static bool inBox(const ChalkStroke &s,int left,int top,int right,int bottom) {
        if(left>s.right || right<s.left || top>s.bottom || bottom<s.top) return false;
        if(s.length()==1) return true;
        for(size_t i=1;i<s.length();++i) {
            const int ax=s.points[2*i-2],ay=s.points[2*i-1],bx=s.points[2*i],by=s.points[2*i+1];
            if(left<=std::max(ax,bx) && right>=std::min(ax,bx) &&
               top<=std::max(ay,by) && bottom>=std::min(ay,by)) return true;
        }
        return false;
    }

private:
    void add(int x,int y) {
        auto &s=strokes_.back();
        if(s.points.empty()){s.left=s.right=x;s.top=s.bottom=y;}
        s.points.push_back(x);s.points.push_back(y);
        s.left=std::min(s.left,x);s.right=std::max(s.right,x);
        s.top=std::min(s.top,y);s.bottom=std::max(s.bottom,y);
        ++revision_;
    }
    std::vector<ChalkStroke> strokes_;
    bool drawing_=false;
    uint64_t revision_=0;
};

namespace chalk_detail {
inline void plot(uint32_t *frame,int w,int h,int x,int y,int radius,uint32_t color) {
    for(int dy=-radius;dy<=radius;++dy)for(int dx=-radius;dx<=radius;++dx){
        const int px=x+dx,py=y+dy;
        if(px>=0 && py>=0 && px<w && py<h) frame[(size_t)py*w+px]=color;
    }
}
inline void line(uint32_t *frame,int w,int h,int x0,int y0,int x1,int y1,int radius,uint32_t color) {
    const int dx=std::abs(x1-x0),sx=x0<x1?1:-1,dy=-std::abs(y1-y0),sy=y0<y1?1:-1;
    int err=dx+dy;
    for(;;) {
        plot(frame,w,h,x0,y0,radius,color);
        if(x0==x1 && y0==y1) break;
        const int e2=2*err;
        if(e2>=dy){err+=dy;x0+=sx;}
        if(e2<=dx){err+=dx;y0+=sy;}
    }
}
}

// Editor view: a 3 px white line (XSetLineAttributes width 3) and a 6 px
// dot for a single click. frame covers world pixels from (originX,originY).
inline void drawChalk(const ChalkOverlay &overlay,uint32_t *frame,int w,int h,
                      int originX,int originY,uint32_t color=0xffffffff) {
    for(const auto &s:overlay.strokes()) {
        if(s.right<originX-3 || s.left>originX+w+3 || s.bottom<originY-3 || s.top>originY+h+3) continue;
        if(s.length()==1) { chalk_detail::plot(frame,w,h,s.points[0]-originX,s.points[1]-originY,3,color); continue; }
        for(size_t i=1;i<s.length();++i)
            chalk_detail::line(frame,w,h,s.points[2*i-2]-originX,s.points[2*i-1]-originY,
                               s.points[2*i]-originX,s.points[2*i+1]-originY,1,color);
    }
}

// Overview (DrawMapInk): one pixel wide, scaled by 3/16 like the 3 px/tile map.
inline void drawChalkOverview(const ChalkOverlay &overlay,uint32_t *frame,int w,int h,
                              uint32_t color=0xffffffff) {
    for(const auto &s:overlay.strokes()) {
        if(s.length()==1) { chalk_detail::plot(frame,w,h,s.points[0]*3/16,s.points[1]*3/16,0,color); continue; }
        for(size_t i=1;i<s.length();++i)
            chalk_detail::line(frame,w,h,s.points[2*i-2]*3/16,s.points[2*i-1]*3/16,
                               s.points[2*i]*3/16,s.points[2*i+1]*3/16,0,color);
    }
}
#endif

#ifndef MICROPOLIS_VIEW_GEOMETRY_H
#define MICROPOLIS_VIEW_GEOMETRY_H
#include <algorithm>
#include <cstddef>
#include <cstdint>
// Zoom changes only screen pixels per city tile (8, 16 or 32). The world
// frame stays 16 px per tile, so tiles, sprites, footprint, chalk and quake
// share one renderer and only the finished frame is scaled.
struct ViewGeometry {
    int width=640, height=448, tile=16;
    static ViewGeometry fromInner(int w, int h, int tile=16) {
        return {std::max(tile, std::min(120*tile, w / tile * tile)),
                std::max(tile, std::min(100*tile, h / tile * tile)), tile};
    }
    int columns() const { return width / tile; }
    int rows() const { return height / tile; }
    int worldWidth() const { return columns() * 16; }
    int worldHeight() const { return rows() * 16; }
    void clampCamera(int &x, int &y) const {
        x=std::max(0, std::min(x, 120-columns()));
        y=std::max(0, std::min(y, 100-rows()));
    }
    bool contains(int x, int y) const {
        return x>=0 && y>=0 && x<width && y<height;
    }
    int mapX(int x, int camera) const { return camera + x/tile; }
    int mapY(int y, int camera) const { return camera + y/tile; }
    // Screen pixel to 16 px world-frame pixel.
    int worldX(int x) const { return x*16/tile; }
    int worldY(int y) const { return y*16/tile; }
};

// Power-of-two steps keep the classic pixel art undistorted.
inline int zoomTileStep(int tile, int direction) {
    if(direction>0) return tile>=32 ? 32 : tile*2;
    if(direction<0) return tile<=8 ? 8 : tile/2;
    return 16;
}
inline int zoomPercent(int tile) { return tile*100/16; }

// World frame (16 px per tile) to screen frame. 32 px doubles each pixel;
// 8 px averages 2x2 blocks so one-pixel road and rail lines do not vanish.
inline void scaleWorldFrame(const uint32_t *world, int worldW, int worldH,
                            uint32_t *screen, int tile) {
    if(tile==32) {
        const int w=worldW*2;
        for(int y=0;y<worldH;++y) {
            uint32_t *a=screen+(size_t)(2*y)*w, *b=a+w;
            const uint32_t *s=world+(size_t)y*worldW;
            for(int x=0;x<worldW;++x){a[2*x]=a[2*x+1]=b[2*x]=b[2*x+1]=s[x];}
        }
    } else if(tile==8) {
        const int w=worldW/2,h=worldH/2;
        for(int y=0;y<h;++y) {
            const uint32_t *s0=world+(size_t)(2*y)*worldW, *s1=s0+worldW;
            uint32_t *d=screen+(size_t)y*w;
            for(int x=0;x<w;++x) {
                const uint32_t p[4]={s0[2*x],s0[2*x+1],s1[2*x],s1[2*x+1]};
                uint32_t out=0;
                for(int shift=0;shift<32;shift+=8) {
                    uint32_t sum=2;
                    for(uint32_t v:p) sum+=(v>>shift)&0xff;
                    out|=(sum/4)<<shift;
                }
                d[x]=out;
            }
        }
    } else {
        std::copy(world, world+(size_t)worldW*worldH, screen);
    }
}
#endif

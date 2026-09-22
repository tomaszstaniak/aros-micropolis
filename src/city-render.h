#ifndef MICROPOLIS_CITY_RENDER_H
#define MICROPOLIS_CITY_RENDER_H
#include "bmp.h"
#include "micropolis.h"
#include <cstring>
#include <vector>
inline void renderCityTiles(Micropolis &city,const BmpImage &sheet,
                            std::vector<uint32_t> &pixels,int width,int height,
                            int cameraX,int cameraY) {
    constexpr int tileSize=16,sheetCols=32,tileCount=960;
    if(width<=0||height<=0||sheet.width<sheetCols*tileSize){pixels.clear();return;}
    pixels.assign((size_t)width*height,0xff000000);
    const int columns=width/tileSize,rows=height/tileSize;
    for(int ty=0;ty<rows;++ty)for(int tx=0;tx<columns;++tx){int mx=cameraX+tx,my=cameraY+ty,tile=0;if(mx>=0&&mx<WORLD_W&&my>=0&&my<WORLD_H)tile=city.map[mx][my]&0x03ff;if(tile<0||tile>=tileCount)tile=0;const uint32_t *src=&sheet.pixels[(size_t)(tile/sheetCols)*tileSize*sheet.width+(size_t)(tile%sheetCols)*tileSize];for(int py=0;py<tileSize;++py)memcpy(&pixels[(size_t)(ty*tileSize+py)*width+tx*tileSize],src+(size_t)py*sheet.width,tileSize*sizeof(uint32_t));}
}
#endif

#ifndef MICROPOLIS_SPRITES_H
#define MICROPOLIS_SPRITES_H
#include "bmp.h"
#include "micropolis.h"
#include <array>
struct SpriteArt {
    std::array<std::vector<BmpImage>,9> frames;
    bool load(const std::string &directory,std::string &error);
};
// The pinned original frames are RGBA8 PNGs, at most 48x48 (limit 64).
bool loadSpritePng(const std::string &path,BmpImage &out,std::string &error);
// Camera is in map tiles; sprites and their drawing offsets are world pixels.
size_t renderSprites(const SpriteArt &art,const SimSprite *sprites,
                     std::vector<uint32_t> &pixels,int width,int height,int camX,int camY);
#endif

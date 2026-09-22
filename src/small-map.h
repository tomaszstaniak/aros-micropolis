#ifndef MICROPOLIS_SMALL_MAP_H
#define MICROPOLIS_SMALL_MAP_H

#include "overview-model.h"
#include <cstddef>
#include <cstdint>
#include <vector>

// Exact X11 colours used by the classic renderer (w_x.c:382-395).
constexpr uint32_t smallMapLightGray = 0xffbfbfbf;
constexpr uint32_t smallMapYellow    = 0xffffff00;
constexpr uint32_t smallMapOrange    = 0xffff7f00;
constexpr uint32_t smallMapRed       = 0xffff0000;
constexpr uint32_t smallMapDarkGreen = 0xff007f00;
constexpr uint32_t smallMapLightGreen= 0xff00e600;
constexpr uint32_t smallMapLightBlue = 0xff6666e6;

inline uint32_t smallMapColor(OverviewIntensityLevel level) {
    switch(level) {
        case OverviewIntensityLevel::Low: return smallMapLightGray;
        case OverviewIntensityLevel::Medium: return smallMapYellow;
        case OverviewIntensityLevel::High: return smallMapOrange;
        case OverviewIntensityLevel::VeryHigh: return smallMapRed;
        default: return 0;
    }
}

inline uint32_t smallMapGrowthColor(OverviewGrowthLevel level) {
    switch(level) {
        case OverviewGrowthLevel::VeryMinus: return smallMapYellow;
        case OverviewGrowthLevel::Minus: return smallMapOrange;
        case OverviewGrowthLevel::Plus: return smallMapDarkGreen;
        case OverviewGrowthLevel::VeryPlus: return smallMapLightGreen;
        default: return 0;
    }
}

inline int smallMapAtlasTile(int rawTile,int tileCount) {
    if(tileCount<=0)return 0;
    int tile=rawTile & (int)overviewLOMASK;
    // The classic renderer subtracts TILE_COUNT once for its animation bank.
    // The imported atlas contains 960 entries while the current engine can
    // expose 0..1023, so 960..1023 deliberately fall back to 0..63.
    if(tile>=tileCount)tile-=tileCount;
    return tile>=0 && tile<tileCount?tile:0;
}

inline bool smallMapFilterKeeps(OverviewLayer layer,int rawTile) {
    switch(layer) {
        case OverviewLayer::Residential:return overviewIsResidential(rawTile);
        case OverviewLayer::Commercial:return overviewIsCommercial(rawTile);
        case OverviewLayer::Industrial:return overviewIsIndustrial(rawTile);
        case OverviewLayer::Transportation:return overviewIsTransportation(rawTile);
        default:return true;
    }
}

inline int smallMapBaseTile(OverviewLayer layer,int rawTile,int tileCount) {
    bool keep=smallMapFilterKeeps(layer,rawTile);
    if(layer==OverviewLayer::TrafficDensity)keep=overviewIsTransportation(rawTile);
    if(layer==OverviewLayer::PowerGrid) {
        const int low=rawTile&(int)overviewLOMASK;
        keep=low<=63 || overviewPowerState(rawTile)!=OverviewPowerState::Background;
    }
    return keep?smallMapAtlasTile(rawTile,tileCount):0;
}

// `atlas` is the original 4x3-per-tile tilessm.xpm strip. The fourth column
// is padding; a world tile becomes exactly 3x3 output pixels. `samples` is one
// already-expanded value per world tile for data layers and may be null for
// zone layers. Invalid dimensions produce an empty output.
inline void renderSmallMap(const uint32_t *atlas,int tileCount,
                           const int *tiles,const int *samples,
                           int worldWidth,int worldHeight,OverviewLayer layer,
                           std::vector<uint32_t> &out) {
    if(!atlas || !tiles || tileCount<=0 || worldWidth<=0 || worldHeight<=0) {
        out.clear();return;
    }
    out.assign((size_t)worldWidth*3*worldHeight*3,0xff000000);
    const int outputWidth=worldWidth*3;
    for(int y=0;y<worldHeight;++y)for(int x=0;x<worldWidth;++x) {
        const size_t index=(size_t)y*worldWidth+x;
        const int raw=tiles[index];
        int tile=smallMapBaseTile(layer,raw,tileCount);
        const uint32_t *src=atlas+(size_t)tile*12;
        uint32_t solid=0;
        if(samples) {
            const int value=samples[index];
            switch(layer) {
                case OverviewLayer::PopulationDensity:
                case OverviewLayer::LandValue:
                case OverviewLayer::CrimeRate:
                case OverviewLayer::TrafficDensity:
                case OverviewLayer::FireCoverage:
                case OverviewLayer::PoliceCoverage:
                    solid=smallMapColor(overviewIntensityLevel(value));break;
                case OverviewLayer::PollutionDensity:
                    solid=smallMapColor(overviewPollutionLevel(value));break;
                case OverviewLayer::RateOfGrowth:
                    solid=smallMapGrowthColor(overviewGrowthLevel(value));break;
                default:break;
            }
        }
        if(layer==OverviewLayer::PowerGrid) {
            switch(overviewPowerState(raw)) {
                case OverviewPowerState::Powered:solid=smallMapRed;break;
                case OverviewPowerState::Unpowered:solid=smallMapLightBlue;break;
                case OverviewPowerState::Conductive:solid=smallMapLightGray;break;
                default:break;
            }
        }
        for(int py=0;py<3;++py)for(int px=0;px<3;++px) {
            out[(size_t)(y*3+py)*outputWidth+x*3+px]=solid?solid:src[py*4+px];
        }
    }
}

#endif

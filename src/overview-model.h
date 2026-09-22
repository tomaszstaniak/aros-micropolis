#ifndef MICROPOLIS_OVERVIEW_MODEL_H
#define MICROPOLIS_OVERVIEW_MODEL_H
#include <algorithm>
#include <cstdint>

// Classic reference: SimHacker/micropolis c98f6b08, wmap.tcl:90-111 creates
// a mapview sized WorldX*3 by WorldY*3 (3 overview pixels per world tile);
// wmap.tcl:114-239 lists the zone/overlay radio choices below, in this
// order. This header is the pure model (layer identity, tile/overlay
// classification, colour and camera-rectangle math); it has no Intuition
// dependency so it can be host-tested (tests/overview/test.cpp).
enum class OverviewLayer {
    All=0, Residential, Commercial, Industrial, Transportation,
    PopulationDensity, RateOfGrowth, LandValue, CrimeRate,
    PollutionDensity, TrafficDensity, PowerGrid, FireCoverage,
    PoliceCoverage, Count
};

inline const char *overviewLayerName(OverviewLayer layer) {
    static const char *names[]={
        "All","Residential","Commercial","Industrial","Transportation",
        "Population Density","Rate of Growth","Land Value","Crime Rate",
        "Pollution Density","Traffic Density","Power Grid",
        "Fire Coverage","Police Coverage"};
    int i=(int)layer;
    return (i>=0 && i<(int)OverviewLayer::Count)?names[i]:"";
}

// The engine's own get*() accessors do not all take full 120x100 tile
// coordinates: Micropolis::get* wraps Map<DATA,BLKSIZE>::get(), which
// indexes in the map's *block* space, not Map<>::worldGet(). Measured
// 2026-09-20 from micropolis-engine/src/micropolis.cpp and map_type.h:
//   getPopulationDensity/getCrimeRate/getLandValue/getPollutionDensity/
//   getTrafficDensity  -> MapByte2,  block 2  (0..WORLD_W_2/WORLD_H_2)
//   getRateOfGrowth/getFireCoverage/getPoliceCoverage -> MapShort8/MapByte4-
//   backed maps, block 8 (0..WORLD_W_8/WORLD_H_8)
//   getPowerGrid -> calls powerGridMap.worldGet(), already full tile res.
//   getTile -> raw map[x][y], full tile res, LOMASK still applies.
// A caller sampling tile (x,y) must divide by this block size before
// calling the engine accessor; do not index these as if they were 120x100.
inline int overviewLayerBlockSize(OverviewLayer layer) {
    switch(layer) {
        case OverviewLayer::RateOfGrowth:
        case OverviewLayer::FireCoverage:
        case OverviewLayer::PoliceCoverage:
            return 8;
        case OverviewLayer::PopulationDensity:
        case OverviewLayer::CrimeRate:
        case OverviewLayer::LandValue:
        case OverviewLayer::PollutionDensity:
        case OverviewLayer::TrafficDensity:
            return 2;
        default:
            return 1; // All/zone layers and PowerGrid: full tile resolution.
    }
}

constexpr int overviewWorldW=120, overviewWorldH=100;
constexpr uint32_t overviewLOMASK=0x03ff; // tool.h LOMASK; strips ANIMBIT/BULLBIT/etc.

// Tile buckets for the "All" bucket colouring and for naming what a raw
// tile *is*. wmap.tcl's zone radio list is only All/Residential/
// Commercial/Industrial/Transportation; the single-zone filters below
// (overviewIsResidential/Commercial/Industrial/Transportation) are the
// ones that actually decide what those five choices highlight, ported
// tile-range-for-tile-range from the classic drawRes/drawCom/drawInd/
// drawLilTransMap in g_smmaps.c. OverviewZone exists for "All" bucket
// colours and is intentionally finer than the radio list: 612..1023 is
// NOT one "Industrial" blob. From INDBASE(612) the classic tile enum
// (micropolis.h) runs empty/built industrial zones to 692, then a run of
// named specials with their own tile ranges: Port(693..708),
// Airport(709..744), the coal PowerPlant(745..760), FireStation
// (761..769), PoliceStation(770..778), Stadium(779..810), the nuclear
// PowerPlant(811..826), and from 827 up, non-zone tiles: lightning bolt,
// bridge/radar animation frames, the fountain, a second industrial-tile
// bank at 844 (INDBASE2/TELEBASE), explosions, coal-smoke chimney
// animation, football-game and church overlay frames — bucketed here as
// Animation since none of them is a zone tile in the classic sense
// (verified: none is in RBRDR..LASTZONE, macros.h:102-104, so none can
// ever be a zone's *centre* tile). Do not read 612..1023 as "industry":
// only 612..692 is.
enum class OverviewZone {
    Dirt, Water, Other, Road, Rail, Power, Residential, Commercial,
    Industrial, Port, Airport, PowerPlant, FireStation, PoliceStation,
    Stadium, Animation
};

// Boundaries from micropolis.h tile enum (also cross-checked against the
// classic build's own copy at micropolis-activity/src/sim/headers/sim.h,
// same numeric values): DIRT=0, water 2..20 (RIVER..LASTRIVEDGE), road
// 64..206 (HBRIDGE..LASTROAD), power 208..223 (HPOWER..LASTPOWER, includes
// rail/power overlaps), rail 224..239 (HRAIL..LASTRAIL plus fire),
// residential 240..422 (RESBASE..), commercial 423..611 (COMBASE..
// COMLAST=609, plus 610-611 unlabeled but still commercial-range per
// drawCom's own `tile>609` cutoff), industrial 612..692 (INDBASE..IND9),
// PORTBASE=693..LASTPORT=708, AIRPORTBASE=709..744 (AIRPORT centre 716,
// rest unlabeled taxiway/terminal tiles), COALBASE=745..LASTPOWERPLANT=760,
// FIRESTBASE=761..769, POLICESTBASE=770..778, STADIUMBASE=779..810
// (includes FULLSTADIUM=800), NUCLEARBASE=811..LASTZONE=826; 827..1023 is
// the Animation bucket described above.
inline OverviewZone overviewClassifyTile(int rawTile) {
    int tile=rawTile & (int)overviewLOMASK;
    if(tile==0) return OverviewZone::Dirt;
    if(tile>=2 && tile<=20) return OverviewZone::Water;
    if(tile>=64 && tile<=206) return OverviewZone::Road;
    if(tile>=208 && tile<=223) return OverviewZone::Power;
    if(tile>=224 && tile<=239) return OverviewZone::Rail;
    if(tile>=240 && tile<=422) return OverviewZone::Residential;
    if(tile>=423 && tile<=611) return OverviewZone::Commercial;
    if(tile>=612 && tile<=692) return OverviewZone::Industrial;
    if(tile>=693 && tile<=708) return OverviewZone::Port;
    if(tile>=709 && tile<=744) return OverviewZone::Airport;
    if(tile>=745 && tile<=760) return OverviewZone::PowerPlant;
    if(tile>=761 && tile<=769) return OverviewZone::FireStation;
    if(tile>=770 && tile<=778) return OverviewZone::PoliceStation;
    if(tile>=779 && tile<=810) return OverviewZone::Stadium;
    if(tile>=811 && tile<=826) return OverviewZone::PowerPlant; // nuclear
    if(tile>=827 && tile<=1023) return OverviewZone::Animation;
    return OverviewZone::Other;
}

// Exact ports of the classic single-zone filters (g_smmaps.c:185-224,
// SimHacker/micropolis c98f6b08). Each classic drawXxx() walks every tile
// and sets it to 0 (dirt) when the *condition below* is true, i.e. the
// condition is "hide this tile from the Xxx overlay". These predicates
// return the opposite (true = the overlay keeps/highlights the tile) so
// callers read them as "is this tile part of the Xxx overlay", but the
// tile ranges are copied verbatim, not re-derived from OverviewZone —
// deliberately, since e.g. drawInd's own cutoffs do not line up with the
// zone table above (it hides 240..611 twice-removed, 693..851, 860..883
// and >=932, which is not the same shape as OverviewZone::Industrial).
inline bool overviewIsResidential(int rawTile) {
    int tile=rawTile & (int)overviewLOMASK;
    return !(tile>422); // g_smmaps.c:188
}
inline bool overviewIsCommercial(int rawTile) {
    int tile=rawTile & (int)overviewLOMASK;
    return !((tile>609) || (tile>=232 && tile<423)); // g_smmaps.c:197-198
}
inline bool overviewIsIndustrial(int rawTile) {
    int tile=rawTile & (int)overviewLOMASK;
    return !((tile>=240 && tile<=611) || (tile>=693 && tile<=851) ||
             (tile>=860 && tile<=883) || (tile>=932)); // g_smmaps.c:207-210
}
inline bool overviewIsTransportation(int rawTile) {
    int tile=rawTile & (int)overviewLOMASK;
    return !((tile>=240) || (tile>=207 && tile<=220) ||
             (tile==223)); // g_smmaps.c:219-221 (drawLilTransMap)
}

// Power overlay state, exactly as drawPower (g_smmaps.c:233-362) derives
// it from the *unmasked* tile value: tiles below 64 are never touched by
// this overlay at all (background passthrough, `pix=-1`, drawn as the
// plain tile); at/above 64, ZONEBIT decides powered/unpowered *zone*
// centres, otherwise CONDBIT decides a conductive (wire/rail/power line)
// tile, and anything else is background dirt. This needs the bits, so it
// takes the raw tile including PWRBIT/CONDBIT/ZONEBIT — do not mask with
// overviewLOMASK before calling it.
enum class OverviewPowerState { Background, Conductive, Powered, Unpowered };
inline OverviewPowerState overviewPowerState(int rawTile) {
    constexpr int kPwrBit=0x8000, kCondBit=0x4000, kZoneBit=0x0400;
    int low=rawTile & (int)overviewLOMASK;
    if((unsigned)low<=63) return OverviewPowerState::Background;
    if(rawTile & kZoneBit)
        return (rawTile & kPwrBit)?OverviewPowerState::Powered:OverviewPowerState::Unpowered;
    if(rawTile & kCondBit) return OverviewPowerState::Conductive;
    return OverviewPowerState::Background;
}

// Approximate classic minimap bucket colours (0xAARRGGBB). This is a
// legible adaptation, not a pixel-sampled palette from wmap.tcl, which the
// classic Tk build draws through its own mini-tile renderer.
inline uint32_t overviewZoneColor(OverviewZone zone) {
    switch(zone) {
        case OverviewZone::Water:        return 0xff0000a0;
        case OverviewZone::Dirt:         return 0xff785030;
        case OverviewZone::Road:
        case OverviewZone::Rail:         return 0xff909090;
        case OverviewZone::Power:        return 0xffffff00;
        case OverviewZone::Residential:  return 0xff00c000;
        case OverviewZone::Commercial:   return 0xff0000ff;
        case OverviewZone::Industrial:   return 0xffc0a000;
        case OverviewZone::Port:         return 0xff6060c0;
        case OverviewZone::Airport:      return 0xffa0a0e0;
        case OverviewZone::PowerPlant:   return 0xffff8000;
        case OverviewZone::FireStation:  return 0xffff4040;
        case OverviewZone::PoliceStation:return 0xff4040ff;
        case OverviewZone::Stadium:      return 0xffffff80;
        case OverviewZone::Animation:    return 0xffc0a000; // same bucket as Industrial pre-fix; rare, cosmetic only
        default:                         return 0xff305030;
    }
}

constexpr uint32_t overviewDimColor=0xff202020; // non-matching tile in a single-zone filter

// Diverging colour for RateOfGrowth (signed; engine clamps roughly to a
// few hundred either side, but not to a fixed documented bound like the
// other overlays, so this clamps defensively at +-1000 for display only).
inline uint32_t overviewGrowthColor(int rate) {
    int v=std::clamp(rate,-1000,1000);
    if(v>=0) { int level=v*255/1000; return 0xff000000u | (uint32_t)(level<<8); }
    int level=(-v)*255/1000; return 0xff000000u | (uint32_t)(level<<16);
}

// GetCI's five discrete bands (g_map.c:105-112), used unmodified by
// population/traffic/crime/land value/fire/police and, for pollution,
// applied to (value+10) (g_map.c:180: `GetCI(10 + PollutionMem[x][y])`).
// Exposed separately from the colour ramp below so tests can check the
// exact 50/100/150/200 boundaries without depending on colour maths.
enum class OverviewIntensityLevel { None, Low, Medium, High, VeryHigh };
inline OverviewIntensityLevel overviewIntensityLevel(int value) {
    if(value<50) return OverviewIntensityLevel::None;
    if(value<100) return OverviewIntensityLevel::Low;
    if(value<150) return OverviewIntensityLevel::Medium;
    if(value<200) return OverviewIntensityLevel::High;
    return OverviewIntensityLevel::VeryHigh;
}
inline OverviewIntensityLevel overviewPollutionLevel(int pollution) {
    return overviewIntensityLevel(pollution+10); // g_map.c:180
}

// Rate-of-growth's five discrete bands (g_map.c:136-148): >100 very-plus,
// >20 plus, <-100 very-minus, <-20 minus, else neutral. Exposed separately
// from overviewGrowthColor so the ±20/±100 thresholds are directly
// testable.
enum class OverviewGrowthLevel { VeryMinus, Minus, Neutral, Plus, VeryPlus };
inline OverviewGrowthLevel overviewGrowthLevel(int rate) {
    if(rate>100) return OverviewGrowthLevel::VeryPlus;
    if(rate>20) return OverviewGrowthLevel::Plus;
    if(rate<-100) return OverviewGrowthLevel::VeryMinus;
    if(rate<-20) return OverviewGrowthLevel::Minus;
    return OverviewGrowthLevel::Neutral;
}

// Monotonic black-to-green-to-red ramp for the unsigned 0..255 byte
// overlays (population/traffic/pollution/crime/land value/fire/police).
inline uint32_t overviewIntensityColor(int value,int maxValue=255) {
    if(maxValue<=0) maxValue=255;
    int v=std::clamp(value,0,maxValue);
    int level=v*255/maxValue;
    if(level<128) return 0xff000000u | (uint32_t)((level*2)<<8);
    int r=(level-128)*2;
    return 0xff000000u | (uint32_t)(r<<16) | (uint32_t)((255-r)<<8);
}

constexpr uint32_t overviewPowerConductiveColor=0xffc0c0c0; // COLOR_LIGHTGRAY, g_smmaps.c:230
constexpr uint32_t overviewPowerPoweredColor=0xffff0000;    // COLOR_RED, g_smmaps.c:229
constexpr uint32_t overviewPowerUnpoweredColor=0xff80c0ff;  // COLOR_LIGHTBLUE, g_smmaps.c:228

// sample: for the zone single-filters (Residential/Commercial/Industrial/
// Transportation) and PowerGrid, the raw *unmasked* getTile() value (the
// power bits live above LOMASK, so PowerGrid must not be pre-masked); for
// PollutionDensity, the raw PollutionMem byte before the +10 offset
// (applied here, not by the caller); for the remaining overlays, the raw
// value from the matching engine accessor at its own block coords.
inline uint32_t overviewColorForSample(OverviewLayer layer,int sample) {
    switch(layer) {
        case OverviewLayer::All:
            return overviewZoneColor(overviewClassifyTile(sample));
        case OverviewLayer::Residential:
            return overviewIsResidential(sample) && overviewClassifyTile(sample)==OverviewZone::Residential?
                overviewZoneColor(OverviewZone::Residential):overviewDimColor;
        case OverviewLayer::Commercial:
            return overviewIsCommercial(sample) && overviewClassifyTile(sample)==OverviewZone::Commercial?
                overviewZoneColor(OverviewZone::Commercial):overviewDimColor;
        case OverviewLayer::Industrial:
            return overviewIsIndustrial(sample) && overviewClassifyTile(sample)==OverviewZone::Industrial?
                overviewZoneColor(OverviewZone::Industrial):overviewDimColor;
        case OverviewLayer::Transportation: {
            if(!overviewIsTransportation(sample)) return overviewDimColor;
            OverviewZone z=overviewClassifyTile(sample);
            return (z==OverviewZone::Road || z==OverviewZone::Rail || z==OverviewZone::Power)?
                overviewZoneColor(z==OverviewZone::Power?OverviewZone::Road:z):overviewDimColor;
        }
        case OverviewLayer::RateOfGrowth:
            return overviewGrowthColor(sample);
        case OverviewLayer::PollutionDensity:
            return overviewIntensityColor((sample+10<0)?0:(sample+10));
        case OverviewLayer::PowerGrid:
            switch(overviewPowerState(sample)) {
                case OverviewPowerState::Powered:    return overviewPowerPoweredColor;
                case OverviewPowerState::Unpowered:  return overviewPowerUnpoweredColor;
                case OverviewPowerState::Conductive: return overviewPowerConductiveColor;
                default:                              return overviewDimColor;
            }
        default:
            return overviewIntensityColor(sample);
    }
}

// Camera navigation, independent of Intuition. `scale` is overview pixels
// per world tile (wmap.tcl uses 3; the native window may pick another
// integer scale to fit its own client size).
struct OverviewCamera {
    // Click/drag at (px,py) in overview pixel space centers the viewport
    // there, then clamps exactly like ViewGeometry::clampCamera so the
    // camera never runs off the 120x100 world.
    static void panTo(int px,int py,int scale,int columns,int rows,int &cameraX,int &cameraY) {
        if(scale<=0)scale=1;
        int tileX=px/scale, tileY=py/scale;
        cameraX=tileX-columns/2;
        cameraY=tileY-rows/2;
        cameraX=std::max(0,std::min(cameraX,std::max(0,overviewWorldW-columns)));
        cameraY=std::max(0,std::min(cameraY,std::max(0,overviewWorldH-rows)));
    }
    // The visible-camera rectangle in overview pixel space, for drawing
    // the "you are here" box over the underlying layer.
    static void rect(int cameraX,int cameraY,int columns,int rows,int scale,
                      int &px,int &py,int &pw,int &ph) {
        if(scale<=0)scale=1;
        px=cameraX*scale; py=cameraY*scale;
        pw=std::min(columns,overviewWorldW)*scale;
        ph=std::min(rows,overviewWorldH)*scale;
    }
};

#endif

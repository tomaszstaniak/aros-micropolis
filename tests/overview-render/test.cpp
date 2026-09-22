#include "small-map.h"
#include <cstdio>
#include <vector>

static int total=0,failed=0;
static void check(bool ok,const char *name){++total;if(!ok){++failed;std::printf("FAIL: %s\n",name);}}

int main() {
    // Two synthetic four-by-three atlas entries; the fourth column is the
    // classic XPM separator and must never reach the three-pixel map cell.
    const uint32_t atlas[]={
        1,2,3,99, 4,5,6,99, 7,8,9,99,
        11,12,13,99, 14,15,16,99, 17,18,19,99
    };
    int tiles[]={1,960,-1,0};
    int samples[]={0,0,0,0};
    std::vector<uint32_t> pixels;
    renderSmallMap(atlas,2,tiles,samples,2,2,OverviewLayer::All,pixels);
    check(pixels.size()==36,"2x2 world becomes a 6x6 overview");
    check(pixels[0]==11 && pixels[1]==12 && pixels[2]==13,
          "classic atlas uses the first three columns of each 4x3 tile");
    check(pixels[3]==1 && pixels[6+3]==4,
          "out-of-range tiles use explicit dirt fallback");

    tiles[0]=240; // residential
    renderSmallMap(atlas,2,tiles,samples,1,1,OverviewLayer::Commercial,pixels);
    check(pixels[0]==1 && pixels[8]==9,
          "zone filters replace hidden tiles with classic dirt art");

    tiles[0]=0;samples[0]=50;
    renderSmallMap(atlas,2,tiles,samples,1,1,OverviewLayer::PopulationDensity,pixels);
    check(pixels[0]==smallMapColor(OverviewIntensityLevel::Low),
          "overlay values cover the base tile with the classic low colour");
    samples[0]=49;
    renderSmallMap(atlas,2,tiles,samples,1,1,OverviewLayer::PopulationDensity,pixels);
    check(pixels[0]==1,"VAL_NONE leaves the small tile visible");
    check(smallMapBaseTile(OverviewLayer::TrafficDensity,240,960)==0,
          "traffic overlay uses the classic transportation-only base");
    check(smallMapBaseTile(OverviewLayer::TrafficDensity,64,960)==64,
          "traffic overlay keeps a road under VAL_NONE");
    check(smallMapBaseTile(OverviewLayer::PowerGrid,64,960)==0,
          "plain nonconductive infrastructure is dirt in the power view");
    check(smallMapBaseTile(OverviewLayer::PowerGrid,20,960)==20,
          "terrain below tile 64 remains visible in the power view");

    std::printf("RESULT: %d/%d passed\n",total-failed,total);
    return failed?1:0;
}

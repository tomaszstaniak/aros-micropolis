#include "overview-model.h"
#include <cstdio>
#include <string>

int main() {
    int total=0,failed=0;
    auto check=[&](bool ok,const char *why){++total;failed+=!ok;printf("%s %s\n",ok?"PASS":"FAIL",why);};

    // Layer identity: 14 classic choices in wmap.tcl:114-239 order, plus a
    // safe out-of-range name.
    check((int)OverviewLayer::Count==14,"wmap.tcl lists 5 zone choices and 9 overlays");
    check(std::string(overviewLayerName(OverviewLayer::All))=="All","first classic choice is All");
    check(std::string(overviewLayerName(OverviewLayer::PoliceCoverage))=="Police Coverage","last classic choice is Police Coverage");
    check(std::string(overviewLayerName(OverviewLayer::Count))=="","out-of-range layer name is safe, not garbage");
    check(std::string(overviewLayerName((OverviewLayer)-1))=="","negative layer name is safe");

    // Block size: only the accessors that wrap Map<>::get() (not
    // worldGet()) need their coordinates pre-divided.
    check(overviewLayerBlockSize(OverviewLayer::All)==1,"zone/tile map is full resolution");
    check(overviewLayerBlockSize(OverviewLayer::PowerGrid)==1,"getPowerGrid uses worldGet(), already full resolution");
    check(overviewLayerBlockSize(OverviewLayer::PopulationDensity)==2,"MapByte2-backed overlay is half resolution");
    check(overviewLayerBlockSize(OverviewLayer::CrimeRate)==2,"crime rate is MapByte2, half resolution");
    check(overviewLayerBlockSize(OverviewLayer::LandValue)==2,"land value is MapByte2, half resolution");
    check(overviewLayerBlockSize(OverviewLayer::PollutionDensity)==2,"pollution is MapByte2, half resolution");
    check(overviewLayerBlockSize(OverviewLayer::TrafficDensity)==2,"traffic is MapByte2, half resolution");
    check(overviewLayerBlockSize(OverviewLayer::RateOfGrowth)==8,"rate of growth is block 8");
    check(overviewLayerBlockSize(OverviewLayer::FireCoverage)==8,"fire coverage is block 8");
    check(overviewLayerBlockSize(OverviewLayer::PoliceCoverage)==8,"police coverage is block 8");

    // Tile classification boundaries (micropolis.h tile enum ranges).
    check(overviewClassifyTile(0)==OverviewZone::Dirt,"tile 0 is dirt");
    check(overviewClassifyTile(2)==OverviewZone::Water,"tile 2 (RIVER) is water");
    check(overviewClassifyTile(20)==OverviewZone::Water,"tile 20 (LASTRIVEDGE) is still water");
    check(overviewClassifyTile(21)==OverviewZone::Other,"tile 21 (TREEBASE) is not a classic zone choice");
    check(overviewClassifyTile(64)==OverviewZone::Road,"tile 64 (HBRIDGE/ROADBASE) is road");
    check(overviewClassifyTile(206)==OverviewZone::Road,"tile 206 (LASTROAD) is still road");
    check(overviewClassifyTile(208)==OverviewZone::Power,"tile 208 (HPOWER) is power");
    check(overviewClassifyTile(224)==OverviewZone::Rail,"tile 224 (HRAIL) is rail");
    check(overviewClassifyTile(239)==OverviewZone::Rail,"tile 239 (LASTRAIL region) is still rail");
    check(overviewClassifyTile(240)==OverviewZone::Residential,"tile 240 (RESBASE) is residential");
    check(overviewClassifyTile(422)==OverviewZone::Residential,"tile 422 is still residential");
    check(overviewClassifyTile(423)==OverviewZone::Commercial,"tile 423 (COMBASE) is commercial");
    check(overviewClassifyTile(611)==OverviewZone::Commercial,"tile 611 is still commercial");
    check(overviewClassifyTile(612)==OverviewZone::Industrial,"tile 612 (INDBASE) is industrial");
    check(overviewClassifyTile(692)==OverviewZone::Industrial,"tile 692 (IND9) is still industrial");
    check(overviewClassifyTile(612 | 0x0400)==OverviewZone::Industrial,"ANIMBIT/high bits are masked off by LOMASK");

    // 693..1023 is NOT industry (review finding: it used to be lumped in).
    // Named specials from micropolis.h, cross-checked against
    // micropolis-activity/src/sim/headers/sim.h in the pinned classic repo.
    check(overviewClassifyTile(693)==OverviewZone::Port,"tile 693 (PORTBASE) is the seaport, not industry");
    check(overviewClassifyTile(708)==OverviewZone::Port,"tile 708 (LASTPORT) is still port");
    check(overviewClassifyTile(709)==OverviewZone::Airport,"tile 709 (AIRPORTBASE) is the airport");
    check(overviewClassifyTile(716)==OverviewZone::Airport,"tile 716 (AIRPORT centre) is still airport");
    check(overviewClassifyTile(745)==OverviewZone::PowerPlant,"tile 745 (COALBASE) is a power plant");
    check(overviewClassifyTile(761)==OverviewZone::FireStation,"tile 761 (FIRESTBASE) is the fire station");
    check(overviewClassifyTile(770)==OverviewZone::PoliceStation,"tile 770 (POLICESTBASE) is the police station");
    check(overviewClassifyTile(779)==OverviewZone::Stadium,"tile 779 (STADIUMBASE) is the stadium");
    check(overviewClassifyTile(811)==OverviewZone::PowerPlant,"tile 811 (NUCLEARBASE) is a power plant");
    check(overviewClassifyTile(844)==OverviewZone::Animation,"tile 844 (INDBASE2/TELEBASE) is an animation frame, not industry");
    check(overviewClassifyTile(932)==OverviewZone::Animation,"tile 932 (FOOTBALLGAME1) is an animation frame");
    check(overviewClassifyTile(1023)==OverviewZone::Animation,"tile 1023 is still in the animation bucket");

    // Exact ports of drawRes/drawCom/drawInd/drawLilTransMap (g_smmaps.c),
    // tile-range for tile-range, independent of the OverviewZone bucket
    // table above.
    check(overviewIsResidential(0)==true,"drawRes keeps dirt (tile<=422 stays)");
    check(overviewIsResidential(422)==true,"drawRes keeps tile 422");
    check(overviewIsResidential(423)==false,"drawRes hides tile 423 (first commercial tile)");
    check(overviewIsCommercial(231)==true,"drawCom keeps tile 231 (below its 232 cutoff)");
    check(overviewIsCommercial(232)==false,"drawCom hides tile 232 (>=232 and <423)");
    check(overviewIsCommercial(422)==false,"drawCom hides tile 422 (still <423)");
    check(overviewIsCommercial(423)==true,"drawCom keeps tile 423 (first commercial tile)");
    check(overviewIsCommercial(609)==true,"drawCom keeps tile 609 (COMLAST)");
    check(overviewIsCommercial(610)==false,"drawCom hides tile 610 (>609)");
    check(overviewIsIndustrial(612)==true,"drawInd keeps tile 612 (industrial)");
    check(overviewIsIndustrial(692)==true,"drawInd keeps tile 692 (industrial)");
    check(overviewIsIndustrial(693)==false,"drawInd hides tile 693 (port, in its 693..851 cutout)");
    check(overviewIsIndustrial(851)==false,"drawInd hides tile 851 (last tile of the 693..851 cutout)");
    check(overviewIsIndustrial(852)==true,"drawInd keeps tile 852 (just past the cutout)");
    check(overviewIsIndustrial(859)==true,"drawInd keeps tile 859 (just before the next cutout)");
    check(overviewIsIndustrial(860)==false,"drawInd hides tile 860 (explosion animation cutout starts)");
    check(overviewIsIndustrial(883)==false,"drawInd hides tile 883 (last tile of the explosion cutout)");
    check(overviewIsIndustrial(884)==true,"drawInd keeps tile 884 (past the explosion cutout, below 932)");
    check(overviewIsIndustrial(931)==true,"drawInd keeps tile 931 (just before the final >=932 cutoff)");
    check(overviewIsIndustrial(932)==false,"drawInd hides tile 932 (>=932 cutoff)");
    check(overviewIsTransportation(206)==true,"drawLilTransMap keeps tile 206 (last road tile)");
    check(overviewIsTransportation(207)==false,"drawLilTransMap hides tile 207 (207..220 cutout)");
    check(overviewIsTransportation(220)==false,"drawLilTransMap hides tile 220 (last of the 207..220 cutout)");
    check(overviewIsTransportation(221)==true,"drawLilTransMap keeps tile 221 (just past the cutout)");
    check(overviewIsTransportation(222)==true,"drawLilTransMap keeps tile 222");
    check(overviewIsTransportation(223)==false,"drawLilTransMap hides tile 223 (the lone ==223 cutout)");
    check(overviewIsTransportation(224)==true,"drawLilTransMap keeps tile 224 (HRAIL, below the 240 cutoff)");
    check(overviewIsTransportation(239)==true,"drawLilTransMap keeps tile 239 (last rail tile)");
    check(overviewIsTransportation(240)==false,"drawLilTransMap hides tile 240 (>=240 cutoff)");

    // GetCI's exact 50/100/150/200 boundaries (g_map.c:105-112).
    check(overviewIntensityLevel(49)==OverviewIntensityLevel::None,"GetCI: 49 is None");
    check(overviewIntensityLevel(50)==OverviewIntensityLevel::Low,"GetCI: 50 is Low");
    check(overviewIntensityLevel(99)==OverviewIntensityLevel::Low,"GetCI: 99 is still Low");
    check(overviewIntensityLevel(100)==OverviewIntensityLevel::Medium,"GetCI: 100 is Medium");
    check(overviewIntensityLevel(149)==OverviewIntensityLevel::Medium,"GetCI: 149 is still Medium");
    check(overviewIntensityLevel(150)==OverviewIntensityLevel::High,"GetCI: 150 is High");
    check(overviewIntensityLevel(199)==OverviewIntensityLevel::High,"GetCI: 199 is still High");
    check(overviewIntensityLevel(200)==OverviewIntensityLevel::VeryHigh,"GetCI: 200 is VeryHigh");

    // Pollution runs through GetCI(10 + value) (g_map.c:180), so its bands
    // sit 10 lower than the raw GetCI thresholds.
    check(overviewPollutionLevel(39)==OverviewIntensityLevel::None,"pollution: 39+10=49 is None");
    check(overviewPollutionLevel(40)==OverviewIntensityLevel::Low,"pollution: 40+10=50 is Low");
    check(overviewPollutionLevel(90)==OverviewIntensityLevel::Medium,"pollution: 90+10=100 is Medium");
    check(overviewPollutionLevel(140)==OverviewIntensityLevel::High,"pollution: 140+10=150 is High");
    check(overviewPollutionLevel(190)==OverviewIntensityLevel::VeryHigh,"pollution: 190+10=200 is VeryHigh");

    // Rate-of-growth's exact else-if chain (g_map.c:136-148): `z>100` is
    // checked first, then (independently) `z>20`, so a value like 100 or
    // -100 that fails its own strict boundary still falls through to the
    // next, looser test rather than landing on Neutral — verified by
    // reading the chain, not assumed to be symmetric around zero.
    check(overviewGrowthLevel(-150)==OverviewGrowthLevel::VeryMinus,"growth: <-100 is VeryMinus");
    check(overviewGrowthLevel(-100)==OverviewGrowthLevel::Minus,"growth: -100 fails <-100 but still satisfies <-20, so Minus");
    check(overviewGrowthLevel(-99)==OverviewGrowthLevel::Minus,"growth: -99 satisfies <-20, so Minus");
    check(overviewGrowthLevel(-21)==OverviewGrowthLevel::Minus,"growth: -21 is Minus");
    check(overviewGrowthLevel(-20)==OverviewGrowthLevel::Neutral,"growth: -20 itself is Neutral");
    check(overviewGrowthLevel(0)==OverviewGrowthLevel::Neutral,"growth: 0 is Neutral");
    check(overviewGrowthLevel(20)==OverviewGrowthLevel::Neutral,"growth: 20 itself is Neutral");
    check(overviewGrowthLevel(21)==OverviewGrowthLevel::Plus,"growth: 21 is Plus");
    check(overviewGrowthLevel(100)==OverviewGrowthLevel::Plus,"growth: 100 fails >100 but still satisfies >20, so Plus");
    check(overviewGrowthLevel(101)==OverviewGrowthLevel::VeryPlus,"growth: 101 is VeryPlus");

    // Power overlay states (drawPower, g_smmaps.c:233-362): background
    // passthrough below tile 64, then ZONEBIT->powered/unpowered,
    // else CONDBIT->conductive, else background. These take the *raw*
    // (bit-including) tile value, not the LOMASK'd one.
    check(overviewPowerState(0)==OverviewPowerState::Background,"power: dirt is background");
    check(overviewPowerState(63)==OverviewPowerState::Background,"power: tile 63 (below the 64 floor) is background");
    check(overviewPowerState(64)==OverviewPowerState::Background,"power: a plain road tile (no CONDBIT/ZONEBIT) is background");
    check(overviewPowerState(64 | 0x4000)==OverviewPowerState::Conductive,"power: CONDBIT set makes it conductive");
    check(overviewPowerState(240 | 0x0400 | 0x8000)==OverviewPowerState::Powered,"power: ZONEBIT+PWRBIT is a powered zone centre");
    check(overviewPowerState(240 | 0x0400)==OverviewPowerState::Unpowered,"power: ZONEBIT without PWRBIT is unpowered");

    // Single-zone filters dim everything else; the matching zone keeps colour.
    check(overviewColorForSample(OverviewLayer::Residential,240)==overviewZoneColor(OverviewZone::Residential),
          "residential filter lights up a residential tile");
    check(overviewColorForSample(OverviewLayer::Residential,423)==overviewDimColor,
          "residential filter dims a commercial tile");
    check(overviewColorForSample(OverviewLayer::Transportation,64)==overviewZoneColor(OverviewZone::Road),
          "transportation filter lights up road");
    check(overviewColorForSample(OverviewLayer::Transportation,224)==overviewZoneColor(OverviewZone::Rail),
          "transportation filter also lights up rail");
    check(overviewColorForSample(OverviewLayer::Transportation,0)==overviewDimColor,
          "transportation filter dims dirt");

    // PowerGrid dispatches through overviewPowerState on the raw tile, not
    // a plain 0/nonzero bool (drawPower has three real states, not two).
    check(overviewColorForSample(OverviewLayer::PowerGrid,240|0x0400|0x8000)==overviewPowerPoweredColor,
          "powered zone centre gets the powered colour");
    check(overviewColorForSample(OverviewLayer::PowerGrid,240|0x0400)==overviewPowerUnpoweredColor,
          "unpowered zone centre gets the unpowered colour");
    check(overviewColorForSample(OverviewLayer::PowerGrid,64|0x4000)==overviewPowerConductiveColor,
          "a conductive non-zone tile gets the conductive colour");
    check(overviewColorForSample(OverviewLayer::PowerGrid,0)==overviewDimColor,
          "plain background is dim, not one of the three power colours");

    // Diverging growth colour: sign distinguishes growth from decline, zero
    // is a third, distinct value (the baseline).
    uint32_t up=overviewGrowthColor(500), down=overviewGrowthColor(-500), zero=overviewGrowthColor(0);
    check(up!=down,"positive and negative growth render differently");
    check(zero!=up && zero!=down,"zero growth is visually distinct from both signs");
    check(overviewGrowthColor(5000)==overviewGrowthColor(1000),"extreme growth clamps rather than overflowing colour math");
    check(overviewGrowthColor(-5000)==overviewGrowthColor(-1000),"extreme decline clamps symmetrically");

    // Intensity ramp: monotonic and bounded.
    int prevLevel=-1;bool monotonic=true;
    for(int v=0;v<=255;v+=17) {
        uint32_t c=overviewIntensityColor(v);
        int level=(int)((c>>16)&0xff)+(int)((c>>8)&0xff); // rough brightness proxy
        if(level<prevLevel)monotonic=false;
        prevLevel=level;
    }
    check(monotonic,"intensity ramp brightness proxy is nondecreasing across the sampled range");
    check(overviewIntensityColor(-10)==overviewIntensityColor(0),"negative intensity clamps to the floor");
    check(overviewIntensityColor(9999)==overviewIntensityColor(255),"intensity above 255 clamps to the ceiling");

    // Camera navigation: center-on-click with edge clamping, matching
    // ViewGeometry::clampCamera's bounds without depending on it directly.
    int camX,camY;
    OverviewCamera::panTo(60,50,1,20,15,camX,camY); // dead center of a 120x100 world at scale 1
    check(camX==60-10 && camY==50-7,"pan centers the viewport on the clicked world tile");
    OverviewCamera::panTo(0,0,1,20,15,camX,camY);
    check(camX==0 && camY==0,"pan near the top-left clamps to zero, not negative");
    OverviewCamera::panTo(119,99,1,20,15,camX,camY);
    check(camX==overviewWorldW-20 && camY==overviewWorldH-15,"pan near the bottom-right clamps to the world edge");
    OverviewCamera::panTo(357,297,3,20,15,camX,camY); // scale-3 pixel coordinates, wmap.tcl's own scale
    check(camX==overviewWorldW-20 && camY==overviewWorldH-15,"scale-3 click coordinates are divided before clamping");

    int px,py,pw,ph;
    OverviewCamera::rect(10,10,20,15,3,px,py,pw,ph);
    check(px==30 && py==30 && pw==60 && ph==45,"camera rectangle scales position and size together");
    OverviewCamera::rect(0,0,overviewWorldW+50,overviewWorldH+50,3,px,py,pw,ph);
    check(pw==overviewWorldW*3 && ph==overviewWorldH*3,"an oversized viewport clamps the rectangle to the world, not beyond it");

    printf("RESULT: %d/%d passed\n",total-failed,total);
    return failed?1:0;
}

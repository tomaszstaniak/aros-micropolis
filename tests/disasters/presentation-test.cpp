#include "disaster-presentation.h"
#include <cstdio>
#include <climits>
#include <cstdlib>
int main() {
    int total=0,failed=0;
    auto check=[&](bool ok,const char *name){++total;failed+=!ok;printf("%s %s\n",ok?"PASS":"FAIL",name);};
    EarthquakePresentation quake;
    check(!quake.active() && quake.x()==0 && quake.y()==0 && !quake.tick(),"idle quake leaves display origin untouched");
    const int strengths[]={INT_MIN,300,301,650,1000,INT_MAX};
    const int expectedTicks[]={3,3,4,7,10,10};
    for(int sample=0;sample<6;++sample) {
        int strength=strengths[sample];
        quake.start(strength); int ticks=0; bool bounded=true,changed=false,displaced=false;
        while(quake.active() && ticks<11) {
            bounded &= std::abs(quake.x())<=3 && std::abs(quake.y())<=3;
            displaced |= quake.x()!=0 || quake.y()!=0;
            changed |= quake.tick(); ++ticks;
        }
        check(ticks==expectedTicks[sample],"strength maps to bounded 100ms presentation steps");
        check(bounded && changed && displaced,"quake produces bounded visible displacement");
        check(!quake.active() && quake.x()==0 && quake.y()==0 && !quake.tick(),"quake finishes at exact zero without later redraws");
    }
    quake.start(1000); quake.tick(); quake.start(300);
    for(int i=0;i<3;++i) quake.tick();
    check(!quake.active(),"new quake replaces prior duration");
    quake.start(1000); quake.clear();
    check(!quake.active() && quake.x()==0 && quake.y()==0,"city switch clears quake displacement");
    ScenarioPresentation result;
    check(result.take()==ScenarioPresentation::None && !result.message(MESSAGE_FIRE_REPORTED),"ordinary messages do not create scenario results");
    check(result.message(MESSAGE_SCENARIO_WON),"victory message queues result without win callback");
    check(!result.message(MESSAGE_SCENARIO_WON) && !result.message(MESSAGE_SCENARIO_LOST),"first result stays stable before consumption");
    check(result.take()==ScenarioPresentation::Won && result.take()==ScenarioPresentation::None,"main loop consumes victory exactly once");
    check(!result.message(MESSAGE_SCENARIO_LOST),"completed scenario result cannot reopen dialog");
    result.reset();
    check(result.message(MESSAGE_SCENARIO_LOST) && result.take()==ScenarioPresentation::Lost,"reset permits next city's loss result");
    result.reset(); result.message(MESSAGE_SCENARIO_WON); result.reset();
    check(result.take()==ScenarioPresentation::None,"city switch discards stale pending result");
    const uint32_t original[]={1,2,3,4,5,6};
    std::vector<uint32_t> scratch;
    auto shiftCheck=[&](int dx,int dy,const std::vector<uint32_t> &expected,const char *name) {
        std::vector<uint32_t> frame(original,original+6);
        shiftEarthquakeFrame(frame.data(),3,2,dx,dy,scratch);
        check(frame==expected,name);
    };
    const uint32_t black=0xff000000u;
    shiftCheck(1,1,{black,black,black,black,1,2},"positive quake displacement preserves original source pixels");
    shiftCheck(-1,-1,{5,6,black,black,black,black},"negative quake displacement preserves original source pixels");
    shiftCheck(0,0,{1,2,3,4,5,6},"zero quake displacement preserves framebuffer");
    shiftCheck(1,0,{black,1,2,black,4,5},"horizontal quake does not bleed across rows");
    shiftCheck(0,-1,{4,5,6,black,black,black},"vertical quake fills exposed last row");
    shiftCheck(INT_MIN,INT_MAX,{black,black,black,black,black,black},"extreme offsets remain in bounds");
    uint32_t guard=42;
    shiftEarthquakeFrame(&guard,0,2,1,1,scratch);
    shiftEarthquakeFrame(&guard,3,0,1,1,scratch);
    shiftEarthquakeFrame(nullptr,0,0,1,1,scratch);
    check(guard==42,"zero dimensions do not read or modify framebuffer");
    printf("RESULT presentation: %d/%d passed\n",total-failed,total); return failed?1:0;
}

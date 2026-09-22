#include "demand-model.h"
#include <cstdio>

int main() {
    int total=0,failed=0;
    auto check=[&](bool ok,const char *why){++total;failed+=!ok;printf("%s %s\n",ok?"PASS":"FAIL",why);};

    DemandModel m;
    check(!m.has && m.residential==0 && m.commercial==0 && m.industrial==0,
          "no callback yet: values are zero, not stale/garbage");

    m.update(300.0f,-200.0f,0.0f);
    check(m.has,"first callback marks the indicator live");
    check(m.residential==300.0f && m.commercial==-200.0f && m.industrial==0.0f,
          "stores the three signed values independently");

    m.update(-1500.0f,1500.0f,750.0f);
    check(m.residential==-1500.0f && m.commercial==1500.0f && m.industrial==750.0f,
          "engine's own clamp bounds pass through unchanged");

    m.reset();
    check(!m.has && m.residential==0 && m.commercial==0 && m.industrial==0,
          "reset (load/new game) clears both the values and the has-data flag");

    // Bar height: signed, zero baseline, symmetric, clamped to the engine's
    // own valve range so a caller can pass raw r/c/i straight through.
    check(demandBarHeight(0,16)==0,"zero valve sits exactly on the baseline");
    check(demandBarHeight(1500,16)==16,"max positive valve reaches full bar height");
    check(demandBarHeight(-1500,16)==-16,"max negative valve reaches full bar height, negative sign");
    check(demandBarHeight(750,16)==8,"half demand is half height");
    check(demandBarHeight(-750,16)==-8,"half negative demand is half height, negative sign");
    check(demandBarHeight(3000,16)==16,"above-range demand clamps rather than overflowing the bar");
    check(demandBarHeight(-3000,16)==-16,"below-range demand clamps symmetrically");
    check(demandBarHeight(300,0)==0,"a zero-height bar (window not yet sized) never divides by zero or goes negative height");
    check(demandBarHeight(-1,-5)==0,"a negative max height is treated as zero, not inverted");

    printf("RESULT: %d/%d passed\n",total-failed,total);
    return failed?1:0;
}

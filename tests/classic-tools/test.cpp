#include "classic-tools.h"
#include <cstdio>
#include <cmath>
int main() {
    int count=0,failed=0;
    auto check=[&](bool ok,const char *why){++count;failed+=!ok;printf("%s %s\n",ok?"PASS":"FAIL",why);};
    check(classicToolAt(10,59)==0,"residential origin belongs to original tall icon");
    check(classicToolAt(15,105)==0,"lower part of tall residential icon selectable");
    check(classicToolAt(2,150)==-1,"space left of centered wire is not a tool");
    check(classicToolAt(29,151)==6,"wire uses centered original position");
    check(classicToolAt(120,205)==9,"right end of wide road selectable");
    check(classicToolAt(36,347)==17,"airport origin");
    check(classicToolAt(92,403)==17,"airport bottom-right pixel");
    check(classicToolAt(95,406)==-1,"outside airport is not a tool");
    for(int i=0;i<18;i++) {
        const auto &r=classicTools[i];
        check(classicToolAt(r.x+r.w/2,r.y+r.h/2)==i,"every icon center selects itself");
    }
    ClassicPie p;
    check(p.pick(80,0)==0,"root east = Road");
    check(p.pick(0,-80)==2,"root north = Zone submenu");
    check(p.pick(0,80)==6,"root south = Build submenu");
    check(p.pick(1,1)==-1,"center is inactive");
    check(p.release(0,0)==ClassicPie::pending && !p.held,"initial center release latches menu");
    p.press();check(p.release(0,0)==ClassicPie::cancel,"second center release cancels");
    p=ClassicPie();check(p.release(90,0)==9,"east release selects road palette slot");
    p=ClassicPie();check(p.release(0,-90)==ClassicPie::submenu && p.page==1 && !p.held,"zone opens on release and waits for next press");
    check(p.pick(0,90)==0,"zone starts south: Query");
    check(p.release(0,90)==ClassicPie::pending,"unpaired submenu release cannot choose");
    p.press();check(p.release(0,90)==4,"zone Query maps to palette slot, not engine enum");
    p=ClassicPie();check(p.release(0,90)==ClassicPie::submenu && p.page==2,"build submenu");
    p.press();check(p.release(0,90)==17,"build south = Airport");
    for(int page=0;page<3;page++) {
        p=ClassicPie();p.page=page;
        for(int i=0;i<p.size();i++) {
            double a=p.angle(i);int x=std::lround(90*std::cos(a)),y=std::lround(-90*std::sin(a));
            check(p.pick(x,y)==i,"rendered direction agrees with selected sector");
        }
    }
    printf("RESULT: %d/%d passed\n",count-failed,count);return failed?1:0;
}

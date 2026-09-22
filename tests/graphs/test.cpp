#include "graph-model.h"
#include <cstdio>
#include <limits>
struct History {
    GraphHistory values{};
    int calls=0,scale=-1;
    short getHistory(int type,int range,int index) {
        ++calls;scale=range;return values[type][index];
    }
};
int main() {
    int total=0,failed=0;
    auto check=[&](bool ok,const char *why){++total;failed+=!ok;printf("%s %s\n",ok?"PASS":"FAIL",why);};
    History city;GraphModel m;
    check(m.mask==63 && m.scale==0,"classic default enables six monthly histories");
    check(m.update(city) && city.calls==720,"snapshot reads each monthly sample exactly once");
    check(!m.update(city),"unchanged history does not redraw");
    city.values[0][0]=100;city.values[0][119]=20;
    m.update(city);
    check(m.value(0,0)==20 && m.value(0,119)==100,"oldest left and newest right");
    city.values[0][0]=256;city.values[1][0]=128;city.values[2][0]=64;
    m.update(city);
    check(m.value(0,119)==128 && m.value(1,119)==64 && m.value(2,119)==32,"RCI share classic scale above 128");
    city.values[3][0]=128;city.values[4][0]=255;city.values[5][0]=-20;
    m.update(city);
    check(m.value(3,119)==128 && m.value(4,119)==255 && m.value(5,119)==0,"cash crime pollution are indices clamped independently of RCI");
    m.toggle(0);check(m.mask==62 && m.value(1,119)==64,"hiding a population series does not change other scales");
    for(int i=1;i<6;i++)m.toggle(i);
    check(m.mask==0,"all series can be hidden");
    m.toggle(-1);m.toggle(6);check(m.mask==0,"invalid tool indices do not alter mask");
    m.setScale(1);check(m.update(city) && city.scale==1,"120-year switch reads annual bank");
    m.setScale(2);check(m.scale==1,"invalid range ignored");
    check(!m.update(city),"unchanged annual snapshot stable");
    city.values={};city.values[2][119]=32767;city.values[2][0]=-32768;
    check(m.update(city) && m.value(2,0)==128 && m.value(2,119)==0,"range uses oldest sample too and clamps negative extremes");
    city.values={};m.update(city);
    check(m.value(0,0)==0 && m.value(0,119)==0,"empty or newly loaded history is safe");
    for(int v=0;v<256;v++)check(graphY(v,180)>=0 && graphY(v,180)<180,"plotted index stays inside graph");
    check(graphY(0,180)==179 && graphY(255,180)==0,"graph endpoints are inclusive");
    printf("RESULT: %d/%d passed\n",total-failed,total);return failed?1:0;
}

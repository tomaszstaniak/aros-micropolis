#ifndef MICROPOLIS_GRAPH_MODEL_H
#define MICROPOLIS_GRAPH_MODEL_H
#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>
using GraphHistory=std::array<std::array<short,120>,6>;
// Classic w_graph.c colors and shared R/C/I scaling. Money is encoded cash
// flow (128 = zero), not a treasury balance or a dollar-valued axis.
inline constexpr uint32_t graphColors[]={0xff00e600,0xff0000e6,0xffffff00,
                                       0xff007f00,0xffff0000,0xff997f4c};
inline constexpr const char *graphNames[]={"Residential","Commercial","Industrial",
                                         "Cash flow","Crime","Pollution"};
inline int graphY(int value,int height) {
    return (height-1)-(std::clamp(value,0,255)*(height-1)/255);
}
// Classic DoUpdateGraph (w_graph.c) calendar ticks: every January over ten
// years, every decade start over 120 years. x is a plot column 0..plotW-1.
struct GraphTick { int x; int year; };
inline std::vector<GraphTick> graphTicks(int scale,long cityTime,int plotW) {
    std::vector<GraphTick> ticks;
    int year=1900+(int)(cityTime/48);
    const int month=(int)((cityTime/4)%12);
    if(scale==0) {
        for(int x=120-month;x>=0;x-=12) ticks.push_back({x*(plotW-1)/120,year--});
    } else {
        int decade=year/10;
        for(int x=1200-10*(year%10);x>=0;x-=120) ticks.push_back({x*(plotW-1)/1200,10*decade--});
    }
    return ticks;
}
struct GraphModel {
    unsigned mask=63;
    int scale=0;
    GraphHistory history{};
    bool fresh=false;
    int populationMax=0;
    void toggle(int series){if(series>=0 && series<6)mask^=1u<<series;}
    void setScale(int range){if(range>=0 && range<2 && scale!=range){scale=range;fresh=false;}}
    template<class City> bool update(City &city) {
        GraphHistory next{};
        for(int s=0;s<6;s++)for(int i=0;i<120;i++)next[s][i]=city.getHistory(s,scale,i);
        if(fresh && next==history)return false;
        history=next;fresh=true;populationMax=0;
        // Include all displayed samples, including the oldest one. Computing
        // maxima here avoids mutating the city via legacy initGraphMax().
        for(int s=0;s<3;s++)for(int v:history[s])populationMax=std::max(populationMax,v);
        return true;
    }
    int value(int series,int x) const {
        if(series<0 || series>=6 || x<0 || x>=120)return 0;
        int v=history[series][119-x];
        if(series<3 && populationMax>128)v=v*128/populationMax;
        return std::clamp(v,0,255);
    }
};
#endif

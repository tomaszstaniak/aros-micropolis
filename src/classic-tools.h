#ifndef MICROPOLIS_CLASSIC_TOOLS_H
#define MICROPOLIS_CLASSIC_TOOLS_H
#include <cmath>

// Source: micropolis-activity/res/weditor.tcl at c98f6b08519887b450d9be198bfca5237aab6d0c.
// Coordinates are widget origins; w/h are the original XPM sizes (1px widget border).
struct ClassicTool { const char *image; int x,y,w,h; };
inline constexpr ClassicTool classicTools[] = {
    {"res",9,58,34,50},{"com",47,58,34,50},{"ind",85,58,34,50},
    {"fire",9,112,34,34},{"qry",47,112,34,34},{"pol",85,112,34,34},
    {"wire",28,150,34,34},{"dozr",66,150,34,34},
    {"rail",6,188,56,24},{"road",66,188,56,24},
    {"chlk",28,216,34,34},{"ersr",66,216,34,34},
    {"stad",1,254,42,42},{"park",47,254,34,34},{"seap",85,254,42,42},
    {"coal",1,300,42,42},{"nuc",85,300,42,42},{"airp",35,346,58,58}
};
inline int classicToolAt(int x,int y) {
    for(int i=0;i<18;i++) {
        const auto &r=classicTools[i];
        if(x>=r.x && y>=r.y && x<r.x+r.w+2 && y<r.y+r.h+2)return i;
    }
    return -1;
}

// Tcl PieMenuDown/Up: first center-release latches; second center-release cancels.
// Submenus post at the release position and require another press/release.
struct ClassicPie {
    static constexpr int pending=-1,cancel=-2,submenu=-3;
    static constexpr double pi=3.14159265358979323846;
    int page=0;
    bool held=true,first=true;
    int size() const {return page==0?8:6;}
    double angle(int i) const {return (page==0?0:1.5*pi)+i*(2*pi/size());}
    const char *title() const {return page==0?"Tool":page==1?"Zone":"Build";}
    int slot(int i) const {
        // -1 = Zone, -2 = Build. Values otherwise index the editor palette.
        static const int root[]={9,7,-1,6,8,10,-2,11};
        static const int zone[]={4,5,2,1,0,3};
        static const int build[]={17,16,14,13,12,15};
        return i<0 || i>=size()?-99:(page==0?root[i]:page==1?zone[i]:build[i]);
    }
    int pick(int dx,int dy) const {
        if(double(dx)*dx+double(dy)*dy<12*12)return -1;
        double a=std::atan2(-double(dy),double(dx))-angle(0);
        a=std::fmod(a+4*pi,2*pi);
        return int(std::floor(a/(2*pi/size())+0.5))%size();
    }
    void press() {held=true;}
    int release(int dx,int dy) {
        if(!held)return pending;
        held=false;int i=pick(dx,dy);
        if(i<0) {if(first){first=false;return pending;}return cancel;}
        int selected=slot(i);
        if(selected<0) {page=-selected;first=false;return submenu;}
        return selected;
    }
};
#endif

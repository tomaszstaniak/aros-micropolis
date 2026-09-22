#include "../../src/view-geometry.h"
#include <cstdio>
int main() {
    int errors=0;
    auto check=[&](bool ok,const char*n){printf("%s %s\n",ok?"PASS":"FAIL",n);errors+=!ok;};
    auto v=ViewGeometry::fromInner(647,455);
    check(v.width==640 && v.height==448, "partial edge tile excluded from rendering and hit testing");
    check(v.contains(639,447) && !v.contains(640,447) && !v.contains(-1,0), "map hit bounds exclude borders");
    int x=100,y=90; v.clampCamera(x,y);
    check(x==80 && y==72, "camera clamped at original viewport size");
    auto big=ViewGeometry::fromInner(1024,768);big.clampCamera(x,y);
    check(x==56 && y==52, "enlarging window clamps camera to new map extent");
    auto whole=ViewGeometry::fromInner(3000,2000);whole.clampCamera(x,y);
    check(whole.width==1920 && whole.height==1600 && x==0 && y==0, "oversized display never pans outside the world");
    check(big.mapX(160,12)==22 && big.mapY(32,15)==17, "resized view uses unchanged 16px coordinates");
    printf("RESULT: %d/6 passed\n",6-errors);return errors?1:0;
}

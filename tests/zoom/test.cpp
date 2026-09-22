#include "view-geometry.h"
#include <cstdio>
#include <vector>
static int n=0,failed=0;
static void check(bool ok,const char *s){++n;failed+=!ok;printf("%s %s\n",ok?"PASS":"FAIL",s);}
int main(){
 auto d=ViewGeometry::fromInner(647,455);
 check(d.tile==16 && d.width==640 && d.height==448,"default tile 16 keeps previous snapping");
 check(ViewGeometry::fromInner(3000,2000).width==1920 && ViewGeometry::fromInner(3000,2000).height==1600,"16 px limit is whole world");
 check(ViewGeometry::fromInner(5,5).width==16 && ViewGeometry::fromInner(5,5).height==16,"16 px minimum is one tile");
 auto z8=ViewGeometry::fromInner(647,455,8);
 check(z8.width==640 && z8.height==448 && z8.columns()==80 && z8.rows()==56,"8 px snaps to 8 and doubles columns");
 check(z8.worldWidth()==1280 && z8.worldHeight()==896,"8 px world frame is twice the screen");
 auto z8big=ViewGeometry::fromInner(3000,2000,8);
 check(z8big.width==960 && z8big.height==800 && z8big.columns()==120 && z8big.rows()==100,"8 px limit is 120x100 tiles");
 check(ViewGeometry::fromInner(3,3,8).width==8,"8 px minimum is one tile");
 auto z32=ViewGeometry::fromInner(647,455,32);
 check(z32.width==640 && z32.height==448 && z32.columns()==20 && z32.rows()==14,"32 px snaps to 32");
 check(z32.worldWidth()==320 && z32.worldHeight()==224,"32 px world frame is half the screen");
 auto z32big=ViewGeometry::fromInner(10000,10000,32);
 check(z32big.width==3840 && z32big.height==3200,"32 px limit is 120*32 x 100*32");
 check(ViewGeometry::fromInner(20,20,32).width==32,"32 px minimum is one tile");
 for(int t:{8,16,32}){auto v=ViewGeometry::fromInner(1000,700,t);
  check(v.worldWidth()*t/16==v.width && v.worldHeight()*t/16==v.height,"screen buffer equals world*tile/16");}
 check(z8.mapX(15,10)==11 && z8.mapY(16,3)==5,"8 px mapX/mapY");
 check(z32.mapX(31,10)==10 && z32.mapX(32,10)==11 && z32.mapY(95,0)==2,"32 px mapX/mapY");
 check(d.mapX(160,12)==22,"16 px mapX unchanged");
 check(z8.worldX(8)==16 && z8.worldY(3)==6 && z32.worldX(32)==16 && z32.worldY(31)==15 && d.worldX(17)==17,"worldX/worldY convert screen to 16 px frame");
 int x=200,y=200;z8.clampCamera(x,y);check(x==40 && y==44,"8 px clamp uses its column count");
 x=200;y=200;z8big.clampCamera(x,y);check(x==0 && y==0,"whole-world 8 px view clamps to origin");
 x=200;y=200;z32.clampCamera(x,y);check(x==100 && y==86,"32 px clamp");
 x=-5;y=-5;z32.clampCamera(x,y);check(x==0 && y==0,"negative camera clamps to zero");
 check(zoomTileStep(16,1)==32 && zoomTileStep(32,1)==32 && zoomTileStep(8,1)==16,"zoom in steps to 32 max");
 check(zoomTileStep(16,-1)==8 && zoomTileStep(8,-1)==8 && zoomTileStep(32,-1)==16,"zoom out steps to 8 min");
 check(zoomTileStep(8,0)==16 && zoomTileStep(32,0)==16,"zero direction resets to 16");
 check(zoomPercent(8)==50 && zoomPercent(16)==100 && zoomPercent(32)==200,"zoom percent");
 {std::vector<uint32_t> w={1,2,3,4,5,6},s(24,0);scaleWorldFrame(w.data(),3,2,s.data(),32);
  bool ok=true;for(int y=0;y<4;++y)for(int x=0;x<6;++x)ok&=s[y*6+x]==w[(y/2)*3+x/2];
  check(ok,"32 px duplicates each pixel into 2x2");}
 {std::vector<uint32_t> w={0xff000000,0xff0000ff,0xffffffff,0xff00ff00, 0xff0000ff,0xff0000ff,0x00ffffff,0xff010203},s(2,0);
  scaleWorldFrame(w.data(),4,2,s.data(),8);
  // block0 blue channel (0+255+255+255+2)/4=191; block1: A (255+255+0+255+2)/4=191, R (255+0+255+1+2)/4=128, G (255+255+255+2+2)/4=192, B (255+0+255+3+2)/4=128
  check(s[0]==0xff0000bf,"8 px averages 2x2 per channel with rounding");
  check(s[1]==0xbf80c080,"8 px averages alpha and every channel independently");}
 {std::vector<uint32_t> w={7,8,9,10},s(4,0);scaleWorldFrame(w.data(),2,2,s.data(),16);check(s==w,"16 px copies frame");}
 {auto v=ViewGeometry::fromInner(333,211,8);std::vector<uint32_t> w((size_t)v.worldWidth()*v.worldHeight(),0x11223344),s((size_t)v.width*v.height,0);
  scaleWorldFrame(w.data(),v.worldWidth(),v.worldHeight(),s.data(),8);bool ok=true;for(auto p:s)ok&=p==0x11223344;check(ok,"8 px fills exactly the screen buffer (ASan bounds)");}
 {auto v=ViewGeometry::fromInner(333,211,32);std::vector<uint32_t> w((size_t)v.worldWidth()*v.worldHeight(),5),s((size_t)v.width*v.height,0);
  scaleWorldFrame(w.data(),v.worldWidth(),v.worldHeight(),s.data(),32);bool ok=true;for(auto p:s)ok&=p==5;check(ok,"32 px fills exactly the screen buffer (ASan bounds)");}
 printf("RESULT: %d/%d passed\n",n-failed,n);return failed?1:0;
}

#include "chalk-overlay.h"
#include <cstdio>
#include <vector>
static int n=0,failed=0;
static void check(bool ok,const char *s){++n;failed+=!ok;printf("%s %s\n",ok?"PASS":"FAIL",s);}
static int count(const std::vector<uint32_t> &f,uint32_t c){int k=0;for(auto p:f)k+=p==c;return k;}
int main(){
 ChalkOverlay o;
 check(o.strokes().empty() && !o.drawing() && o.revision()==0,"new overlay is empty");
 o.start(100,50);
 check(o.drawing() && o.strokes().size()==1 && o.strokes()[0].length()==1,"start opens one-point stroke");
 auto r0=o.revision();
 o.extend(100,50);
 check(o.strokes()[0].length()==1 && o.revision()==r0,"duplicate point is skipped");
 o.extend(120,40);o.extend(90,70);
 const auto &s=o.strokes()[0];
 check(s.length()==3 && s.left==90 && s.right==120 && s.top==40 && s.bottom==70,"bounding box covers all points");
 check(o.revision()>r0,"adding points changes revision");
 o.finish();o.extend(0,0);
 check(!o.drawing() && o.strokes()[0].length()==3,"extend after finish is ignored");
 // eraser semantics
 ChalkOverlay e;
 e.start(10,10);e.finish();                 // single point
 e.start(200,200);e.extend(300,200);e.finish(); // horizontal segment
 e.start(1000,1000);e.extend(1010,1010);e.finish(); // far
 auto rv=e.revision();
 check(!e.eraseAt(500,500) && e.strokes().size()==3 && e.revision()==rv,"miss erases nothing, returns false, revision unchanged");
 check(e.eraseAt(18,18) && e.strokes().size()==2,"single point inside +-8 box (corner) is erased");
 check(e.revision()!=rv,"erase changes revision");
 check(!e.eraseAt(250,209) && e.strokes().size()==2,"segment 9px away is kept");
 check(e.eraseAt(250,208) && e.strokes().size()==1 && e.strokes()[0].left==1000,"segment within 8px erased as whole stroke; far stroke kept");
 ChalkOverlay p;p.start(5,5);check(p.eraseAt(0,0) && !p.drawing(),"erasing stroke in progress stops drawing");
 // multi-segment: bbox hit but no segment near (L shape)
 ChalkStroke L;L.points={0,0,100,0,100,100};L.left=0;L.top=0;L.right=100;L.bottom=100;
 check(!ChalkOverlay::inBox(L,40,40,60,60),"bbox inside stroke bbox but away from segments misses (segment bbox rule)");
 check(ChalkOverlay::inBox(L,92,40,108,60),"box over vertical segment hits");
 ChalkStroke diag;diag.points={0,0,100,100};diag.left=diag.top=0;diag.right=diag.bottom=100;
 check(ChalkOverlay::inBox(diag,80,0,96,16),"diagonal segment uses its bounding box, as classic InkInBox");
 o.clear();check(o.strokes().empty() && !o.drawing(),"clear removes all");
 auto rc=o.revision();o.clear();check(o.revision()==rc,"clearing empty overlay keeps revision");
 // drawing
 const int W=40,H=30;const uint32_t C=0xffffffff;
 {ChalkOverlay d;d.start(20,15);d.finish();std::vector<uint32_t> f(W*H,0);drawChalk(d,f.data(),W,H,0,0);
  check(count(f,C)==49 && f[12*W+17]==C && f[18*W+23]==C && f[11*W+20]==0,"single click draws 7x7 dot");}
 {ChalkOverlay d;d.start(5,10);d.extend(30,10);d.finish();std::vector<uint32_t> f(W*H,0);drawChalk(d,f.data(),W,H,0,0);
  bool ok=true;for(int x=5;x<=30;++x)for(int y=9;y<=11;++y)ok&=f[y*W+x]==C;
  check(ok && f[8*W+15]==0 && f[12*W+15]==0 && count(f,C)==28*3,"horizontal line is 3px wide");}
 {ChalkOverlay d;d.start(105,210);d.extend(115,210);d.finish();std::vector<uint32_t> f(W*H,0);drawChalk(d,f.data(),W,H,100,200);
  check(f[10*W+5]==C && f[10*W+15]==C && f[10*W+16]==C && f[10*W+17]==0 && f[10*W+3]==0,"origin offset maps world to frame");}
 {ChalkOverlay d;d.start(-50,-50);d.extend(90,80);d.finish();d.start(0,0);d.finish();d.start(39,29);d.finish();
  std::vector<uint32_t> f(W*H,0);drawChalk(d,f.data(),W,H,0,0);
  check(f[0]==C && f[W*H-1]==C,"strokes crossing frame edges clip without overflow");}
 {ChalkOverlay d;d.start(500,500);d.extend(600,600);d.finish();std::vector<uint32_t> f(W*H,0);drawChalk(d,f.data(),W,H,0,0,0xff123456);
  check(count(f,0)==W*H,"stroke outside frame draws nothing");}
 {ChalkOverlay d;d.start(160,160);d.extend(320,160);d.finish();d.start(16,32);d.finish();
  std::vector<uint32_t> f(60*50,0);drawChalkOverview(d,f.data(),60,50);
  bool ok=true;for(int x=30;x<=60&&x<60;++x)ok&=f[30*60+x]==C;
  check(ok && f[29*60+40]==0 && f[31*60+40]==0,"overview line scaled by 3/16, 1px wide");
  check(f[6*60+3]==C && f[6*60+4]==0,"overview single point scaled by 3/16");}
 printf("RESULT: %d/%d passed\n",n-failed,n);return failed?1:0;
}

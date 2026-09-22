#include "notice-preview.h"
#include <cstdio>
int main(){int f=0,t=0;auto c=[&](bool x,const char*n){++t;if(!x){++f;std::printf("FAIL: %s\n",n);}};
CityMessage m{};m.picture=true;m.x=60;m.y=50;
c(noticePreviewVisible(m,true),"located picture message has a preview");
c(!noticePreviewVisible(m,false),"Notices option hides picture presentation");
m.picture=false;c(!noticePreviewVisible(m,true),"text-only message allocates no preview");
m.picture=true;m.x=-1;c(!noticePreviewVisible(m,true),"unlocated message allocates no blank preview");
int x=0,y=0;noticePreviewCamera(2,2,8,8,x,y);c(x==0&&y==0,"crop clamps top-left");
noticePreviewCamera(119,99,8,8,x,y);c(x==112&&y==92,"crop clamps bottom-right");
std::printf("RESULT: %d/%d passed\n",t-f,t);return f?1:0;}

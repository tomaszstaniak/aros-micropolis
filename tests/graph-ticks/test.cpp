#include "graph-model.h"
#include <cstdio>
static int n=0,failed=0;
static void check(bool ok,const char *s){++n;failed+=!ok;printf("%s %s\n",ok?"PASS":"FAIL",s);}
// Independent transcription of classic DoUpdateGraph (w_graph.c).
static std::vector<GraphTick> classic(int scale,long cityTime,int plotW){
 std::vector<GraphTick> t;int year=(int)(cityTime/48)+1900,month=(int)((cityTime/4)%12);
 if(scale==0){for(int x=120-month;x>=0;x-=12)t.push_back({x*(plotW-1)/120,year--});}
 else{int past=10*(year%10);year/=10;for(int x=1200-past;x>=0;x-=120)t.push_back({x*(plotW-1)/1200,10*year--});}
 return t;
}
static bool same(const std::vector<GraphTick>&a,const std::vector<GraphTick>&b){
 if(a.size()!=b.size())return false;for(size_t i=0;i<a.size();++i)if(a[i].x!=b[i].x||a[i].year!=b[i].year)return false;return true;}
int main(){
 const int W=466;
 auto jan=graphTicks(0,57*48,W); // January 1957
 check(jan.size()==11 && jan[0].x==W-1 && jan[0].year==1957 && jan[10].x==0 && jan[10].year==1947,"10-year January: ticks at both edges, 1957..1947");
 auto dec=graphTicks(0,57*48+44,W); // December 1957
 check(dec.size()==10 && dec[0].x==109*(W-1)/120 && dec[0].year==1957 && dec[9].x==1*(W-1)/120 && dec[9].year==1948,"10-year December: first tick 11 months back");
 auto d0=graphTicks(1,70*48,W); // 1970
 check(d0.size()==11 && d0[0].x==W-1 && d0[0].year==1970 && d0[10].year==1870 && d0[10].x==0,"120-year, year ending in 0: decade ticks at both edges");
 auto d9=graphTicks(1,79*48+20,W); // 1979
 check(d9.size()==10 && d9[0].x==1110*(W-1)/1200 && d9[0].year==1970 && d9[9].year==1880,"120-year, year ending in 9: first tick 9 years back");
 bool ok=true;
 for(long t:{0L,4L,47L,48L*57+44,48L*100+20,48L*139+47,48L*9+8})for(int s:{0,1})ok&=same(graphTicks(s,t,W),classic(s,t,W));
 check(ok,"matches classic transcription over months, decades, scales");
 ok=true;for(int s:{0,1}){auto t=graphTicks(s,48L*83+28,W);for(size_t i=1;i<t.size();++i)ok&=t[i].x<t[i-1].x && t[i].year<t[i-1].year;for(auto k:t)ok&=k.x>=0&&k.x<W;}
 check(ok,"ticks descend and stay inside the plot");
 printf("RESULT: %d/%d passed\n",n-failed,n);return failed?1:0;
}

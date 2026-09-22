#include "unsaved-city.h"
#include "micropolis.h"
#include "test-callback.h"
#include <cstdio>
#include <string>
static int n=0,failed=0;
static void check(bool ok,const char *s){++n;failed+=!ok;printf("%s %s\n",ok?"PASS":"FAIL",s);}
static bool exists(const std::string &p){FILE *f=fopen(p.c_str(),"rb");if(f)fclose(f);return f!=nullptr;}
int main(int argc,char **argv){
 if(argc<2)return 20;
 const std::string scratch=std::string(argv[1])+"/fingerprint.tmp";
 Micropolis m;m.setCallback(new TestCallback(),emscripten::val());m.init();
 m.totalFunds=20000;m.setSpeed(3);
 const uint64_t a=cityFingerprint(m,scratch.c_str()),b=cityFingerprint(m,scratch.c_str());
 check(a!=0 && a==b,"unchanged city has a stable nonzero fingerprint");
 check(!exists(scratch),"scratch file is removed afterwards");
 UnsavedCity u;check(u.modified(a),"baseline 0 counts as modified");
 u.baseline=a;check(!u.modified(a),"matching fingerprint is not modified");
 check(u.modified(0),"unknown fingerprint (0) counts as modified");
 m.setSpeed(0);const uint64_t paused=cityFingerprint(m,scratch.c_str());
 check(paused==a && m.simSpeed==0,"pausing alone does not modify; simSpeed restored (0)");
 m.setSpeed(3);check(cityFingerprint(m,scratch.c_str())==a && m.simSpeed==3,"speed change alone does not modify; simSpeed restored (3)");
 const auto tile=m.map[60][50]&LOMASK;const auto funds=m.totalFunds;
 m.toolDown(TOOL_ROAD,60,50);
 check((m.map[60][50]&LOMASK)!=tile || m.totalFunds!=funds,"fixture: road tool changed the city");
 const uint64_t edited=cityFingerprint(m,scratch.c_str());
 check(edited!=0 && edited!=a && u.modified(edited),"tool edit changes fingerprint");
 u.baseline=edited;
 m.setCityTax(m.cityTax==7?9:7);
 const uint64_t taxed=cityFingerprint(m,scratch.c_str());
 check(taxed!=edited && u.modified(taxed),"tax change changes fingerprint");
 u.baseline=taxed;
 const auto t0=m.cityTime;for(int i=0;i<200 && m.cityTime==t0;++i)m.simTick();
 check(m.cityTime!=t0,"fixture: simTick advanced cityTime");
 check(u.modified(cityFingerprint(m,scratch.c_str())),"simulated time changes fingerprint");
 const uint64_t bad=cityFingerprint(m,"/nonexistent-dir/xyz/fingerprint.tmp");
 check(bad==0 && u.modified(bad),"unwritable scratch path returns 0, treated as modified");
 check(m.simSpeed==3,"simSpeed restored after failed save");
 // confirmLeavingCity
 int asks=0,saves=0;
 auto ask=[&](UnsavedChoice c){return [&asks,c]{++asks;return c;};};
 auto save=[&](bool r){return [&saves,r]{++saves;return r;};};
 check(confirmLeavingCity(false,ask(UnsavedChoice::Cancel),save(false)) && asks==0 && saves==0,"unmodified city proceeds without asking");
 asks=saves=0;check(confirmLeavingCity(true,ask(UnsavedChoice::Discard),save(false)) && asks==1 && saves==0,"Discard proceeds without saving");
 asks=saves=0;check(confirmLeavingCity(true,ask(UnsavedChoice::Save),save(true)) && saves==1,"successful Save proceeds");
 asks=saves=0;check(!confirmLeavingCity(true,ask(UnsavedChoice::Save),save(false)) && saves==1,"failed or cancelled Save keeps the city");
 asks=saves=0;check(!confirmLeavingCity(true,ask(UnsavedChoice::Cancel),save(true)) && asks==1 && saves==0,"Cancel keeps the city and never saves");
 printf("RESULT: %d/%d passed\n",n-failed,n);return failed?1:0;
}

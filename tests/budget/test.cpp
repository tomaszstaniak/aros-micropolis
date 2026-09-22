#include "budget-model.h"
#include <cstdio>
#include <cmath>
#include <memory>
#include "test-callback.h"
int main(int argc,char **argv) {
 if(argc>2 && !freopen(argv[2],"w",stdout))return 20;
 Micropolis m; m.setCallback(new TestCallback(),emscripten::val()); m.init();
 m.autoBudget=true;m.totalFunds=10000;m.taxFund=1000;
 m.roadFund=m.fireFund=m.policeFund=100;
 m.roadPercent=m.firePercent=m.policePercent=0.5f;
 m.doBudget();
 int fails=0;
 auto check=[&](bool ok,const char *s){printf("%s %s\n",ok?"PASS":"FAIL",s);fails+=!ok;};
 check(m.totalFunds==10850,"annual settlement charges selected funding once");
 check(m.roadSpend==50 && m.fireSpend==50 && m.policeSpend==50,"autobudget records selected spending, not full costs");
 check(m.roadEffect==MAX_ROAD_EFFECT/2 && m.fireEffect==MAX_FIRE_STATION_EFFECT/2 && m.policeEffect==MAX_POLICE_STATION_EFFECT/2,"funding cuts affect real service effectiveness");
 auto original=BudgetDraft::read(m); auto draft=original;
 draft.set(0,25);draft.set(1,75);draft.set(2,10);draft.set(3,12);
 check(m.cityTax==original.tax && m.roadPercent==original.road,"editing and discarding a draft does not touch engine settings");
 draft=original;check(draft.road==original.road && draft.tax==original.tax,"reset restores exact original settings");
 draft.set(0,25);draft.set(1,75);draft.set(2,10);draft.set(3,12);draft.automatic=false;
 auto treasury=m.totalFunds;draft.apply(m);
 check(m.cityTax==12 && m.roadPercent==0.25f && m.firePercent==0.75f && m.policePercent==0.1f && !m.autoBudget,"accepted draft changes engine tax and funding");
 check(m.totalFunds==treasury,"editing budget never collects taxes or spends treasury");
 check(m.roadValue==25 && m.fireValue==75 && m.policeValue==10,"engine computes allocated amounts from accepted percentages");
 draft.set(0,-1);draft.set(1,101);draft.set(3,99);
 check(draft.road==0 && draft.fire==1 && draft.tax==20,"controls clamp to original tax and funding ranges");
 if(argc>1) {
   m.roadPercent=0.25f;m.firePercent=0.75f;m.policePercent=0.125f;m.setCityTax(12);
   check(m.saveFile(argv[1]),"budget fixture saves through real engine");
   m.roadPercent=m.firePercent=m.policePercent=1;m.setCityTax(7);
   check(m.loadCity(argv[1]),"budget fixture reloads through real engine");
   check(std::abs(m.roadPercent-.25f)<.0001 && std::abs(m.firePercent-.75f)<.0001 && std::abs(m.policePercent-.125f)<.0001 && m.cityTax==12,"load restores saved tax and all funding percentages");
   m.totalFunds=-1234; m.cityTime=0x123456;
   m.miscHist[10]=1234;m.miscHist[11]=2345;m.miscHist[64]=3456;m.miscHist[65]=4567;
   check(m.saveFile(argv[1]),"signed 32-bit city fixture saves");
   check(m.miscHist[10]==1234 && m.miscHist[11]==2345 && m.miscHist[64]==3456 && m.miscHist[65]==4567,
         "writing packed 32-bit fields preserves adjacent history words");
   unsigned char disk[4]={};FILE *saved=fopen(argv[1],"rb");bool diskOK=false;
   if(saved){diskOK=fseek(saved,6*HISTORY_LENGTH+50*2,SEEK_SET)==0 && fread(disk,1,4,saved)==4;fclose(saved);}
   check(diskOK && disk[0]==0xff && disk[1]==0xff && disk[2]==0xfb && disk[3]==0x2e,
         "negative funds encode as exactly four big-endian bytes on disk");
   check(m.loadCity(argv[1]) && m.totalFunds==-1234 && m.cityTime==0x123456,
         "signed funds and city time survive the classic 32-bit format");
 }
 printf("RESULT: %d failures\n",fails);
 return fails?1:0;
}

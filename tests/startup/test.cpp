#include "startup-model.h"
#include "../budget/test-callback.h"
#include <cstdio>
#include <unistd.h>
Micropolis *makeCity(){auto *m=new Micropolis;m->setCallback(new TestCallback(),emscripten::val());m->init();return m;}
int main(int argc,char **argv){
 int tests=0,failed=0;auto check=[&](bool ok,const char *s){++tests;failed+=!ok;printf("%s %s\n",ok?"PASS":"FAIL",s);};
 StartupModel model(makeCity);
 check(!model.city() && !model.previous() && !model.next(),"empty selection cannot play or navigate");
 model.generate(1234,0);auto *first=model.city();
 check(first && first->scenario==SC_NONE && first->totalFunds==20000 && first->cityTime==0,"new city has generated map, easy funds and reset time");
 model.level(2);check(first->gameLevel==LEVEL_HARD && first->totalFunds==5000,"difficulty sets new-city starting funds");
 first->setCityName("First City");
 model.generate(5678,1);auto *second=model.city();
 check(second!=first && second->totalFunds==10000,"regeneration creates separate medium city");
 check(model.previous() && model.city()==first && first->cityName=="First_City","back restores exact preview and edited name");
 check(model.next() && model.city()==second,"forward restores exact preview");
 check(!model.load("missing-file.cty") && model.city()==second,"failed load preserves selected city");
 const int ids[]={1,2,3,4,5,8,7,6};const int years[]={1900,1906,1944,1965,1957,2047,2010,1972};
 for(int i=0;i<8;++i){
  check(model.scenario(ids[i]),"bundled scenario loads without NULL-string crash");
  auto *m=model.city();
  check(m->scenario==ids[i] && m->cityTime==(years[i]-1900)*48+2 && m->totalFunds==(i?20000:5000),"scenario id, year and budget match engine rules");
  check(m->disasterEvent==ids[i] && m->scoreType==ids[i],"scenario disaster and scoring are initialized");
 }
 auto *scenario=model.city();auto funds=scenario->totalFunds;model.level(2);
 check(scenario->totalFunds==funds && scenario->gameLevel==LEVEL_HARD,"difficulty does not refill a scenario treasury");
 char cwd[4096];getcwd(cwd,sizeof cwd);chdir(argv[1]);
 check(!model.scenario(5) && model.city()==scenario,"missing scenario file preserves previous preview");
 FILE *bad=fopen("broken.cty","wb");fputs("broken",bad);fclose(bad);
 check(!model.load("broken.cty") && model.city()==scenario,"truncated city preserves previous preview");
 chdir(cwd);
 check(model.load("cities/haight.cty"),"existing city loads into preview");
 auto *loaded=model.city();funds=loaded->totalFunds;model.level(0);
 check(loaded->totalFunds==funds,"difficulty never replaces saved funds");
 model.previous();model.generate(8910,0);check(!model.next(),"successful new choice replaces forward history");
 for(int i=0;i<18;i++)model.generate(i,0);
 int back=0;while(model.previous())++back;check(back==15,"preview history bounded to sixteen cities");
 auto *chosen=model.city();auto selected=model.take();check(selected.get()==chosen,"play transfers the exact selected simulator");
 check(startupScenarioId(15)==8 && startupScenarioId(17)==6,"classic visual scenario order is explicitly mapped");
 auto layout=StartupLayout::fit(640,480);check(layout.width>0 && layout.height+24<=440,"small-screen layout fits with native chrome reserved");
 for(int i=0;i<18;i++){auto r=layout.button(i);check(layout.hit(r.x+r.w/2,r.y+r.h/2)==i,"scaled hit coordinates match original button");}
 check(layout.hit(-1,0)==-1 && layout.hit(layout.width,layout.height)==-1,"outside points never activate a button");
 printf("RESULT: %d/%d passed\n",tests-failed,tests);return failed?1:0;
}

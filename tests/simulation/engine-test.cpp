#include "micropolis.h"
#include "simulation-control.h"
#include "../budget/test-callback.h"
#include <cstdio>
#include <memory>
int main(){
 long elapsed[3]={};
 for(int speed=1;speed<=3;++speed){
  auto m=std::make_unique<Micropolis>();m->setCallback(new TestCallback(),emscripten::val());m->init();m->generateSomeCity(1234);
  auto before=m->cityTime;
  for(int i=0;i<200;++i)simulationBatch(*m,speed,[]{return false;});
  elapsed[speed-1]=m->cityTime-before;
  printf("speed %d: %ld cityTime units / 200 batches\n",speed,elapsed[speed-1]);
 }
 bool ok=elapsed[0]>0 && elapsed[1]>elapsed[0] && elapsed[2]>elapsed[1];
 printf("RESULT engine speed ordering: %s\n",ok?"PASS":"FAIL");return ok?0:1;
}

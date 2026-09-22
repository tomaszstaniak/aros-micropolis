#include "simulation-control.h"
#include <cstdio>
int main(){
 int n=0,fail=0;auto check=[&](bool ok,const char *s){++n;if(!ok)++fail;printf("%s %s\n",ok?"PASS":"FAIL",s);};
 SimulationControl c;
 check(!c.running && c.speed==3,"starts paused with fast resume speed");
 c.select(1);check(c.running && c.speed==1,"Slow starts slow");
 c.toggle();check(!c.running && c.speed==1,"pause remembers speed");
 c.toggle();check(c.running && c.speed==1,"resume restores slow");
 c.select(2);check(c.running && c.speed==2,"Medium starts medium");
 c.select(0);check(!c.running && c.speed==2,"explicit Pause retains medium");
 c.select(-1);check(!c.running && c.speed==2,"cancel does not alter speed");
 c.select(4);check(!c.running && c.speed==2,"invalid selection ignored");
 check(simulationDate(0).month==1 && simulationDate(0).year==1900,"January starts at month one");
 check(simulationDate(47).month==12 && simulationDate(47).year==1900,"last unit of year is December");
 check(simulationDate(48).month==1 && simulationDate(48).year==1901,"December rolls into next January");
 check(simulationDate(4851).month==1 && simulationDate(4851).year==2001,"saved city date from engine time");
 struct City {int speed=0,ticks=0;void setSpeed(int s){speed=s;}void simTick(){++ticks;}} city;
 simulationBatch(city,2,[]{return false;});check(city.speed==2 && city.ticks==16,"timer batch uses selected engine speed and bounded ticks");
 city.ticks=0;simulationBatch(city,3,[&]{return city.ticks==3;});check(city.ticks==3,"modal callback interrupts batch immediately");
 printf("RESULT %d/%d\n",n-fail,n);return fail?1:0;
}

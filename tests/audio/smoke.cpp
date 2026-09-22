#include "game-audio.h"
#include <proto/dos.h>
#include <cstdio>
#ifndef MICROPOLIS_AUDIO_TARGET
#define MICROPOLIS_AUDIO_TARGET "unspecified"
#endif
int main(int argc,char** argv) {
 if(argc!=3) return 2;
 FILE* report=std::fopen(argv[2],"w"); if(!report)return 2;
 std::fprintf(report,"Native AHI audio smoke; target=%s; backend=ahi.device unit 0 PCM16 mono\n",MICROPOLIS_AUDIO_TARGET);
 GameAudio audio;
 bool pass=audio.open(argv[1]);
 std::fprintf(report,"open=%d loaded=%u (expected 11)\n",pass,audio.loaded());
 pass=pass && audio.loaded()==11;
 const char* names[]={"ExplosionHigh","ExplosionLow","FogHornLow","HeavyTraffic","HonkHonkHigh","HonkHonkLow","HonkHonkMed","Monster","Siren","Sorry","UhUh"};
 for(const char* name:names) {
  const unsigned before=audio.completed();
  bool sent=audio.play(name);
  unsigned ticks=0;
  while(audio.busy() && ticks++<1500) { Delay(1); audio.poll(); }
  const bool ok=sent && !audio.busy() && audio.completed()==before+1 && audio.errors()==0;
  std::fprintf(report,"%s sent=%d completed=%u errors=%u ticks=%u result=%s\n",name,sent,audio.completed(),audio.errors(),ticks,ok?"PASS":"FAIL"); std::fflush(report);
  pass=pass && ok;
  if(audio.busy()) break;
 }
 const unsigned burstBefore=audio.completed();
 bool burst=true;
 for(int i=0;i<4;++i)burst=audio.play(names[i]) && burst;
 const bool fifthDropped=!audio.play(names[4]);
 unsigned burstTicks=0;
 while(audio.busy() && burstTicks++<1500) { Delay(1); audio.poll(); }
 const bool burstOk=burst && fifthDropped && !audio.busy() &&
     audio.completed()==burstBefore+4 && audio.errors()==0;
 std::fprintf(report,"fourVoiceBurst=%d fifthBusyDrop=%d completed=%u errors=%u ticks=%u result=%s\n",
     burst,fifthDropped,audio.completed(),audio.errors(),burstTicks,burstOk?"PASS":"FAIL");
 pass=pass && burstOk;
 pass=pass && !audio.play("../unknown");
 // Exercise shutdown with an outstanding device request, then reopen.
 bool sent=audio.play("Monster"); audio.close();
 bool reopened=audio.open(argv[1]); audio.close(); audio.close();
 bool missing=!audio.open("RAM:micropolis-missing-sounds") && !audio.play("Siren");
 pass=pass && sent && reopened && missing;
 std::fprintf(report,"abort-close/reopen=%d missing-safe=%d\nRESULT: %s\n",sent&&reopened,missing,pass?"PASS":"FAIL");
 std::fclose(report); return pass?0:1;
}

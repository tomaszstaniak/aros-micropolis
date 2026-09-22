#include "game-audio.h"
#include <devices/ahi.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <string>
struct State {IORequest* request;bool pending=false,done=false;};
static State states[16];static int stateCount=0;
static int failure=0,ports=0,requests=0,opened=0,sends=0,waits=0,aborts=0;
static State& state(IORequest* request){for(int i=0;i<stateCount;++i)if(states[i].request==request)return states[i];assert(false);return states[0];}
MsgPort* CreateMsgPort(){ if(failure==1)return nullptr; ++ports; return new MsgPort; }
void* CreateIORequest(MsgPort*,size_t){if(failure==2)return nullptr;assert(stateCount<16);auto* request=new AHIRequest;states[stateCount++]={reinterpret_cast<IORequest*>(request),false,false};++requests;return request;}
int OpenDevice(const char* name,unsigned unit,IORequest* req,unsigned){assert(!std::strcmp(name,"ahi.device") && !unit);assert(reinterpret_cast<AHIRequest*>(req)->ahir_Version==4);if(failure==3)return 1;++opened;return 0;}
IORequest* CheckIO(IORequest* req){auto& s=state(req);assert(s.pending);return s.done?req:nullptr;}
void WaitIO(IORequest* req){auto& s=state(req);assert(s.pending && s.done); // The sample is still owned until after completion.
 assert(static_cast<int16_t*>(req->io_Data)[0]==-32768);s.pending=false;++waits;}
void SendIO(IORequest* req){auto& s=state(req);assert(opened && !s.pending && req->io_Command==CMD_WRITE && req->io_Length==6);auto* ahi=reinterpret_cast<AHIRequest*>(req);assert(ahi->ahir_Type==AHIST_M16S && ahi->ahir_Frequency==22050 && ahi->ahir_Link==nullptr);s.pending=true;s.done=false;++sends;}
void AbortIO(IORequest* req){auto& s=state(req);assert(s.pending);s.done=true;++aborts;}
void CloseDevice(IORequest* req){assert(!state(req).pending);assert(opened>0);--opened;}
void DeleteIORequest(IORequest* req){assert(!state(req).pending);--requests;for(int i=0;i<stateCount;++i)if(states[i].request==req){states[i]=states[--stateCount];break;}delete reinterpret_cast<AHIRequest*>(req);}
void DeleteMsgPort(MsgPort* p){--ports;delete p;}
int main(int argc,char** argv){
 assert(argc==2); const std::string dir=argv[1];
 const unsigned char bytes[]={'R','I','F','F',42,0,0,0,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,1,0,34,86,0,0,68,172,0,0,2,0,16,0,'d','a','t','a',6,0,0,0,0,128,0,0,255,127};
 FILE* f=std::fopen((dir+"/Monster.wav").c_str(),"wb");assert(f);assert(std::fwrite(bytes,1,sizeof(bytes),f)==sizeof(bytes));std::fclose(f);
 GameAudio audio;assert(!audio.play("Monster"));assert(!audio.open("/nonexistent/audio"));
 for(failure=1;failure<=3;++failure){assert(!audio.open(dir.c_str()));assert(!ports && !requests && !opened && !audio.available());}
 failure=0;assert(audio.open(dir.c_str()) && audio.loaded()==1);
 assert(ports==4 && requests==4 && opened==4);
 assert(!audio.play("Siren") && !audio.play("../Monster") && !audio.play(nullptr));
 for(int i=0;i<4;++i)assert(audio.play("Monster"));
 assert(!audio.play("Monster"));audio.poll();assert(waits==0 && sends==4 && audio.busy());
 for(int i=0;i<stateCount;++i)states[i].done=true;
 audio.poll();assert(waits==4 && audio.completed()==4 && !audio.busy());
 assert(audio.play("Monster"));states[0].request->io_Error=1;states[0].done=true;
 audio.poll();assert(audio.errors()==1 && audio.completed()==4);
 for(int i=0;i<4;++i)assert(audio.play("Monster"));audio.close();assert(aborts==4 && waits==9 && !ports && !requests && !opened);
 audio.close();assert(audio.open(dir.c_str()));assert(audio.play("Monster"));
 assert(audio.open(dir.c_str()));assert(aborts==5 && !audio.busy());audio.close();assert(!ports && !requests && !opened);
 puts("audio transport: four voices, fifth busy-drop, failure cleanup, errors and abort-before-free pass");
}

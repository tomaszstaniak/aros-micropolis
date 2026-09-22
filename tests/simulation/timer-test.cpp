#include "simulation-timer.h"
#include <cstdio>
#include <cstdlib>
#include <string>
static int failure=0,sends=0;static bool completed=false;
static std::string events;
MsgPort *CreateMsgPort(){return failure==1?nullptr:new MsgPort;}
void *CreateIORequest(MsgPort *,unsigned long){return failure==2?nullptr:new timerequest;}
int OpenDevice(CONST_STRPTR,int,IORequest *,int){return failure==3?1:0;}
void CloseDevice(IORequest *){events+='C';}
void DeleteIORequest(IORequest *r){events+='R';delete (timerequest *)r;}
void DeleteMsgPort(MsgPort *p){events+='P';delete p;}
void SendIO(IORequest *r){++sends;completed=false;auto *t=(timerequest *)r;if(t->tr_time.tv_micro!=100000 || t->tr_time.tv_secs || r->io_Command!=TR_ADDREQUEST)abort();events+='S';}
IORequest *CheckIO(IORequest *r){return completed?r:nullptr;}
int WaitIO(IORequest *){events+='W';return 0;}
void AbortIO(IORequest *){events+='A';completed=true;}
int main(){
 int n=0,fail=0;auto check=[&](bool ok,const char *s){++n;fail+=!ok;printf("%s %s\n",ok?"PASS":"FAIL",s);};
 for(failure=1;failure<=3;++failure){events.clear();{SimulationTimer t;check(!t.open(),"partial initialization reports failure");}check(events==(failure==1?"":failure==2?"P":"RP"),"partial initialization releases only acquired resources");}
 failure=0;events.clear();
 {SimulationTimer t;check(t.open() && t.signal()==16,"dedicated timer signal");t.start();t.start();check(sends==1,"at most one outstanding timer");check(!t.consume(),"uncompleted request does not tick");completed=true;check(t.consume() && !t.consume(),"one completion produces exactly one batch");t.start();t.stop();check(events=="SWSAW","stop aborts and waits, no catch-up");t.start();}
 check(events=="SWSAWSAWCRP","pending IO completes before device and port destruction");
 events.clear();{SimulationTimer t;t.open();t.start();completed=true;t.stop();}check(events=="SWCRP","completed deadline drained without abort");
 printf("RESULT timer %d/%d\n",n-fail,n);return fail?1:0;
}

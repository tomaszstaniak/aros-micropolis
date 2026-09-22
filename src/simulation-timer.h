#ifndef MICROPOLIS_SIMULATION_TIMER_H
#define MICROPOLIS_SIMULATION_TIMER_H
#include <devices/timer.h>
#include <proto/exec.h>
// Single outstanding one-shot request. Rearm after processing, so a blocked
// modal or slow frame cannot accumulate a burst of simulation work.
class SimulationTimer {
public:
    SimulationTimer()=default;
    SimulationTimer(const SimulationTimer&)=delete;
    SimulationTimer& operator=(const SimulationTimer&)=delete;
    ~SimulationTimer(){
        stop();
        if(open_)CloseDevice((IORequest *)request_);
        if(request_)DeleteIORequest((IORequest *)request_);
        if(port_)DeleteMsgPort(port_);
    }
    bool open(){
        port_=CreateMsgPort();
        if(!port_)return false;
        request_=(timerequest *)CreateIORequest(port_,sizeof(timerequest));
        if(!request_)return false;
        open_=OpenDevice((CONST_STRPTR)TIMERNAME,UNIT_MICROHZ,(IORequest *)request_,0)==0;
        return open_;
    }
    ULONG signal()const{return port_?1UL<<port_->mp_SigBit:0;}
    void start(){
        if(!open_ || pending_)return;
        request_->tr_node.io_Command=TR_ADDREQUEST;
        request_->tr_time.tv_secs=0;request_->tr_time.tv_micro=100000;
        SendIO((IORequest *)request_);pending_=true;
    }
    bool consume(){
        if(!pending_ || !CheckIO((IORequest *)request_))return false;
        WaitIO((IORequest *)request_);pending_=false;return true;
    }
    void stop(){
        if(!pending_)return;
        if(!CheckIO((IORequest *)request_))AbortIO((IORequest *)request_);
        WaitIO((IORequest *)request_);pending_=false;
    }
private:
    MsgPort *port_=nullptr;
    timerequest *request_=nullptr;
    bool open_=false,pending_=false;
};
#endif

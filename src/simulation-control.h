#ifndef MICROPOLIS_SIMULATION_CONTROL_H
#define MICROPOLIS_SIMULATION_CONTROL_H
#include <cstdint>
struct SimulationControl {
    bool running=false;
    int speed=3;
    void toggle(){running=!running;}
    void select(int value){
        if(value<0 || value>3)return;
        running=value!=0;
        if(value)speed=value;
    }
};
inline const char *simulationSpeedName(int speed){
    return speed==1?"slow":speed==2?"medium":"fast";
}
struct SimulationDate{int year,month;};
inline SimulationDate simulationDate(int64_t cityTime){
    return {1900+(int)(cityTime/48),1+(int)((cityTime/4)%12)};
}
// One bounded frontend batch, not a calendar month. Stop immediately when
// an engine callback opens a modal dialog; no post-dialog catch-up work.
template<class City,class Stop>
void simulationBatch(City &city,int speed,Stop stop){
    city.setSpeed(speed);
    for(int i=0;i<16 && !stop();++i)city.simTick();
}
#endif

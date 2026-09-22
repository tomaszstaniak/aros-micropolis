#ifndef MICROPOLIS_STARTUP_MODEL_H
#define MICROPOLIS_STARTUP_MODEL_H
#include "micropolis.h"
#include <algorithm>
#include <memory>
#include <vector>

struct StartupRect{int x,y,w,h;};
// res/micropolis.tcl ScenarioButtons: artwork order differs from engine IDs.
inline constexpr StartupRect startupButtons[]={
 {70,238,157,90},{62,392,157,90},{68,544,157,90},{101,705,157,90},
 {982,106,190,70},{982,176,190,70},{982,246,190,70},
 {540,375,50,50},{841,375,50,50},{625,376,180,50},
 {310,451,209,188},{519,451,209,188},{727,450,209,188},{936,450,209,188},
 {310,639,209,188},{519,639,209,188},{728,638,209,188},{937,638,209,188}};
inline int startupScenarioId(int slot){
    constexpr int ids[]={1,2,3,4,5,8,7,6};return slot>=10 && slot<18?ids[slot-10]:0;
}
struct StartupLayout {
    int width=0,height=0;
    static StartupLayout fit(int w,int h){
        const int width=std::max(0,std::min({1200,w-16,(h-64)*1200/900}));
        return {width,width*900/1200};
    }
    StartupRect rect(StartupRect r)const{
        const int x=r.x*width/1200,y=r.y*height/900;
        return {x,y,(r.x+r.w)*width/1200-x,(r.y+r.h)*height/900-y};
    }
    StartupRect button(int i)const{return rect(startupButtons[i]);}
    int hit(int x,int y)const{
        for(int i=0;i<18;i++){auto r=button(i);if(x>=r.x && x<r.x+r.w && y>=r.y && y<r.y+r.h)return i;}
        return -1;
    }
};

class StartupModel {
public:
    using Factory=Micropolis *(*)();
    explicit StartupModel(Factory factory):factory_(factory){}
    Micropolis *city()const{return cities_.empty()?nullptr:cities_[index_].city.get();}
    bool generate(int seed,int level){
        std::unique_ptr<Micropolis> next(factory_());
        if(!next)return false;
        next->generateSomeCity(seed);
        next->setGameLevelFunds((GameLevel)std::clamp(level,0,2));
        next->setCityName("NowHere");
        append(std::move(next),true);return true;
    }
    bool load(const std::string &path){
        std::unique_ptr<Micropolis> next(factory_());
        if(!next || !next->loadCity(path))return false;
        append(std::move(next),false);return true;
    }
    bool scenario(int id){
        if(id<1 || id>8)return false;
        std::unique_ptr<Micropolis> next(factory_());
        if(!next || !next->loadScenario((Scenario)id))return false;
        append(std::move(next),false);return true;
    }
    void level(int value){
        if(!city() || value<0 || value>2)return;
        if(cities_[index_].generated)city()->setGameLevelFunds((GameLevel)value);
        else city()->setGameLevel((GameLevel)value);
    }
    bool canPrevious()const{return !cities_.empty() && index_>0;}
    bool canNext()const{return !cities_.empty() && index_+1<cities_.size();}
    bool previous(){if(!canPrevious())return false;--index_;return true;}
    bool next(){if(!canNext())return false;++index_;return true;}
    std::unique_ptr<Micropolis> take(){return city()?std::move(cities_[index_].city):nullptr;}
private:
    struct Entry{std::unique_ptr<Micropolis> city;bool generated;};
    void append(std::unique_ptr<Micropolis> next,bool generated){
        // Failed reads never get here, so both current and forward history
        // survive cancellation/errors. Bound native memory use explicitly.
        if(!cities_.empty())cities_.resize(index_+1);
        cities_.push_back({std::move(next),generated});
        if(cities_.size()>16)cities_.erase(cities_.begin());
        index_=cities_.size()-1;
    }
    Factory factory_;
    std::vector<Entry> cities_;
    size_t index_=0;
};
#endif

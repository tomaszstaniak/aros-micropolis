#include "micropolis.h"
#include "../budget/test-callback.h"
#include <algorithm>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

struct RecordingCallback : TestCallback {
    std::vector<int> messages;
    std::vector<std::string> sounds;
    int earthquake = 0, won = 0, lost = 0;
    void sendMessage(Micropolis *, emscripten::val, int id, int, int, bool, bool) override { messages.push_back(id); }
    void makeSound(Micropolis *, emscripten::val, std::string, std::string sound, int, int) override { sounds.push_back(sound); }
    void startEarthquake(Micropolis *, emscripten::val, int strength) override { earthquake = strength; }
    void didWinGame(Micropolis *, emscripten::val) override { ++won; }
    void didLoseGame(Micropolis *, emscripten::val) override { ++lost; }
    bool has(int id) const { return std::find(messages.begin(), messages.end(), id) != messages.end(); }
};
struct City {
    RecordingCallback *events = new RecordingCallback;
    std::unique_ptr<Micropolis> engine = std::make_unique<Micropolis>();
    City() {
        engine->setCallback(events, emscripten::val()); engine->init();
        engine->seedRandom(1234); engine->enableDisasters = false; engine->enableSound = true;
        engine->setSpeed(3);
        fill(RESBASE | BURNBIT | BULLBIT);
    }
    void fill(MapValue tile) { for (int x=0;x<WORLD_W;++x) for(int y=0;y<WORLD_H;++y) engine->map[x][y]=tile; }
    int count(int low, int high) const {
        int n=0; for(int x=0;x<WORLD_W;++x) for(int y=0;y<WORLD_H;++y) {
            int t=engine->map[x][y]&LOMASK; n += t>=low && t<=high;
        } return n;
    }
};
int main(int argc, char **argv) {
    if(argc!=2) return 2;
    int checks=0, failures=0;
    auto check=[&](bool value,const std::string &name) { ++checks; failures+=!value; printf("%s %s\n",value?"PASS":"FAIL",name.c_str()); };
    auto roundtrip=[&](City &city,const char *name) {
        std::vector<MapValue> before;
        for(int x=0;x<WORLD_W;++x) for(int y=0;y<WORLD_H;++y) before.push_back(city.engine->map[x][y]);
        std::string path=std::string(argv[1])+"/"+name+".cty";
        check(city.engine->saveFile(path),std::string(name)+": damaged city saves");
        City restored; check(restored.engine->loadFileData(path),std::string(name)+": damaged map data loads");
        bool same=true; size_t i=0;
        for(int x=0;x<WORLD_W;++x) for(int y=0;y<WORLD_H;++y) same &= before[i++]==restored.engine->map[x][y];
        check(same,std::string(name)+": all 12000 tile values and flags survive raw reload");
        // loadCity performs two simulation scans, so active hazards may evolve.
        check(restored.engine->loadCity(path),std::string(name)+": full city loader accepts damaged save");
        check(restored.count(FIRE,LASTFIRE)+restored.count(RUBBLE,LASTRUBBLE)+restored.count(FLOOD,LASTFLOOD)+restored.count(RADTILE,RADTILE)+restored.count(TINYEXP,LASTTINYEXP)>0,std::string(name)+": damage remains after load initialization");
    };
    {
        City c; auto &m=*c.engine; m.makeFire();
        check(c.count(FIRE,LASTFIRE)==1 && c.events->has(MESSAGE_FIRE_REPORTED),"fire ignites burnable tile and reports it");
        for(int step=0;step<320;++step) m.simTick();
        check(c.count(FIRE,LASTFIRE)+c.count(RUBBLE,LASTRUBBLE)>1,"fire spreads and consumes fuel"); roundtrip(c,"fire");
    }
    {
        City c; auto &m=*c.engine;
        for(int x=0;x<WORLD_W;x+=2) for(int y=0;y<WORLD_H;++y) m.map[x][y]=FIRSTRIVEDGE;
        m.makeFlood(); check(c.count(FLOOD,LASTFLOOD)==1 && m.floodCount==30 && c.events->has(MESSAGE_FLOODING_REPORTED),"shoreline flood starts with timer and message");
        for(int step=0;step<160;++step) m.simTick();
        check(c.count(FLOOD,LASTFLOOD)>1,"flood spreads into vulnerable neighboring tiles"); roundtrip(c,"flood");
        for(int i=0;i<1024;++i) m.simTick(); check(m.floodCount==0,"flood timer expires with random disasters disabled");
    }
    for(bool monster:{false,true}) {
        City c; auto &m=*c.engine; const char *name=monster?"monster":"tornado";
        if(monster) m.makeMonster(); else m.makeTornado();
        auto *sprite=m.getSprite(monster?SPRITE_MONSTER:SPRITE_TORNADO);
        check(sprite && sprite->frame>0 && c.events->has(monster?MESSAGE_MONSTER_SIGHTED:MESSAGE_TORNADO_SIGHTED),std::string(name)+": active sprite and sighting message");
        int x=sprite?sprite->x:0,y=sprite?sprite->y:0;
        for(int i=0;i<64;++i) m.moveObjects();
        check(!sprite || sprite->x!=x || sprite->y!=y,std::string(name)+": sprite moves");
        check(c.count(TINYEXP,LASTTINYEXP)+c.count(FIRE,LASTFIRE)+c.count(RUBBLE,LASTRUBBLE)>0,std::string(name)+": movement damages built tiles"); roundtrip(c,name);
    }
    {
        City c; c.engine->makeEarthquake();
        check(c.events->earthquake>=300 && c.events->earthquake<=1000 && c.events->has(MESSAGE_EARTHQUAKE),"earthquake sends bounded strength and message");
        check(c.count(FIRE,LASTFIRE)>0 && c.count(RUBBLE,LASTRUBBLE)>0,"earthquake creates both fire and rubble");
        check(std::find(c.events->sounds.begin(),c.events->sounds.end(),"ExplosionLow")!=c.events->sounds.end(),"earthquake requests ExplosionLow sound"); roundtrip(c,"earthquake");
    }
    {
        City c; auto &m=*c.engine; m.map[60][50]=NUCLEAR|ZONEBIT|BURNBIT; m.makeMeltdown();
        check(c.count(RADTILE,RADTILE)>0 && c.count(FIRE,LASTFIRE)>=16 && c.events->has(MESSAGE_NUCLEAR_MELTDOWN),"meltdown burns plant and leaves radioactive damage plus message");
        int explosions=0; for(auto *s=m.spriteList;s;s=s->next) explosions+=s->type==SPRITE_EXPLOSION && s->frame>0;
        check(explosions==4,"meltdown creates four explosion sprites"); roundtrip(c,"meltdown");
    }
    // Exercise random selection through normal public simulation ticks. The
    // disabled control has the same seed, terrain and per-tick difficulty /
    // pollution inputs, with no direct make* calls or substituted RNG.
    for(bool enabled:{false,true}) {
        City c; auto &m=*c.engine; m.seedRandom(24680);
        for(int x=0;x<WORLD_W;++x) for(int y=0;y<WORLD_H;++y)
            m.map[x][y]=(x%12<2)?FIRSTRIVEDGE:((LHTHR+1)|BURNBIT|BULLBIT);
        m.enableDisasters=enabled; m.gameLevel=LEVEL_HARD;
        const int randomMessages[]={MESSAGE_FIRE_REPORTED,MESSAGE_FLOODING_REPORTED,
            MESSAGE_TORNADO_SIGHTED,MESSAGE_EARTHQUAKE,MESSAGE_MONSTER_SIGHTED};
        for(int tick=0;tick<20000;++tick) {
            // Census normally changes this input; hold the documented monster
            // eligibility condition so the scheduler branch remains reachable.
            m.pollutionAverage=100;
            m.simTick();
        }
        for(int message:randomMessages)
            check(c.events->has(message)==enabled,
                  std::string(enabled?"random scheduler emits ":"disabled random scheduler suppresses ")+std::to_string(message));
    }
    const Scenario scheduled[]={SC_SAN_FRANCISCO,SC_HAMBURG,SC_TOKYO,SC_BOSTON,SC_RIO};
    const int messages[]={MESSAGE_EARTHQUAKE,MESSAGE_FIREBOMBING,MESSAGE_MONSTER_SIGHTED,MESSAGE_NUCLEAR_MELTDOWN,MESSAGE_FLOODING_REPORTED};
    for(int i=0;i<5;++i) {
        City c; auto &m=*c.engine; m.loadScenario(scheduled[i]);
        m.enableDisasters=false; m.seedRandom(1234); c.events->messages.clear();
        // Preserve each bundled map; shorten only its deterministic countdown.
        m.disasterWait=scheduled[i]==SC_RIO?24:scheduled[i]==SC_HAMBURG?10:1;
        for(int tick=0;tick<1024 && !c.events->has(messages[i]);++tick) m.simTick();
        check(c.events->has(messages[i]),"scheduled disaster runs on bundled scenario despite random disasters disabled: "+std::to_string(scheduled[i]));
    }
    // Score at the real sendMessages deadline, setting boundary values rather
    // than waiting years or conflating growth simulation with score routing.
    for(int id=1;id<=8;++id) for(bool win:{false,true}) {
        City c; auto &m=*c.engine; m.loadScenario(static_cast<Scenario>(id));
        check(m.scenario==id && m.scoreType==id && m.scoreWait>0,"bundled scenario initializes scoring: "+std::to_string(id));
        m.cityClass=win?CC_METROPOLIS:CC_CITY; m.cityScore=win?501:500;
        m.trafficAverage=win?79:80; m.crimeAverage=win?59:60; m.scoreWait=1;
        c.events->messages.clear(); m.sendMessages();
        check(m.scoreWait==0 && c.events->has(win?MESSAGE_SCENARIO_WON:MESSAGE_SCENARIO_LOST) && c.events->lost==(!win),"scenario "+std::to_string(id)+(win?" victory":" loss")+" deadline routes message and loss callback");
        // Current engine emits victory only as a message, never didWinGame.
        check(c.events->won==0,"scenario victory callback is absent in pinned engine contract");
        m.sendMessages();
        int resultCount=std::count(c.events->messages.begin(),c.events->messages.end(),win?MESSAGE_SCENARIO_WON:MESSAGE_SCENARIO_LOST);
        check(resultCount==1 && c.events->lost==(!win),"expired scenario deadline does not repeat result");
    }
    printf("RESULT: %d/%d passed\n",checks-failures,checks); return failures?1:0;
}

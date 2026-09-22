#include "messages.h"
#include <cstdio>
#include <fstream>
#include <limits>
#include <map>
#include <string>
#include <vector>

static int checks=0,failures=0;

static void check(bool ok,const std::string& name) {
    ++checks;
    failures+=!ok;
    std::printf("%s %s\n",ok ? "PASS" : "FAIL",name.c_str());
}

static bool isText(int id,const std::string& expected) {
    const char* actual=messageText(id);
    return actual && actual==expected;
}

static void catalog(const std::string& directory) {
    std::ifstream resource(directory+"/stri.301");
    std::vector<std::string> shorts;
    std::string line;
    while(std::getline(resource,line)) shorts.push_back(line);
    check(shorts.size()==64,"independent classic short resource has all 64 lines");
    for(int id=1;id<=44;++id) {
        check(shorts.size()>=static_cast<size_t>(id) && isText(id,shorts[id-1]),
              "catalog matches classic short resource "+std::to_string(id));
    }
    std::ifstream titlesFile(directory+"/classic-titles.tsv");
    std::map<int,std::string> titles;
    while(std::getline(titlesFile,line)) {
        const auto tab=line.find('\t');
        if(tab!=std::string::npos) titles.emplace(std::stoi(line.substr(0,tab)),line.substr(tab+1));
    }
    check(titles.size()==15,"independent Tcl title fixture retains original notice IDs");
    const std::pair<int,int> mapping[]={
        {45,48},{46,49},{47,100},{48,200},{49,300},
        {50,1},{51,2},{52,3},{53,4},{54,5},{55,6},{56,7},{57,8}
    };
    for(const auto& pair:mapping) {
        check(titles.count(pair.second) && isText(pair.first,titles.at(pair.second)),
              "engine "+std::to_string(pair.first)+" maps to Tcl "+std::to_string(pair.second));
    }
    check(isText(12,"Frequent traffic jams reported.") && isText(13,"Citizens demand a Fire Department.") &&
          isText(14,"Citizens demand a Police Department.") && isText(15,"Blackouts reported. Check power map."),
          "traffic, fire, police and blackout IDs are not shifted");
    check(isText(20,"Fire reported !") && isText(28,"Unemployment rate is high.") &&
          isText(29,"YOUR CITY HAS GONE BROKE!") && isText(30,"Firebombing reported !") &&
          isText(31,"Need more parks.") && isText(32,"Explosion detected !") &&
          isText(33,"Insufficient funds to build that.") && isText(34,"Area must be bulldozed first."),
          "disaster, money and tool errors retain exact numeric slots");
    check(isText(35,"Population has reached 2,000.") && isText(39,"Population has reached 500,000.") &&
          isText(40,"Brownouts, build another Power Plant.") && !isText(40,titles[40]),
          "growth milestones end at 39; engine 40 is not Tcl MEGALINIUM");
    check(isText(45,"Start a New City") && isText(46,"Restore a Saved City") &&
          isText(47,"YOU'RE A WINNER!") && isText(48,"IMPEACHMENT NOTICE!") && isText(49,"About Micropolis"),
          "engine 45 through 49 do not reuse obsolete classic short slots");
    check(messageText(0)==nullptr && messageText(-1)==nullptr && messageText(58)==nullptr &&
          messageText(std::numeric_limits<int>::min())==nullptr &&
          messageText(std::numeric_limits<int>::max())==nullptr,"unknown catalog IDs return nullptr without indexing");
    MessageHistory history;
    for(int id=1;id<=57;++id) {
        history.receive(id,-1,-1,false,false,id);
        check(history.entries().back().text==messageText(id) && history.entries().back().id==id &&
              !history.entries().back().picture && !history.entries().back().important,
              "receive owns catalog text without inferred flags "+std::to_string(id));
    }
}

static void ownershipAndLocations() {
    MessageHistory history;
    check(history.entries().empty() && history.revision()==0,"initial history is empty with zero revision");
    {
        std::string text="temporary callback buffer";
        history.receiveText(12,text.c_str(),0,0,true,false,1);
        text.assign(1000,'x');
    }
    check(history.entries().front().text=="temporary callback buffer","callback text survives mutation and destruction");
    check(history.entries().front().picture && !history.entries().front().important &&
          history.entries().front().hasLocation(),"callback flags preserved and origin is a location");
    history.receiveText(12,"",0,0,false,true,2);
    check(history.entries().back().text.empty(),"explicit empty text is preserved");
    history.receiveText(12,nullptr,-1,-1,false,false,3);
    check(history.entries().back().text=="Unknown simulation message (12)","null custom text uses owned numbered fallback");
    for(int id:{0,-1,58,std::numeric_limits<int>::min(),std::numeric_limits<int>::max()}) {
        history.receive(id,-1,-1,false,false,4);
        check(history.entries().back().text=="Unknown simulation message ("+std::to_string(id)+")",
              "unknown ID has exact owned fallback "+std::to_string(id));
    }
    const CityMessage saved=history.entries().back();
    history.receive(999,0,0,true,true,5);
    history.reset();
    check(saved.text=="Unknown simulation message (2147483647)","copied unknown text survives later fallback and reset");
    const int coordinates[][2]={{0,0},{119,99},{119,0},{0,99},{-1,-1},{-1,0},{0,-1},
        {120,0},{0,100},{120,100},{std::numeric_limits<int>::min(),0},
        {0,std::numeric_limits<int>::max()}};
    for(size_t i=0;i<sizeof(coordinates)/sizeof(coordinates[0]);++i) {
        const int x=coordinates[i][0],y=coordinates[i][1];
        history.receive(20,x,y,false,false,6);
        const auto& entry=history.entries().back();
        check(entry.x==x && entry.y==y && entry.hasLocation()==(i<4),
              "raw location preserved and bounds checked "+std::to_string(i));
    }
}

static void deduplication() {
    MessageHistory history;
    history.receive(20,1,2,false,false,100);
    const auto firstSerial=history.entries().back().serial;
    const auto revision=history.revision();
    for(int64_t time:{100,101,120,147}) history.receive(20,1,2,false,false,time);
    check(history.entries().size()==1 && history.entries().back().time==100 &&
          history.entries().back().serial==firstSerial && history.revision()==revision,
          "suppression at 0 through 47 units changes neither timestamp, serial nor revision");
    history.receive(20,1,2,false,false,148);
    check(history.entries().size()==2 && history.entries().back().time==148 &&
          history.entries().back().serial==firstSerial+1,"exactly 48 units accepts recurrence despite recent suppression");
    history.receive(20,1,2,false,false,195);
    check(history.entries().size()==2,"dedup uses newest accepted match, not oldest recurrence");
    history.receive(20,1,2,false,false,196);
    history.receive(20,1,2,false,false,245);
    check(history.entries().size()==4,"48 and 49 unit later recurrences are always accepted");

    history.reset();
    history.receiveText(20,"A",1,2,false,false,0);
    history.receiveText(21,"A",1,2,false,false,1);
    history.receiveText(20,"B",1,2,false,false,2);
    history.receiveText(20,"A",2,2,false,false,3);
    history.receiveText(20,"A",1,3,false,false,4);
    history.receiveText(20,"A",1,2,true,false,5);
    history.receiveText(20,"A",1,2,false,true,6);
    history.receiveText(20,"A",1,2,true,true,7);
    check(history.entries().size()==8,"ID, text, each coordinate and both flags independently distinguish events");
    const auto interleavedRevision=history.revision();
    history.receiveText(20,"A",1,2,false,false,47);
    history.receiveText(20,"A",1,2,true,false,47);
    check(history.entries().size()==8 && history.revision()==interleavedRevision,
          "interleaving other messages and flag variants does not defeat dedup");
    history.receiveText(20,"A",1,2,false,false,48);
    history.receiveText(20,"A",1,2,true,false,52);
    check(history.entries().size()==9,"interleaved signatures retain separate accepted-time windows");
    history.receiveText(20,"A",1,2,true,false,53);
    check(history.entries().size()==10,"flag variant recurs exactly 48 after its own acceptance");
    history.receiveText(20,"A",-1,2,false,false,54);
    history.receiveText(20,"A",-2,2,false,false,54);
    check(history.entries().size()==12,"different invalid raw locations are not normalized into duplicates");
}

static void resetAndBounds() {
    MessageHistory history;
    uint64_t serial=0;
    bool bounded=true,ordered=true,revisions=true;
    for(int i=0;i<4096;++i) {
        const auto before=history.revision();
        history.receiveText(i,"queued callback",i%120,i%100,false,false,0);
        bounded=bounded && history.entries().size()==static_cast<size_t>(i<64 ? i+1 : 64);
        ordered=ordered && history.entries().back().serial>serial && history.entries().back().id==i &&
            history.entries().front().id==(i<64 ? 0 : i-63);
        revisions=revisions && history.revision()==before+1;
        serial=history.entries().back().serial;
    }
    check(MessageHistory::capacity==64 && bounded,"callback flood retains at most 64 owned entries");
    check(ordered && revisions,"oldest entry evicted with ordered serials and one revision per acceptance");
    history.receiveText(0,"queued callback",0,0,false,false,0);
    check(history.entries().back().id==0 && history.entries().back().serial>serial,
          "evicted signature is accepted; history itself is the dedup cache");
    serial=history.entries().back().serial;
    auto revision=history.revision();
    history.reset();
    check(history.entries().empty() && history.revision()==revision+1,"reset clears entries and invalidates selection revision");
    revision=history.revision();
    history.reset();
    check(history.revision()==revision+1,"empty reset still invalidates revision");
    history.receive(1,-1,-1,false,false,-100);
    check(history.entries().size()==1 && history.entries().back().serial>serial,
          "serial never restarts and explicit reset forgets prior clock");

    history.reset();
    history.receive(20,0,0,false,false,100);
    history.receive(20,0,0,false,false,147);
    serial=history.entries().back().serial;
    revision=history.revision();
    history.receive(20,0,0,false,false,146);
    check(history.entries().size()==1 && history.entries().back().time==146 &&
          history.entries().back().serial>serial && history.revision()==revision+2,
          "backward clock relative to suppressed input resets before accepting");
    history.receive(21,0,0,false,false,146);
    check(history.entries().size()==2,"equal clock does not reset history");
    history.receive(22,0,0,false,false,-1);
    check(history.entries().size()==1 && history.entries().back().id==22,"negative backward clock also resets");

    history.reset();
    const auto low=std::numeric_limits<int64_t>::min();
    const auto high=std::numeric_limits<int64_t>::max();
    history.receive(20,0,0,false,false,low);
    history.receive(20,0,0,false,false,low+47);
    history.receive(20,0,0,false,false,low+48);
    check(history.entries().size()==2,"dedup boundaries work near INT64_MIN");
    history.receive(20,0,0,false,false,high-48);
    history.receive(20,0,0,false,false,high-1);
    history.receive(20,0,0,false,false,high);
    check(history.entries().size()==4 && history.entries().back().time==high,
          "huge forward jumps and INT64_MAX boundary do not overflow signed arithmetic");
    serial=history.entries().back().serial;
    history.receive(20,0,0,false,false,low);
    check(history.entries().size()==1 && history.entries().back().serial>serial,
          "INT64_MAX to INT64_MIN resets with monotonic identity");

    const char* borrowed=history.entries().front().text.c_str();
    const std::string expected=borrowed;
    history.receiveText(21,borrowed,0,0,false,false,low);
    check(history.entries().back().text==expected,"receiveText safely copies an existing history string");
    history.receiveText(22,history.entries().back().text.c_str(),0,0,false,false,low+1);
    history.receiveText(23,history.entries().back().text.c_str(),0,0,false,false,low);
    check(history.entries().size()==1 && history.entries().back().text==expected,
          "aliased callback text is owned before backward-clock reset destroys its source");
}

int main(int argc,char** argv) {
    if(argc!=2) return 2;
    catalog(argv[1]);
    ownershipAndLocations();
    deduplication();
    resetAndBounds();
    std::printf("RESULT: %d/%d passed\n",checks-failures,checks);
    return failures ? 1 : 0;
}

#include "messages.h"
#include <utility>

const char* messageText(int id) {
    static const char* const texts[]={
        nullptr,
        "More residential zones needed.",
        "More commercial zones needed.",
        "More industrial zones needed.",
        "More roads required.",
        "Inadequate rail system.",
        "Build a Power Plant.",
        "Residents demand a Stadium.",
        "Industry requires a Sea Port.",
        "Commerce requires an Airport.",
        "Pollution very high.",
        "Crime very high.",
        "Frequent traffic jams reported.",
        "Citizens demand a Fire Department.",
        "Citizens demand a Police Department.",
        "Blackouts reported. Check power map.",
        "Citizens upset. The tax rate is too high.",
        "Roads deteriorating, due to lack of funds.",
        "Fire departments need funding.",
        "Police departments need funding.",
        "Fire reported !",
        "A Monster has been sighted !!",
        "Tornado reported !!",
        "Major earthquake reported !!!",
        "A plane has crashed !",
        "Shipwreck reported !",
        "A train crashed !",
        "A helicopter crashed !",
        "Unemployment rate is high.",
        "YOUR CITY HAS GONE BROKE!",
        "Firebombing reported !",
        "Need more parks.",
        "Explosion detected !",
        "Insufficient funds to build that.",
        "Area must be bulldozed first.",
        "Population has reached 2,000.",
        "Population has reached 10,000.",
        "Population has reached 50,000.",
        "Population has reached 100,000.",
        "Population has reached 500,000.",
        "Brownouts, build another Power Plant.",
        "Heavy Traffic reported.",
        "Flooding reported !!",
        "A Nuclear Meltdown has occurred !!!",
        "They're rioting in the streets !!",
        "Start a New City",
        "Restore a Saved City",
        "YOU'RE A WINNER!",
        "IMPEACHMENT NOTICE!",
        "About Micropolis",
        "DULLSVILLE, USA  1900",
        "SAN FRANCISCO, CA.  1906",
        "HAMBURG, GERMANY  1944",
        "BERN, SWITZERLAND  1965",
        "TOKYO, JAPAN  1957",
        "DETROIT, MI.  1972",
        "BOSTON, MA.  2010",
        "RIO DE JANEIRO, BRAZIL  2047"
    };
    static_assert(sizeof(texts)/sizeof(texts[0])==58);
    return id>0 && id<58 ? texts[id] : nullptr;
}

bool CityMessage::hasLocation() const {
    return x>=0 && x<120 && y>=0 && y<100;
}

void MessageHistory::receive(int id,int x,int y,bool picture,bool important,int64_t time) {
    receiveText(id,messageText(id),x,y,picture,important,time);
}

void MessageHistory::receiveText(int id,const char* text,int x,int y,bool picture,bool important,int64_t time) {
    CityMessage incoming{id,x,y,picture,important,
        text ? std::string(text) : "Unknown simulation message ("+std::to_string(id)+")",0,time};
    if(hasTime_ && time<lastTime_) reset();
    lastTime_=time;
    hasTime_=true;
    for(auto it=messages_.rbegin();it!=messages_.rend();++it) {
        const uint64_t elapsed=static_cast<uint64_t>(time)-static_cast<uint64_t>(it->time);
        if(elapsed>=48) break;
        if(it->id==incoming.id && it->text==incoming.text &&
           it->x==x && it->y==y && it->picture==picture && it->important==important) return;
    }
    incoming.serial=++serial_;
    messages_.push_back(std::move(incoming));
    if(messages_.size()>capacity) messages_.pop_front();
    ++revision_;
}

void MessageHistory::reset() {
    messages_.clear();
    hasTime_=false;
    ++revision_;
}

const std::deque<CityMessage>& MessageHistory::entries() const {
    return messages_;
}

uint64_t MessageHistory::revision() const {
    return revision_;
}

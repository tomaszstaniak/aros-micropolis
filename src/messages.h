#ifndef MICROPOLIS_MESSAGES_H
#define MICROPOLIS_MESSAGES_H

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>

struct CityMessage {
    int id,x,y;
    bool picture,important;
    std::string text;
    uint64_t serial;
    int64_t time;
    bool hasLocation() const;
};

const char* messageText(int id);

class MessageHistory {
public:
    static constexpr size_t capacity=64;
    void receive(int id,int x,int y,bool picture,bool important,int64_t time);
    void receiveText(int id,const char* text,int x,int y,bool picture,bool important,int64_t time);
    void reset();
    const std::deque<CityMessage>& entries() const;
    uint64_t revision() const;

private:
    std::deque<CityMessage> messages_;
    uint64_t serial_=0,revision_=0;
    int64_t lastTime_=0;
    bool hasTime_=false;
};

#endif

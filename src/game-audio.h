#ifndef MICROPOLIS_GAME_AUDIO_H
#define MICROPOLIS_GAME_AUDIO_H
#include "audio-wave.h"
struct MsgPort;
struct AHIRequest;
class GameAudio {
public:
 GameAudio() = default;
 ~GameAudio() { close(); }
 GameAudio(const GameAudio&) = delete;
 GameAudio& operator=(const GameAudio&) = delete;
 bool open(const char* directory);
 void close();
 bool play(const char* name);
 void poll();
 bool available() const { return openedVoices_ != 0; }
 bool busy() const;
 unsigned loaded() const { return loaded_; }
 unsigned completed() const { return completed_; }
 unsigned errors() const { return errors_; }
private:
 static constexpr unsigned effectCount=11;
 static constexpr unsigned voiceCount=4;
 AudioWave effects_[effectCount];
 MsgPort* ports_[voiceCount]={};
 AHIRequest* requests_[voiceCount]={};
 bool opened_[voiceCount]={}, pending_[voiceCount]={};
 unsigned openedVoices_=0, loaded_=0, completed_=0, errors_=0;
};
#endif

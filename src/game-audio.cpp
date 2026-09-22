#include "game-audio.h"
#include <devices/ahi.h>
#include <proto/exec.h>
#include <cstdio>
#include <cstring>
#include <new>
#include <string>
static const char* const names[]={"ExplosionHigh","ExplosionLow","FogHornLow","HeavyTraffic","HonkHonkHigh","HonkHonkLow","HonkHonkMed","Monster","Siren","Sorry","UhUh"};
bool GameAudio::open(const char* directory) {
 close(); completed_=errors_=0;
 if(!directory) return false;
 try {
  for(unsigned i=0;i<effectCount;++i) {
   const std::string path=std::string(directory)+"/"+names[i]+".wav";
   FILE* f=std::fopen(path.c_str(),"rb");
   if(!f) continue;
   unsigned char* bytes=nullptr;
   if(std::fseek(f,0,SEEK_END)==0) {
    const long length=std::ftell(f);
    if(length>0 && length<=4*1024*1024 && std::fseek(f,0,SEEK_SET)==0) {
     bytes=new(std::nothrow) unsigned char[length];
     if(bytes && std::fread(bytes,1,length,f)==size_t(length)) {
      // Close before decoding, which can allocate and throw.
      std::fclose(f); f=nullptr;
      try { if(decodeAudioWave(bytes,length,effects_[i])) ++loaded_; }
      catch(...) { delete[] bytes; throw; }
     }
    }
   }
   if(f) std::fclose(f);
   delete[] bytes;
  }
 } catch(...) { close(); return false; }
 if(!loaded_) { close(); return false; }
 for(unsigned voice=0;voice<voiceCount;++voice) {
  ports_[voice]=CreateMsgPort();
  if(!ports_[voice]) continue;
  requests_[voice]=static_cast<AHIRequest*>(CreateIORequest(ports_[voice],sizeof(AHIRequest)));
  if(!requests_[voice]) { DeleteMsgPort(ports_[voice]);ports_[voice]=nullptr;continue; }
  requests_[voice]->ahir_Version=4;
  opened_[voice]=OpenDevice(AHINAME,AHI_DEFAULT_UNIT,
      reinterpret_cast<IORequest*>(requests_[voice]),0)==0;
  if(opened_[voice]) {
   ++openedVoices_;
  } else {
   DeleteIORequest(reinterpret_cast<IORequest*>(requests_[voice]));
   requests_[voice]=nullptr;
   DeleteMsgPort(ports_[voice]);ports_[voice]=nullptr;
  }
 }
 if(!openedVoices_) close();
 return openedVoices_ != 0;
}
void GameAudio::poll() {
 for(unsigned voice=0;voice<voiceCount;++voice) {
  if(pending_[voice] && CheckIO(reinterpret_cast<IORequest*>(requests_[voice]))) {
   WaitIO(reinterpret_cast<IORequest*>(requests_[voice]));
   pending_[voice]=false;
   if(requests_[voice]->ahir_Std.io_Error) ++errors_; else ++completed_;
  }
 }
}
bool GameAudio::busy() const {
 for(bool pending:pending_) if(pending) return true;
 return false;
}
bool GameAudio::play(const char* name) {
 poll();
 // A bounded voice pool lets disaster bursts overlap without delaying the
 // simulation or accumulating stale effects in a queue.
 if(!openedVoices_ || !name) return false;
 unsigned index=0;
 while(index<effectCount && std::strcmp(names[index],name)) ++index;
 if(index==effectCount || effects_[index].samples.empty()) return false;
 unsigned voice=0;
 while(voice<voiceCount && (!opened_[voice] || pending_[voice])) ++voice;
 if(voice==voiceCount) return false;
 AudioWave& wave=effects_[index];
 AHIRequest* request=requests_[voice];
 request->ahir_Std.io_Command=CMD_WRITE;
 request->ahir_Std.io_Flags=0;
 request->ahir_Std.io_Error=0;
 request->ahir_Std.io_Offset=0;
 request->ahir_Std.io_Data=wave.samples.data();
 request->ahir_Std.io_Length=wave.samples.size()*sizeof(int16_t);
 request->ahir_Type=AHIST_M16S;
 request->ahir_Frequency=wave.rate;
 request->ahir_Volume=0x10000;
 request->ahir_Position=0x8000;
 request->ahir_Link=nullptr;
 SendIO(reinterpret_cast<IORequest*>(request));
 pending_[voice]=true;
 return true;
}
void GameAudio::close() {
 for(unsigned voice=0;voice<voiceCount;++voice) {
  if(pending_[voice]) {
   if(!CheckIO(reinterpret_cast<IORequest*>(requests_[voice])))
    AbortIO(reinterpret_cast<IORequest*>(requests_[voice]));
   WaitIO(reinterpret_cast<IORequest*>(requests_[voice]));
   pending_[voice]=false;
  }
  if(opened_[voice]) CloseDevice(reinterpret_cast<IORequest*>(requests_[voice]));
  opened_[voice]=false;
  if(requests_[voice]) DeleteIORequest(reinterpret_cast<IORequest*>(requests_[voice]));
  requests_[voice]=nullptr;
  if(ports_[voice]) DeleteMsgPort(ports_[voice]);
  ports_[voice]=nullptr;
 }
 openedVoices_=0;
 // The device must release all references before storage is reclaimed.
 for(auto& wave:effects_) wave=AudioWave();
 loaded_=0;
}

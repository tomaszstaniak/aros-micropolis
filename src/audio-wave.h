#ifndef MICROPOLIS_AUDIO_WAVE_H
#define MICROPOLIS_AUDIO_WAVE_H
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>
struct AudioWave { unsigned rate = 0; std::vector<int16_t> samples; };
inline unsigned audioLE16(const unsigned char* p) { return p[0] | unsigned(p[1])<<8; }
inline uint32_t audioLE32(const unsigned char* p) { return audioLE16(p) | uint32_t(audioLE16(p+2))<<16; }
// Only the staging contract (signed 16-bit mono PCM WAV) is accepted. Limits
// keep damaged user-replaced resources from allocating unbounded memory.
inline bool decodeAudioWave(const unsigned char* p, size_t size, AudioWave& out) {
 out = AudioWave();
 if (!p || size<12 || size>4*1024*1024 || std::memcmp(p,"RIFF",4) || std::memcmp(p+8,"WAVE",4)) return false;
 const size_t end=size_t(audioLE32(p+4))+8;
 if (end<12 || end>size) return false;
 bool format=false; const unsigned char* data=nullptr; size_t bytes=0; unsigned rate=0;
 for (size_t pos=12;pos<end;) {
  if(end-pos<8) return false;
  const size_t n=audioLE32(p+pos+4); pos+=8;
  if(n>end-pos || (n&1)>end-pos-n) return false;
  if(!std::memcmp(p+pos-8,"fmt ",4)) {
   if(format || n<16 || audioLE16(p+pos)!=1 || audioLE16(p+pos+2)!=1 || audioLE16(p+pos+12)!=2 || audioLE16(p+pos+14)!=16) return false;
   rate=audioLE32(p+pos+4);
   if(rate<8000 || rate>48000 || audioLE32(p+pos+8)!=rate*2) return false;
   format=true;
  } else if(!std::memcmp(p+pos-8,"data",4)) {
   if(data || !n || n%2) return false;
   data=p+pos; bytes=n;
  }
  pos+=n+(n&1);
 }
 if(!format || !data) return false;
 out.rate=rate; out.samples.resize(bytes/2);
 for(size_t i=0;i<out.samples.size();++i) {
  const unsigned value=audioLE16(data+2*i);
  out.samples[i]=int16_t(value<32768 ? int(value) : int(value)-65536);
 }
 return true;
}
#endif

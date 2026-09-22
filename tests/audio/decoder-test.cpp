#include "audio-wave.h"
#include <cassert>
#include <cstdio>
#include <vector>
static void put(std::vector<unsigned char>& v, unsigned x) { for(int i=0;i<4;++i)v.push_back((x>>(8*i))&255); }
static std::vector<unsigned char> wave() {
 std::vector<unsigned char> v={'R','I','F','F'}; put(v,42); v.insert(v.end(),{'W','A','V','E','f','m','t',' '}); put(v,16);
 v.insert(v.end(),{1,0,1,0}); put(v,22050); put(v,44100); v.insert(v.end(),{2,0,16,0,'d','a','t','a'}); put(v,6); v.insert(v.end(),{0,128,0,0,255,127}); return v;
}
int main() {
 AudioWave out; auto v=wave(); assert(decodeAudioWave(v.data(),v.size(),out)); assert(out.rate==22050 && out.samples.size()==3); assert(out.samples[0]==-32768 && out.samples[1]==0 && out.samples[2]==32767);
 for(size_t n=0;n<v.size();++n) assert(!decodeAudioWave(v.data(),n,out));
 auto bad=v; bad[20]=3; assert(!decodeAudioWave(bad.data(),bad.size(),out));
 bad=v; bad[22]=2; assert(!decodeAudioWave(bad.data(),bad.size(),out));
 bad=v; bad[34]=8; assert(!decodeAudioWave(bad.data(),bad.size(),out));
 bad=v; bad[40]=255; assert(!decodeAudioWave(bad.data(),bad.size(),out));
 bad=v; bad[24]=bad[25]=bad[26]=bad[27]=0; assert(!decodeAudioWave(bad.data(),bad.size(),out));
 bad=v; bad.insert(bad.begin()+12,{'J','U','N','K',1,0,0,0,42,0}); bad[4]+=10; assert(decodeAudioWave(bad.data(),bad.size(),out));
 assert(out.samples.size()==3); assert(!decodeAudioWave(nullptr,0,out)); assert(out.samples.empty());
 puts("audio decoder: valid samples, all truncations, odd chunks and malformed formats pass");
}

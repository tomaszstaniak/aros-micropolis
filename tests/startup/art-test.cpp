#include "bmp.h"
#include <cstdio>
#include <fstream>
#include <iterator>
int main(int argc,char **argv){
    if(argc!=3)return 2;
    BmpImage image;std::string error;
    if(!loadBmp8(argv[1],image,error)){fprintf(stderr,"%s: %s\n",argv[1],error.c_str());return 1;}
    std::ifstream f(argv[2],std::ios::binary);
    std::vector<unsigned char> rgb((std::istreambuf_iterator<char>(f)),{});
    if(rgb.size()!=image.pixels.size()*3)return 1;
    for(size_t i=0;i<image.pixels.size();++i){
        uint32_t expected=0xff000000u|(rgb[i*3]<<16)|(rgb[i*3+1]<<8)|rgb[i*3+2];
        if(image.pixels[i]!=expected){fprintf(stderr,"pixel mismatch %zu\n",i);return 1;}
    }
    return 0;
}

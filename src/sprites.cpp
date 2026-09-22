#include "sprites.h"
#include <png.h>
#include <algorithm>
#include <cstdio>
#include <utility>

namespace {
void readPng(png_structp png,png_bytep bytes,png_size_t count) {
    FILE *file=static_cast<FILE *>(png_get_io_ptr(png));
    if(fread(bytes,1,count,file)!=count)png_error(png,"Truncated sprite PNG");
}
std::string join(const std::string &dir,const std::string &name) {
    return dir.empty()?name:dir+((dir.back()=='/' || dir.back()==':')?"":"/")+name;
}
}

bool loadSpritePng(const std::string &path,BmpImage &out,std::string &error) {
    FILE *file=fopen(path.c_str(),"rb");
    if(!file){error="Cannot open "+path;return false;}
    png_structp png=png_create_read_struct(PNG_LIBPNG_VER_STRING,nullptr,nullptr,nullptr);
    png_infop info=png?png_create_info_struct(png):nullptr;
    if(!png || !info){if(png)png_destroy_read_struct(&png,nullptr,nullptr);fclose(file);error="PNG allocation failed";return false;}
    // libpng owns decoded rows until all fallible C calls have completed. No
    // C++ objects are constructed across its longjmp error boundary.
    if(setjmp(png_jmpbuf(png))){png_destroy_read_struct(&png,&info,nullptr);fclose(file);error="Invalid sprite PNG: "+path;return false;}
    png_set_read_fn(png,file,readPng); // SDK png_nostdio has no png_init_io.
    png_set_user_limits(png,64,64);
    png_read_png(png,info,PNG_TRANSFORM_IDENTITY,nullptr);
    const int width=png_get_image_width(png,info),height=png_get_image_height(png,info);
    if(width<1 || height<1 || png_get_bit_depth(png,info)!=8 ||
       png_get_color_type(png,info)!=PNG_COLOR_TYPE_RGBA ||
       png_get_rowbytes(png,info)!=(png_size_t)width*4) {
        png_destroy_read_struct(&png,&info,nullptr);fclose(file);
        error="Expected original RGBA8 sprite: "+path;return false;
    }
    png_bytepp rows=png_get_rows(png,info);
    BmpImage image;image.width=width;image.height=height;
    image.pixels.resize((size_t)width*height);
    for(int y=0;y<height;y++)for(int x=0;x<width;x++) {
        const auto *p=rows[y]+4*x;
        image.pixels[(size_t)y*width+x]=(uint32_t(p[3])<<24)|(uint32_t(p[0])<<16)|(uint32_t(p[1])<<8)|p[2];
    }
    png_destroy_read_struct(&png,&info,nullptr);fclose(file);
    out=std::move(image);error.clear();return true;
}

bool SpriteArt::load(const std::string &directory,std::string &error) {
    // Original content/micropolis/images (not the incomplete plane atlas).
    static const int count[]={0,5,8,11,8,16,3,6,4};
    SpriteArt next;
    for(int type=1;type<=8;type++)for(int frame=0;frame<count[type];frame++) {
        char name[40];snprintf(name,sizeof name,"sprite_%d_%d.png",type,frame);
        BmpImage image;if(!loadSpritePng(join(directory,name),image,error))return false;
        const int size=(type==1 || type==2 || type==8)?32:48;
        if(image.width!=size || image.height!=size){error="Wrong sprite dimensions: "+join(directory,name);return false;}
        next.frames[type].push_back(std::move(image));
    }
    frames.swap(next.frames);return true;
}

size_t renderSprites(const SpriteArt &art,const SimSprite *sprites,
                     std::vector<uint32_t> &pixels,int width,int height,int camX,int camY) {
    if(width<=0 || height<=0 || (uint64_t)width*height>pixels.size())return 0;
    size_t painted=0;
    for(auto *s=sprites;s;s=s->next) {
        if(s->type<1 || s->type>8 || s->frame<=0 || (size_t)s->frame>art.frames[s->type].size())continue;
        const auto &frame=art.frames[s->type][s->frame-1];
        if(frame.width<=0 || frame.height<=0 || (uint64_t)frame.width*frame.height>frame.pixels.size())continue;
        // Classic DrawSprite: x + x_offset - (tile_x << 4), likewise y.
        // xHot/yHot are simulation hotspots, not the art's drawing origin.
        int64_t x=(int64_t)s->x+s->xOffset-(int64_t)camX*16;
        int64_t y=(int64_t)s->y+s->yOffset-(int64_t)camY*16;
        int64_t left=std::max<int64_t>(0,x),top=std::max<int64_t>(0,y);
        int64_t right=std::min<int64_t>(width,x+frame.width),bottom=std::min<int64_t>(height,y+frame.height);
        for(int64_t dy=top;dy<bottom;dy++)for(int64_t dx=left;dx<right;dx++) {
            uint32_t source=frame.pixels[(size_t)(dy-y)*frame.width+(dx-x)];
            unsigned alpha=source>>24;if(!alpha)continue;
            auto &dest=pixels[(size_t)dy*width+dx];
            if(alpha==255)dest=source;
            else {
                uint32_t result=0xff000000;
                for(int shift=0;shift<=16;shift+=8) {
                    unsigned channel=(((source>>shift)&255)*alpha+((dest>>shift)&255)*(255-alpha)+127)/255;
                    result|=channel<<shift;
                }
                dest=result;
            }
            painted++;
        }
    }
    return painted;
}

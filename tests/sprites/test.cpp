#include "sprites.h"
#include <cstdio>
#include <limits>
int main(int argc,char **argv) {
    if(argc<2)return 20;
    if(argc>2 && !freopen(argv[2],"w",stdout))return 20;
    int passed=0,failed=0;
    auto check=[&](bool yes,const char *s){printf("%s %s\n",yes?"PASS":"FAIL",s);yes?passed++:failed++;};
    SpriteArt art;std::string error;
    check(art.load(argv[1],error),"load all original sprite frames");
    const int counts[]={0,5,8,11,8,16,3,6,4};bool all=true;
    for(int i=1;i<=8;i++)all&=art.frames[i].size()==(size_t)counts[i];
    check(all,"all eight engine types have their complete frame sets");
    SpriteArt synthetic;BmpImage f;f.width=2;f.height=2;
    f.pixels={0xffff0000,0x0000ff00,0x800000ff,0xff00ff00};
    synthetic.frames[1].push_back(f);f.pixels.assign(4,0xffffffff);synthetic.frames[1].push_back(f);
    SimSprite s={};s.type=1;s.frame=1;s.x=16;s.y=32;s.xOffset=1;s.yOffset=1;
    std::vector<uint32_t> fb(16,0xff000000);
    check(renderSprites(synthetic,&s,fb,4,4,1,2)==3 && fb[5]==0xffff0000 && fb[10]==0xff00ff00,
          "world position plus original drawing offset minus camera locates sprite");
    check(fb[6]==0xff000000,"transparent sprite pixel preserves the map");
    check(fb[9]==0xff000080,"partial alpha blends onto opaque map");
    s.frame=2;fb.assign(16,0xff000000);renderSprites(synthetic,&s,fb,4,4,1,2);
    check(fb[5]==0xffffffff,"engine one-based frame selects second art frame");
    s.frame=0;auto before=fb;check(renderSprites(synthetic,&s,fb,4,4,1,2)==0 && fb==before,"inactive frame zero draws nothing");
    s.frame=3;check(renderSprites(synthetic,&s,fb,4,4,1,2)==0 && fb==before,"out-of-range frame cannot read past art");
    s.frame=2;s.type=99;check(renderSprites(synthetic,&s,fb,4,4,1,2)==0,"unknown sprite type is ignored");
    s.type=1;s.x=-2;s.y=-2;fb.assign(16,0xff000000);
    check(renderSprites(synthetic,&s,fb,4,4,0,0)==1 && fb[0]==0xffffffff,"negative position clips top and left");
    s.x=2;s.y=2;fb.assign(16,0xff000000);
    check(renderSprites(synthetic,&s,fb,4,4,0,0)==1 && fb[15]==0xffffffff,"bottom and right clip without writing outside framebuffer");
    s.x=std::numeric_limits<int>::max();s.y=0;
    check(renderSprites(synthetic,&s,fb,4,4,0,0)==0,"far outside sprite and offset do not overflow");
    std::vector<uint32_t> tooSmall(1);
    check(renderSprites(synthetic,&s,tooSmall,4,4,0,0)==0,"invalid framebuffer size is rejected");
    BmpImage retained=f;
    check(!loadSpritePng(std::string(argv[1])+"/missing.png",retained,error) && retained.pixels==f.pixels,"missing art reports failure and preserves existing image");
    check(!art.load(std::string(argv[1])+"/missing",error) && art.frames[3].size()==11,"failed pack load preserves previous complete pack");
    if(all){bool alpha=false,opaque=false;for(auto p:art.frames[3][10].pixels){alpha|=(p>>24)==0;opaque|=(p>>24)==255;}
      check(alpha && opaque,"actual airplane takeoff frame retains transparency and visible pixels");}
    printf("RESULT: %d passed, %d failed\n",passed,failed);return failed?1:0;
}

#include "display.h"
#include <algorithm>
#include <new>
#include <libraries/asl.h>
#include <cybergraphx/cybergraphics.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/asl.h>

int chooseRequestedDisplayMode(Window *parent, RequestedDisplayMode &mode) {
    Library *AslBase=OpenLibrary((CONST_STRPTR)"asl.library",0);
    if (!AslBase) return -1;
    TagItem tags[] = {
        {ASLSM_Window,(IPTR)parent}, {ASLSM_SleepWindow,TRUE},
        {ASLSM_TitleText,(IPTR)"Micropolis - fullscreen mode"},
        {ASLSM_InitialDisplayID,GetVPModeID(&parent->WScreen->ViewPort)},
        {ASLSM_InitialDisplayDepth,24},
        {ASLSM_MinWidth,640},{ASLSM_MinHeight,480},
        {ASLSM_MinDepth,24},{ASLSM_MaxDepth,32},
        {ASLSM_DoWidth,FALSE},{ASLSM_DoHeight,FALSE},
        {ASLSM_DoDepth,TRUE},{ASLSM_DoAutoScroll,FALSE},
        {TAG_DONE,0}
    };
    auto *req=(ScreenModeRequester *)AllocAslRequest(ASL_ScreenModeRequest,tags);
    int result=-1;
    if (req) {
        TagItem none[]={{TAG_DONE,0}};
        result=AslRequest(req,none)?1:0;
        if(result==1) mode={req->sm_DisplayID,(int)req->sm_DisplayWidth,
                           (int)req->sm_DisplayHeight,(int)req->sm_DisplayDepth};
        FreeAslRequest(req);
    }
    CloseLibrary(AslBase);
    return result;
}

GameDisplay::~GameDisplay() {
    if(window) CloseWindow(window);
    if(bitmap) FreeBitMap(bitmap);
    if(screen) {
        if(custom) CloseScreen(screen);
        else UnlockPubScreen(NULL,screen);
    }
}

bool GameDisplay::open(const RequestedDisplayMode *mode,const WindowPlacement *placement) {
    custom=mode!=nullptr;
    if(custom) {
        screen=OpenScreenTags(NULL,SA_DisplayID,mode->id,
            SA_Width,mode->width,SA_Height,mode->height,SA_Depth,mode->depth,
            SA_Type,CUSTOMSCREEN,SA_Title,(IPTR)"Micropolis - F10: return to desktop",
            SA_AutoScroll,FALSE,TAG_DONE);
    } else screen=LockPubScreen(NULL);
    if(!screen) return false;
    const int top=screen->BarHeight+6;
    const int left=156;
    const int width=screen->Width-left-8;
    const int height=screen->Height-top-8;
    if(width<256 || height<336) return false;
    int windowLeft=left,windowTop=top;
    int windowWidth=custom?width:std::min(width,660),windowHeight=custom?height:std::min(height,480);
    if(placement) {
        placement->size(screen->Width,screen->Height,screen->BarHeight+1,256,192,windowWidth,windowHeight);
        placement->place(screen->Width,screen->Height,screen->BarHeight+1,windowWidth,windowHeight,windowLeft,windowTop);
    }
    window=OpenWindowTags(NULL,
        custom?WA_CustomScreen:WA_PubScreen,(IPTR)screen,
        WA_Title,(IPTR)"Micropolis - F10: screen/window; F9: mode",
        WA_Left,windowLeft,WA_Top,windowTop,
        WA_Width,windowWidth,
        WA_Height,windowHeight,
        WA_MinWidth,256,WA_MinHeight,192,
        WA_MaxWidth,screen->Width,WA_MaxHeight,screen->Height-top,
        WA_SizeGadget,TRUE,WA_SizeBBottom,TRUE,
        WA_Activate,TRUE,WA_ReportMouse,TRUE,
        WA_CloseGadget,TRUE,WA_DragBar,TRUE,WA_DepthGadget,TRUE,
        WA_IDCMP,IDCMP_CLOSEWINDOW|IDCMP_RAWKEY|IDCMP_MOUSEBUTTONS|
                 IDCMP_MOUSEMOVE|IDCMP_NEWSIZE|
                 IDCMP_REFRESHWINDOW|IDCMP_INACTIVEWINDOW|IDCMP_MENUPICK,
        TAG_DONE);
    return window && resize();
}

bool GameDisplay::resize() {
    auto next=ViewGeometry::fromInner(window->Width-window->BorderLeft-window->BorderRight,
                                      window->Height-window->BorderTop-window->BorderBottom,tile);
    if(bitmap && next.width==view.width && next.height==view.height && next.tile==view.tile) return true;
    auto *replacement=AllocBitMap(next.width,next.height,32,
        BMF_MINPLANES|BMF_CLEAR|BMF_SPECIALFMT|SHIFT_PIXFMT(PIXFMT_ARGB32),NULL);
    if(!replacement) return false;
    std::vector<uint32_t> nextPixels,nextWorld;
    try {
        nextPixels.resize((size_t)next.width*next.height);
        nextWorld.resize((size_t)next.worldWidth()*next.worldHeight());
    }
    catch(const std::bad_alloc &) { FreeBitMap(replacement);return false; }
    // Commit only after all allocations succeed; old resources remain on failure.
    if(bitmap) FreeBitMap(bitmap);
    bitmap=replacement;pixels.swap(nextPixels);world.swap(nextWorld);view=next;
    InitRastPort(&raster);raster.BitMap=bitmap;
    return true;
}

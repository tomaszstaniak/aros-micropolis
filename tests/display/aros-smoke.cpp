// On-target resource test. Results go to a file, never an ambiguous Shell log.
#include "../../src/display.h"
#include <proto/exec.h>
#include <proto/graphics.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <cstdio>
static void drain(Window *w) {
    IntuiMessage *m;
    while ((m=(IntuiMessage *)GetMsg(w->UserPort))) ReplyMsg((Message *)m);
}
int main() {
    FILE *f=fopen("RAM:micropolis-display-report.txt","w"); if(!f) return 20;
    int failed=0;
    auto check=[&](bool ok,const char *label){fprintf(f,"%s %s\n",ok?"PASS":"FAIL",label);failed+=!ok;};
    GameDisplay desktop;
    check(desktop.open(nullptr),"open desktop display");
    if(!desktop.window) {fclose(f);return 20;}
    fprintf(f,"ABIv11 one; screen=%dx%d depth=%lu; initial view=%dx%d\n",
        desktop.screen->Width,desktop.screen->Height,
        (unsigned long)GetBitMapAttr(desktop.screen->RastPort.BitMap,BMA_DEPTH),
        desktop.view.width,desktop.view.height);
    SizeWindow(desktop.window,160,128);Delay(20);drain(desktop.window);
    check(desktop.resize(),"resize allocation succeeds");
    fprintf(f,"resized view=%dx%d\n",desktop.view.width,desktop.view.height);
    check(desktop.view.width>640 && desktop.view.height>432,"larger window exposes more tiles");
    auto *oldWindow=desktop.window;auto *oldBitmap=desktop.bitmap;
    {
        RequestedDisplayMode bad={0xfffffffeUL,1024,768,24};
        GameDisplay candidate;
        check(!candidate.open(&bad),"invalid mode rejected");
    }
    check(desktop.window==oldWindow && desktop.bitmap==oldBitmap,
          "failed candidate leaves current resources intact");
    RequestedDisplayMode mode={GetVPModeID(&desktop.screen->ViewPort),
        desktop.screen->Width,desktop.screen->Height,
        (int)GetBitMapAttr(desktop.screen->RastPort.BitMap,BMA_DEPTH)};
    for(int i=0;i<3;++i) {
        GameDisplay candidate;
        check(candidate.open(&mode),"custom-screen resources open");
        if(candidate.window) {
            fprintf(f,"custom view=%dx%d\n",candidate.view.width,candidate.view.height);
            Delay(5);drain(candidate.window);
        }
    }
    ScreenToFront(desktop.screen);ActivateWindow(desktop.window);
    check(desktop.resize(),"desktop remains usable after three custom-screen lifetimes");
    fprintf(f,"RESULT: %d failures\n",failed);fclose(f);
    return failed?20:0;
}

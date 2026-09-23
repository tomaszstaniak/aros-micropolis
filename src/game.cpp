/* Micropolis for AROS: native Intuition/CyberGraphics frontend.
 * Behaviour and design notes: docs/DESIGN.md.
 * Display resources live in display.cpp; simulation survives display changes.
 */

#include "bmp.h"
#include "sprites.h"
#include "display.h"
#include "city-windows.h"
#include "message-window.h"
#include "graph-window.h"
#include "overview-window.h"
#include "game-menu.h"
#include "startup-window.h"
#include "simulation-control.h"
#include "simulation-timer.h"
#include "launch-directory.h"
#include "classic-tools.h"
#include "classic-tool-ui.h"
#include "game-audio.h"
#include "game-options.h"
#include "disaster-presentation.h"
#include "demand-model.h"
#include "city-render.h"
#include "notice-preview.h"
#include "chalk-overlay.h"
#include "unsaved-city.h"
#include "window-placement.h"
#include <memory>
#include "save-replace.h"
#include "micropolis.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/intuitionbase.h>
#include <graphics/gfx.h>
/* AROS SDK spells the cybergraphics headers differently than AmigaOS:
 * constants live in <cybergraphx/...> (note the spelling) and protos in
 * <proto/...> which pulls <clib/..._protos.h>. */
#include <cybergraphx/cybergraphics.h>
#include <devices/inputevent.h>
#include <devices/rawkeycodes.h>

#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/graphics.h>
#include <proto/cybergraphics.h>
#include <proto/dos.h>
#include <proto/asl.h>
#include <proto/icon.h>
#include <workbench/workbench.h>
#include <workbench/icon.h>

// NewMouse wheel codes; AROS delivers them as IDCMP_RAWKEY. This SDK's
// rawkeycodes.h does not name them.
#ifndef RAWKEY_NM_WHEEL_UP
#define RAWKEY_NM_WHEEL_UP 0x7a
#define RAWKEY_NM_WHEEL_DOWN 0x7b
#endif

#define TILE_SIZE 16
static ViewGeometry view;
#define VIEW_TILES_W (view.columns())
#define VIEW_TILES_H (view.rows())
#define VIEW_W (view.width)
#define VIEW_H (view.height)

/* Tile-sheet geometry for the classic tiles.bmp: 512x480 = 32 columns. */
#define SHEET_COLS 32
#define TILE_COUNT 960

static BmpImage sheet;

struct MessagePreviewContext {Micropolis *city;const BmpImage *tiles;const SpriteArt *sprites;};
static void renderMessagePreview(void *opaque,const CityMessage &message,Window *window,int x,int y) {
    auto &c=*(MessagePreviewContext*)opaque;int cx,cy;noticePreviewCamera(message.x,message.y,8,8,cx,cy);
    std::vector<uint32_t> crop;renderCityTiles(*c.city,*c.tiles,crop,128,128,cx,cy);
    renderSprites(*c.sprites,c.city->spriteList,crop,128,128,cx,cy);
    WritePixelArray(crop.data(),0,0,128*4,window->RPort,window->BorderLeft+x,window->BorderTop+y,128,128,RECTFMT_BGRA32);
}

/* ------------------------------------------------------------------ */

class FrontendCallback;

static void updateTitle(struct Window *win, Micropolis *m,
                        FrontendCallback *cb, bool running,
                        const char *toolName, const char *feedback);

static void buildAt(EditingTool tool, int mapX, int mapY,
                    struct Window *win, Micropolis *micropolis,
                    FrontendCallback *cb, bool running,
                    const char **toolNames, char *feedback);

/* Toolbar model: the 18 palette slots of the classic Micropolis editor
 * (weditor.tcl order), mapped explicitly to EditingTool - the slot order
 * is NOT the enum order. Prices mirror the engine's gCostOf
 * (tool.cpp:216); the classic UI showed Park $20 / Network $1000, which
 * is recorded as a documented difference in docs/DESIGN.md.
 * Chalk and Eraser are classic annotation tools with no engine
 * counterpart: the frontend owns their overlay (chalk-overlay.h). */

struct ToolSlot {
    const char *label;      /* short label drawn in the cell */
    const char *name;       /* name for feedback */
    EditingTool tool;
    int foot;               /* footprint W=H in tiles */
    int price;              /* engine price, gCostOf */
    bool available;
    int col, row;           /* cell position (the weditor.tcl layout) */
};

/* Engine mapping for the classic editor's 18 tools. Pixel geometry and
 * original artwork are separate from the engine enum (classic-tools.h). */
static const ToolSlot TOOLBAR_SLOTS[] = {
    { "RES",  "residential",    TOOL_RESIDENTIAL,  3, 100,   true , 0, 0 },
    { "COM",  "commercial",     TOOL_COMMERCIAL,   3, 100,   true , 1, 0 },
    { "IND",  "industrial",     TOOL_INDUSTRIAL,   3, 100,   true , 2, 0 },
    { "FIRE", "firestation",    TOOL_FIRESTATION,  3, 500,   true , 0, 1 },
    { "QRY",  "query",          TOOL_QUERY,        1, 0,     true , 1, 1 },
    { "POL",  "policestation",  TOOL_POLICESTATION,3, 500,   true , 2, 1 },
    { "WIRE", "wire",           TOOL_WIRE,         1, 5,     true , 0, 2 },
    { "DOZR", "bulldozer",      TOOL_BULLDOZER,    1, 1,     true , 1, 2 },
    { "RAIL", "railroad",       TOOL_RAILROAD,     1, 20,    true , 0, 3 },
    { "ROAD", "road",           TOOL_ROAD,         1, 10,    true , 1, 3 },
    { "CHLK", "chalk",          TOOL_QUERY,        0, 0,     true , 0, 4 },
    { "ERSR", "eraser",         TOOL_QUERY,        0, 0,     true , 1, 4 },
    { "STAD", "stadium",        TOOL_STADIUM,      4, 5000,  true , 0, 5 },
    { "PARK", "park",           TOOL_PARK,         1, 10,    true , 1, 5 },
    { "SEAP", "seaport",        TOOL_SEAPORT,      4, 3000,  true , 2, 5 },
    { "COAL", "coalpower",      TOOL_COALPOWER,    4, 3000,  true , 0, 6 },
    { "NUC",  "nuclearpower",   TOOL_NUCLEARPOWER, 4, 5000,  true , 2, 6 },
    { "AIRP", "airport",        TOOL_AIRPORT,      6, 10000, true , 1, 7 },
};
static const int TOOLBAR_COUNT = sizeof(TOOLBAR_SLOTS) / sizeof(TOOLBAR_SLOTS[0]);
static const int SLOT_CHALK = 10, SLOT_ERASER = 11;
enum { ANNOTATE_NONE, ANNOTATE_CHALK, ANNOTATE_ERASER };

/* Persistent save/load requirements: cancelling the
 * requester changes nothing at all, a save error is visible, and a saved
 * city must restore after an AROS reboot - so the medium is the
 * installed disk (SYS:), never RAM: or the vvfat drive. */
#define SAVE_DIR "SYS:Micropolis"

static bool askFile(Window *parent, bool save, char *path, int pathSize)
{
    struct Library *AslBase = OpenLibrary((CONST_STRPTR)"asl.library", 0L);
    if (!AslBase) {
        return false;
    }
    struct TagItem empty[] = { { TAG_DONE, 0 } };
    APTR req = AllocAslRequest(ASL_FileRequest, empty);
    if (!req) {
        CloseLibrary(AslBase);
        return false;
    }
    struct TagItem tags[] = {
        { ASLFR_TitleText,    (IPTR)(save ? "Micropolis - save city"
                                          : "Micropolis - load city") },
        { ASLFR_InitialDrawer, (IPTR)SAVE_DIR },
        { ASLFR_DoSaveMode,   (IPTR)(save ? TRUE : FALSE) },
        { ASLFR_Window, (IPTR)parent },
        { ASLFR_SleepWindow, TRUE },
        { TAG_DONE, 0 }
    };
    BOOL ok = AslRequest(req, tags);
    struct FileRequester *fr = (struct FileRequester *)req;
    if (ok && fr->fr_File && fr->fr_File[0]) {
        /* cancel or empty name: change nothing at all. fr_Drawer on
         * this build has no trailing separator - join properly. */
        size_t dl = strlen(fr->fr_Drawer);
        char last = dl ? fr->fr_Drawer[dl - 1] : '\0';
        int n = snprintf(path, pathSize, "%s%s%s", fr->fr_Drawer,
                 (last == ':' || last == '/') ? "" : "/", fr->fr_File);
        if (n < 0 || n >= pathSize) {
            ok = FALSE;   /* never operate on a truncated path */
        }
    } else {
        ok = FALSE;
    }
    FreeAslRequest(req);
    CloseLibrary(AslBase);
    return ok != FALSE;
}

static bool saveCitySafely(Micropolis &city,const char *path,char *feedback,size_t feedbackSize) {
    char tmp[300],aux[300];
    if(!makeAuxNames(path,tmp,sizeof tmp,aux,sizeof aux)) {
        snprintf(feedback,feedbackSize,"SAVE FAILED: cannot allocate recovery filenames for %s",path);return false;
    }
    if(!city.saveFile(tmp)) {
        remove(tmp);snprintf(feedback,feedbackSize,"SAVE FAILED: %s (previous save kept)",path);return false;
    }
    FILE *ck=fopen(tmp,"rb");long sz=ck?(fseek(ck,0,SEEK_END),ftell(ck)):-1;if(ck)fclose(ck);
    if(sz<=0){remove(tmp);snprintf(feedback,feedbackSize,"SAVE FAILED: %s (previous save kept)",path);return false;}
    return replaceWithNewFile(path,tmp,feedback,feedbackSize);
}

/* Scratch for the unsaved-changes fingerprint. RAM: is fine here: it is
 * rewritten and deleted immediately, never a place the player's city lives. */
static uint64_t currentFingerprint(Micropolis &city)
{
    return cityFingerprint(city, "RAM:micropolis-fingerprint.tmp");
}

/* A freshly generated map exists nowhere else, so it starts out unsaved;
 * a loaded file or a built-in scenario can be recreated. */
static uint64_t startingFingerprint(Micropolis &city, const std::string &fileName)
{
    return fileName.empty() && city.scenario == SC_NONE ? 0 : currentFingerprint(city);
}

/* Saved cities get a Workbench project icon whose default tool is this
 * program, so a double-click reopens the city. An existing icon is the
 * player's and is never replaced. Failure only costs the icon. */
static void writeCityIcon(const char *cityPath)
{
    std::string iconPath = std::string(cityPath) + ".info";
    if (BPTR existing = Lock((CONST_STRPTR)iconPath.c_str(), SHARED_LOCK)) {
        UnLock(existing);
        return;
    }
    struct Library *IconBase = OpenLibrary((CONST_STRPTR)"icon.library", 44);
    if (!IconBase) return;
    char tool[512] = "";
    if (BPTR home = Lock((CONST_STRPTR)"PROGDIR:", SHARED_LOCK)) {
        if (!NameFromLock(home, (STRPTR)tool, sizeof tool) ||
            !AddPart((STRPTR)tool, (CONST_STRPTR)"Micropolis", sizeof tool)) tool[0] = 0;
        UnLock(home);
    }
    struct DiskObject *icon = tool[0] ? GetDiskObject((CONST_STRPTR)"PROGDIR:icons/def_city") : nullptr;
    if (icon) {
        STRPTR previousTool = icon->do_DefaultTool;
        icon->do_DefaultTool = (STRPTR)tool;
        icon->do_Type = WBPROJECT;
        icon->do_CurrentX = icon->do_CurrentY = NO_ICON_POSITION;
        PutDiskObject((CONST_STRPTR)cityPath, icon);
        icon->do_DefaultTool = previousTool;
        FreeDiskObject(icon);
    }
    CloseLibrary(IconBase);
}
#define TB_LABELH 26
#define TB_W 132
#define TB_GRID_H 408
#define TB_H TB_GRID_H

static struct Window *tbWin = NULL;
static int tbSel = 9;   /* initial slot: ROAD */

/* ------------------------------------------------------------------ */

/* Original normal/selected art, dimensions and placement. Unimplemented
 * annotation tools remain visibly disabled; prices come from the engine. */
static void renderToolbar(void)
{
    if (!tbWin) {
        return;
    }
    struct RastPort *rp = tbWin->RPort;
    int bl = tbWin->BorderLeft;
    int bt = tbWin->BorderTop;
    SetDrMd(rp,JAM1);
    SetAPen(rp,0);
    RectFill(rp,bl,bt,bl+TB_W-1,bt+TB_H-1);

    /* Selected tool name + engine price at the top (weditor.tcl's two
     * cost labels). */
    {
        const ToolSlot *sel = &TOOLBAR_SLOTS[tbSel];
        char line[64];
        /* Text overwrites nothing - clear the label strip first,
         * otherwise a shorter name leaves the old tail behind. */
        SetAPen(rp, 0);
        RectFill(rp, bl, bt, bl + TB_W - 1, bt + TB_LABELH - 1);
        SetAPen(rp, 1);
        snprintf(line, sizeof(line), "%s", sel->name);
        Move(rp, bl + 4, bt + 9);
        Text(rp, line, strlen(line));
        if (sel->foot == 0) snprintf(line, sizeof(line), "free");
        else snprintf(line, sizeof(line), "$%d", sel->price);
        Move(rp, bl + 4, bt + 20);
        Text(rp, line, strlen(line));
    }

    for (int i = 0; i < TOOLBAR_COUNT; i++) {
        const ToolSlot *slot = &TOOLBAR_SLOTS[i];
        const auto &rect=classicTools[i];
        drawClassicTool(rp,i,i==tbSel,slot->available,bl+rect.x,bt+rect.y);
    }
    SetAPen(rp, 0);
    RectFill(rp, bl, bt + TB_LABELH, bl + TB_W - 1, bt + 55);
    SetAPen(rp, 1);
    Move(rp, bl + 4, bt + TB_LABELH + 12);
    const char *label = "Screen (F10)";
    Text(rp, label, strlen(label));
    Move(rp, bl + 4, bt + TB_LABELH + 27);
    const char *menuLabel="Pie: Shift+LMB";
    Text(rp,menuLabel,strlen(menuLabel));
}

/* The ONE place tool selection happens: palette clicks and digit keys
 * both end here, so the selection, the footprint and the feedback can
 * never disagree. */
static EditingTool currentTool(void)
{
    return TOOLBAR_SLOTS[tbSel].tool;
}

static int currentAnnotation(void)
{
    return tbSel == SLOT_CHALK ? ANNOTATE_CHALK
         : tbSel == SLOT_ERASER ? ANNOTATE_ERASER : ANNOTATE_NONE;
}

static void selectSlot(int i, struct Window *win, Micropolis *micropolis,
                       FrontendCallback *cb, bool running,
                       const char **toolNames, char *feedback,
                       int *footW, int *footH);

class FrontendCallback;

static void updateTitle(struct Window *win, Micropolis *m,
                        FrontendCallback *cb, bool running,
                        const char *toolName, const char *feedback);

static void selectSlot(int i, struct Window *win, Micropolis *micropolis,
                       FrontendCallback *cb, bool running,
                       const char **toolNames, char *feedback,
                       int *footW, int *footH)
{
    const ToolSlot *slot = &TOOLBAR_SLOTS[i];
    if (!slot->available) {
        snprintf(feedback, 96, "%s: not available yet (classic-only tool)",
                 slot->label);
        /* The title keeps naming the actually selected tool, not the
         * unavailable slot's engine fallback (TOOL_QUERY). */
        updateTitle(win, micropolis, cb, running,
                    toolNames[TOOLBAR_SLOTS[tbSel].tool], feedback);
        return;  /* selection does not change */
    }
    tbSel = i;
    *footW = slot->foot;
    *footH = slot->foot;
    if (slot->foot == 0)
        snprintf(feedback, 96, "tool: %s (free, not saved with the city)", slot->name);
    else
        snprintf(feedback, 96, "tool: %s (%dx%d, $%d)",
                 slot->name, slot->foot, slot->foot, slot->price);
    updateTitle(win, micropolis, cb, running, toolNames[slot->tool],
                feedback);
    renderToolbar();
}

static Window *openToolbar(GameDisplay &display, const WindowPlacement &placement)
{
    Screen *screen = display.screen;
    const int outerW = TB_W + screen->WBorLeft + screen->WBorRight;
    const int outerH = TB_H + screen->WBorTop + screen->WBorBottom + screen->Font->ta_YSize + 1;
    int left = 8, top = screen->BarHeight + 1;
    placement.place(screen->Width, screen->Height, screen->BarHeight + 1, outerW, outerH, left, top);
    return OpenWindowTags(NULL,
        display.custom ? WA_CustomScreen : WA_PubScreen, (IPTR)screen,
        WA_Title, (IPTR)"Tools", WA_Left, left, WA_Top, top,
        WA_InnerWidth, TB_W, WA_InnerHeight, TB_H,
        WA_Activate, FALSE, WA_CloseGadget, FALSE,
        WA_DragBar, TRUE, WA_DepthGadget, TRUE,
        WA_IDCMP, IDCMP_MOUSEBUTTONS | IDCMP_RAWKEY | IDCMP_REFRESHWINDOW | IDCMP_MENUPICK,
        TAG_DONE);
}

static void rememberWindow(WindowPlacement &placement, const Window *window)
{
    if (window) placement.remember(window->LeftEdge, window->TopEdge, window->Width, window->Height);
}

class FrontendCallback : public Callback {

public:

    virtual ~FrontendCallback() {}

    /* The interesting ones get printed in the window title later; for
     * now they must merely exist so the engine can call them. */
    void didLoseGame(Micropolis *, emscripten::val) { }
    void didWinGame(Micropolis *, emscripten::val) { }
    void updateDate(Micropolis *, emscripten::val, int cityYear, int cityMonth) {
        lastYear = cityYear;
        lastMonth = cityMonth;
    }
    void updateFunds(Micropolis *, emscripten::val, int totalFunds) {
        lastFunds = totalFunds;
    }

    void autoGoto(Micropolis *, emscripten::val, int, int, std::string) {}
    void didGenerateMap(Micropolis *, emscripten::val, int) {}
    void didLoadCity(Micropolis *, emscripten::val, std::string path) { cityFileName=path;scenario.reset(); earthquake.clear(); demand.reset(); chalk.clear(); }
    void didLoadScenario(Micropolis *, emscripten::val, std::string, std::string) {}
    void didSaveCity(Micropolis *, emscripten::val, std::string path) {cityFileName=path;}
    void didTool(Micropolis *, emscripten::val, std::string, int, int) {}
    void didntLoadCity(Micropolis *, emscripten::val, std::string) {}
    void didntSaveCity(Micropolis *, emscripten::val, std::string) {}
    GameAudio *audio=nullptr;
    EarthquakePresentation earthquake;
    ScenarioPresentation scenario;
    void makeSound(Micropolis *, emscripten::val, std::string, std::string sound, int, int) {
        if(audio)audio->play(sound.c_str());
    }
    void newGame(Micropolis *, emscripten::val) { history.reset(); scenario.reset(); earthquake.clear(); demand.reset(); chalk.clear(); }
    ChalkOverlay chalk;
    UnsavedCity unsaved;
    void saveCityAs(Micropolis *, emscripten::val, std::string) {}
    MessageHistory history;
    bool messagesEnabled=true,noticesEnabled=true;
    std::string cityFileName;
    void sendMessage(Micropolis *m, emscripten::val, int id, int x, int y,
                     bool picture, bool important) {
        if(messagesEnabled)history.receive(id,x,y,picture,important,m->cityTime);
        // Victory has no didWinGame callback in the pinned engine. Queue the
        // message outcome and leave the engine stack before showing a dialog.
        if(scenario.message(id))dialogShown=true;
    }
    Window *parent=nullptr;
    bool budgetOpen=false;
    bool dialogShown=false;
    void budget(Micropolis *m) {
        if(!parent || budgetOpen)return;
        budgetOpen=true;
        showCityBudget(parent,*m);
        budgetOpen=false;dialogShown=true;
    }
    void showBudgetAndWait(Micropolis *m, emscripten::val) {budget(m);}
    void showZoneStatus(Micropolis *, emscripten::val, int, int, int, int, int, int, int, int) {}
    void simulateRobots(Micropolis *, emscripten::val) {}
    void simulateChurch(Micropolis *, emscripten::val, int, int, int) {}
    void startEarthquake(Micropolis *, emscripten::val, int strength) { earthquake.start(strength); }
    void startGame(Micropolis *, emscripten::val) {}
    void startScenario(Micropolis *, emscripten::val, int) {}
    void updateBudget(Micropolis *, emscripten::val) {}
    void updateCityName(Micropolis *, emscripten::val, std::string) {}
    // Stores only; the overview polls `demand` from the render loop, so an
    // update never opens or activates a window.
    DemandModel demand;
    void updateDemand(Micropolis *, emscripten::val, float r, float c, float i) { demand.update(r,c,i); }
    void updateEvaluation(Micropolis *, emscripten::val) {}
    void updateGameLevel(Micropolis *, emscripten::val, int) {}
    void updateHistory(Micropolis *, emscripten::val) {}
    void updateMap(Micropolis *, emscripten::val) {}
    void updateOptions(Micropolis *, emscripten::val) {}
    void updatePasses(Micropolis *, emscripten::val, int) {}
    void updatePaused(Micropolis *, emscripten::val, bool) {}
    void updateSpeed(Micropolis *, emscripten::val, int) {}
    void updateTaxRate(Micropolis *, emscripten::val, int) {}

    int lastYear = 0;
    int lastMonth = 0;
    int lastFunds = 0;
};

/* ------------------------------------------------------------------ */

/* ------------------------------------------------------------------ */


/* Apply a tool and give visible feedback. The engine's toolDown()
 * returns void and TOOLRESULT_FAILED arrives without any message (the
 * gap is OURS to handle, not the engine's): so detect success by the
 * observable side effects - funds changed or the tile changed - and
 * report the outcome in the window title. */
static void buildAt(EditingTool tool, int mapX, int mapY,
                    struct Window *win, Micropolis *micropolis,
                    FrontendCallback *cb, bool running,
                    const char **toolNames, char *feedback)
{
    Quad fundsBefore = micropolis->totalFunds;
    int tileBefore = (mapX >= 0 && mapX < WORLD_W &&
                      mapY >= 0 && mapY < WORLD_H)
                         ? (int)(micropolis->map[mapX][mapY] & 0x03ff) : -1;

    micropolis->toolDown(tool, (short)mapX, (short)mapY);

    bool built = (micropolis->totalFunds != fundsBefore) ||
                 (tileBefore >= 0 &&
                  (int)(micropolis->map[mapX][mapY] & 0x03ff) != tileBefore);

    if (tool == TOOL_QUERY) {
        snprintf(feedback, 96, "tile (%d,%d) id=%d",
                 mapX, mapY,
                 (mapX >= 0 && mapX < WORLD_W &&
                  mapY >= 0 && mapY < WORLD_H)
                     ? (int)(micropolis->map[mapX][mapY] & 0x03ff) : -1);
    } else {
        snprintf(feedback, 96, "%s at (%d,%d): %s ($%ld)",
                 toolNames[tool], mapX, mapY,
                 built ? "BUILT" : "BLOCKED - clear the site first",
                 (long)micropolis->totalFunds);
    }
    updateTitle(win, micropolis, cb, running, toolNames[tool], feedback);
}

/* Draw a 2px outline around the tool footprint at view-pixel position
 * (vx, vy): white while hovering, red for a failed build flash. */
/* fb is the 16 px/tile world frame of fbW x fbH pixels. */
static void drawFootprint(uint32_t *fb, int fbW, int fbH, int vx, int vy,
                          int pw, int ph, uint32_t argb)
{
    int x0 = vx;
    int y0 = vy;
    int x1 = vx + pw - 1;
    int y1 = vy + ph - 1;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > fbW - 1) x1 = fbW - 1;
    if (y1 > fbH - 1) y1 = fbH - 1;
    for (int x = x0; x <= x1; x++) {
        for (int t = 0; t < 2; t++) {
            if (y0 + t <= y1) fb[(size_t)(y0 + t) * fbW + x] = argb;
            if (y1 - t >= y0) fb[(size_t)(y1 - t) * fbW + x] = argb;
        }
    }
    for (int y = y0; y <= y1; y++) {
        for (int t = 0; t < 2; t++) {
            if (x0 + t <= x1) fb[(size_t)y * fbW + x0 + t] = argb;
            if (x1 - t >= x0) fb[(size_t)y * fbW + x1 - t] = argb;
        }
    }
}

static void updateTitle(struct Window *win, Micropolis *m,
                        FrontendCallback *cb, bool running,
                        const char *toolName, const char *feedback)
{
    /* Intuition keeps the pointer, it does not copy - a local buffer
     * here produced a corrupt title after return. */
    static char title[160];
    // Chalk and eraser use TOOL_QUERY as an engine placeholder; name the slot.
    if (currentAnnotation() != ANNOTATE_NONE) toolName = TOOLBAR_SLOTS[tbSel].name;
    snprintf(title, sizeof(title),
             "Micropolis - %ld/%ld  $%ld  %s  [%s]%s%s",
             (long)simulationDate(m->cityTime).month, (long)simulationDate(m->cityTime).year,
             (long)m->totalFunds,
             running ? simulationSpeedName(m->simSpeed) : "paused",
             toolName,
             feedback[0] ? "  |  " : "",
             feedback);
    SetWindowTitles(win, (CONST_STRPTR)title, (CONST_STRPTR)-1);
}

/* ------------------------------------------------------------------ */

// Workbench launch must not open an unrelated console. Shell diagnostics stay.
extern "C" { int __nostdiowin=1; }

int main(int argc, char **argv)
{
    auto launchError=[&](const std::string &text){
        fprintf(stderr,"micropolis: %s\n",text.c_str());
        if(argc==0){
            EasyStruct request={sizeof(EasyStruct),0,(STRPTR)"Micropolis",(STRPTR)"%s",(STRPTR)"OK"};
            IPTR args[]={(IPTR)text.c_str()};
            EasyRequestArgs(nullptr,&request,nullptr,(RAWARG)args);
        }
        return 20;
    };
    std::string err;
    LaunchDirectory launch;
    if(!launch.prepare(argc,argv,err))return launchError(err);
    std::string cityFile=launch.arguments.empty()?"CITY.CTY":launch.arguments[0];
    std::string sheetFile=launch.arguments.size()>1?launch.arguments[1]:"tiles.bmp";
    if (!loadBmp8(sheetFile, sheet, err)) {
        return launchError("Cannot load tiles.bmp: "+err+"\nCopy the complete Micropolis folder.");
    }
    if (sheet.width != SHEET_COLS * TILE_SIZE ||
        sheet.height * SHEET_COLS < TILE_COUNT * TILE_SIZE) {
        return launchError("Invalid tile sheet dimensions. Copy the complete Micropolis folder.");
    }

    // An explicit tile sheet does not relocate the remaining default assets.
    std::string spriteDirectory=launch.arguments.size()>2?launch.arguments[2]:"sprites";
    SpriteArt spriteArt;
    if(!spriteArt.load(spriteDirectory,err)) {
        return launchError(err+"\nCopy the complete Micropolis folder, including sprites.");
    }

    std::unique_ptr<GameDisplay> display(new GameDisplay());
    if (!display->open(nullptr)) {
        return launchError("Cannot open the display.");
    }
    Window *win = display->window;
    auto createCity=[]()->Micropolis * {
        auto *city=new Micropolis();
        // Engine owns the callback, including discarded startup previews.
        city->setCallback(new FrontendCallback(),emscripten::val());
        city->init();
        return city;
    };
    GameAudio audio;
    audio.open("PROGDIR:sounds"); // Optional: missing device/assets leave the city playable.
    std::unique_ptr<Micropolis> selected;
    if(!launch.arguments.empty()) {
        // Explicit CLI cities and Workbench project icons bypass startup.
        selected.reset(createCity());
        if(!selected->loadCity(cityFile)) {
            return launchError("Cannot load city: "+cityFile);
        }
    } else {
        selected=showStartup(win,sheet,createCity,askFile,err);
        if(!selected) {
            return err.empty()?0:launchError("Startup: "+err+"\nCopy the complete Micropolis folder.");
        }
    }
    // Startup drains parent events while its modal selection window is open.
    // The player may still have resized the parent behind it.
    if(!display->resize()) {
        return launchError("Cannot synchronize the display after startup.");
    }
    Micropolis *micropolis=selected.get();
    auto *cb=static_cast<FrontendCallback *>(micropolis->callback);
    cb->audio=&audio;
    micropolis->cityEvaluation();
    micropolis->doInitialEval=false;
    // simTick needs a nonzero speed; the frontend decides when to call it.
    micropolis->setSpeed(3);
    cb->parent=win;
    cb->unsaved.baseline=startingFingerprint(*micropolis,cb->cityFileName);
    BitMap *offBm = display->bitmap;
    RastPort offRp = display->raster;
    view = display->view;
    ActivateWindow(win);

    {
        /* CreateDir returns a directory lock that must be released. */
        BPTR saveLock = CreateDir((CONST_STRPTR)SAVE_DIR);
        if (saveLock) UnLock(saveLock);
    }
    SimulationTimer timer;
    if(!timer.open())return launchError("Cannot open timer.device.");
    /* Player window placement per display kind (0 desktop, 1 own screen):
     * returning to a display restores where its windows were left. */
    struct DisplayLayout { WindowPlacement main, tools, messages, graphs, overview; };
    DisplayLayout layouts[2];
    tbWin = openToolbar(*display, layouts[0].tools);
    if (!tbWin)return launchError("Cannot open the tools window.");
    renderToolbar();
    GameMenu gameMenu,toolbarMenu;
    if(!gameMenu.attach(win)||!toolbarMenu.attach(tbWin))
        return launchError("Cannot create the application menus.");

    int camX = (WORLD_W - VIEW_TILES_W) / 2;
    int camY = (WORLD_H - VIEW_TILES_H) / 2;
    /* The selected tool is owned by the toolbar slot table now
     * (single selection path). `tool` mirrors it into the handlers. */
    EditingTool tool = TOOL_ROAD;
    int bCount = 0;
    /* Footprint of the current tool, in tiles: zones 3x3, lines and
     * single-tile tools 1x1. Airport/seaport etc. are not bound to
     * keys yet. */
    int footW = 1, footH = 1;
    char feedback[2048] = "";
    /* Hover position in view pixels (-1 = outside the map area). */
    int hoverVX = -1, hoverVY = -1;
    /* Drag-paint state: with a line tool (road/rail/wire) selected, an
     * LMB drag paints via toolDrag() instead of panning; the last tile
     * painted is kept between moves. */
    int paintTileX = -1, paintTileY = -1;
    /* Classic Tcl/Tk controls adapted to Amiga UI conventions: LMB builds,
     * middle pans, Shift+LMB opens the pie menu. RMB belongs to the native
     * Intuition menu strip; trapping it in the map makes that menu unusable. */
    bool panDragging = false;
    // Some AROS input paths do not repeat the Shift qualifier on the
    // following mouse event. Track the actual modifier keys as well.
    bool shiftHeld = false;
    /* Middle pan during an LMB gesture cancels it until LMB release - the
     * current !panDragging check alone only tests the present. */
    bool buildCancelled = false;
    const char *toolNames[TOOL_COUNT] = {
        "residential", "commercial", "industrial", "firestation",
        "policestation", "query", "wire", "bulldozer", "railroad",
        "road", "stadium", "park", "seaport", "coalpower",
        "nuclearpower", "airport", "network", "water", "land", "forest"
    };
    SimulationControl simulation;
    bool &running=simulation.running;
    std::vector<uint32_t> earthquakeScratch;
    bool needRender = true;
    bool done = false;
    bool dragging = false;
    bool dragMoved = false;
    int dragLastX = 0, dragLastY = 0;

    MessageWindow messageWindow;
    MessagePreviewContext previewContext{micropolis,&sheet,&spriteArt};
    messageWindow.configurePreview(&previewContext,renderMessagePreview,cb->noticesEnabled);
    GraphWindow graphWindow;
    OverviewWindow overviewWindow;
    // Poll results carrying a menu code become the same commands as the map's.
    auto forwardAction=[&](int action)->int {
        if(!isMenuPickResult(action))return action;
        const GameCommand picked=gameMenu.pick((UWORD)(action&0xffff));
        return picked==GameCommand::None?0:gameCommandCode(picked);
    };
    bool chalkVisible=true;
    bool quitRequested=false;
    int wheelZoom=0; // pending wheel steps, applied once per loop
    uint64_t messageRevision=cb->history.revision();
    int uiCommand = 0;
    int displayRequest = 0; // 1: toggle screen/window, 2: choose another mode
    bool resizePending = false;
    auto syncMenu=[&]{
        auto checked=[&](GameCommand command,bool value){
            gameMenu.checked(command,value);toolbarMenu.checked(command,value);
            messageWindow.checked(command,value);graphWindow.checked(command,value);
            overviewWindow.checked(command,value);
        };
        checked(GameCommand::AutoBudget,micropolis->autoBudget);
        checked(GameCommand::AutoBulldoze,micropolis->autoBulldoze);
        checked(GameCommand::DisastersEnabled,micropolis->enableDisasters);
        checked(GameCommand::Sound,micropolis->enableSound);
        checked(GameCommand::Animation,micropolis->doAnimation);
        checked(GameCommand::Messages,cb->messagesEnabled);
        checked(GameCommand::Notices,cb->noticesEnabled);
        checked(GameCommand::ChalkOverlay,chalkVisible);
        checked(GameCommand::Pause,!running);
        checked(GameCommand::Slow,running&&simulation.speed==1);
        checked(GameCommand::Medium,running&&simulation.speed==2);
        checked(GameCommand::Fast,running&&simulation.speed==3);
    };
    auto discardModalInput=[&] {
        timer.stop(); // Drop the expired pre-dialog deadline, never catch up.
        int ignoredX=0,ignoredY=0;
        messageWindow.poll(cb->history,ignoredX,ignoredY,true);
        graphWindow.poll(*micropolis,true);
        overviewWindow.poll(*micropolis,cb->demand,camX,camY,
                            VIEW_TILES_W,VIEW_TILES_H,true);
        dragging=panDragging=false;buildCancelled=true;hoverVX=hoverVY=-1;
        for(Window *covered : {win,tbWin}) {
            if(!covered)continue;
            while(auto *pending=(IntuiMessage *)GetMsg(covered->UserPort)) {
                ULONG cls=pending->Class;
                ReplyMsg((Message *)pending);
                // Input is stale after a modal pause, native lifecycle is not.
                if(covered==win && cls==IDCMP_NEWSIZE)resizePending=true;
                if(cls==IDCMP_REFRESHWINDOW) {
                    BeginRefresh(covered);EndRefresh(covered,TRUE);
                    needRender=true;
                    if(covered==tbWin)renderToolbar();
                }
                if(covered==win && cls==IDCMP_CLOSEWINDOW)quitRequested=true;
            }
        }
    };
    /* Leaving the current city (quit, load, choose another) never loses
     * unsaved work silently. Save reuses the safe replacement path. */
    auto confirmLeave=[&](const char *action)->bool {
        const bool modified=cb->unsaved.modified(currentFingerprint(*micropolis));
        const bool proceed=confirmLeavingCity(modified,
            [&]{
                char text[256];
                snprintf(text,sizeof text,"This city has changes that are not saved.\n"
                         "Save it before you %s?",action);
                EasyStruct ask={sizeof(EasyStruct),0,(STRPTR)"Micropolis",(STRPTR)"%s",
                                (STRPTR)"Save|Discard|Cancel"};
                IPTR args[]={(IPTR)text};
                return (UnsavedChoice)EasyRequestArgs(win,&ask,nullptr,(RAWARG)args);
            },
            [&]{
                char path[256];
                std::string target=cb->cityFileName;
                if(target.empty()) {
                    if(!askFile(win,true,path,sizeof path))return false;
                    target=path;
                }
                if(!saveCitySafely(*micropolis,target.c_str(),feedback,sizeof feedback)) {
                    EasyStruct failed={sizeof(EasyStruct),0,(STRPTR)"Micropolis save failed",
                                       (STRPTR)"%s",(STRPTR)"OK"};
                    IPTR args[]={(IPTR)feedback};
                    EasyRequestArgs(win,&failed,nullptr,(RAWARG)args);
                    return false;
                }
                cb->cityFileName=target;
                writeCityIcon(target.c_str());
                cb->unsaved.baseline=currentFingerprint(*micropolis);
                return true;
            });
        cb->dialogShown=true;
        return proceed;
    };
    /* A city chosen during play replaces the running one completely. Its
     * callback owns messages, chalk and scenario state, so every window
     * that caches the old city is rebound or reopened. */
    auto adoptCity=[&](std::unique_ptr<Micropolis> next) {
        const bool messagesOpen=messageWindow.window()!=nullptr;
        const bool graphsOpen=graphWindow.window()!=nullptr;
        messageWindow.close();graphWindow.close();
        // The overview points into the old callback's chalk; drop it first.
        overviewWindow.setChalk(nullptr);
        // Options > Messages / Notices are player settings, not city state.
        const bool messagesEnabled=cb->messagesEnabled,noticesEnabled=cb->noticesEnabled;
        selected=std::move(next); // deletes the previous city and its callback
        micropolis=selected.get();
        cb=static_cast<FrontendCallback *>(micropolis->callback);
        cb->audio=&audio;cb->parent=win;
        cb->messagesEnabled=messagesEnabled;cb->noticesEnabled=noticesEnabled;
        overviewWindow.setChalk(chalkVisible?&cb->chalk:nullptr);
        micropolis->cityEvaluation();
        micropolis->doInitialEval=false;
        micropolis->setSpeed(simulation.speed?simulation.speed:3);
        previewContext.city=micropolis;
        cb->unsaved.baseline=startingFingerprint(*micropolis,cb->cityFileName);
        messageRevision=cb->history.revision();
        camX=(WORLD_W-VIEW_TILES_W)/2;camY=(WORLD_H-VIEW_TILES_H)/2;
        view.clampCamera(camX,camY);
        if(messagesOpen)messageWindow.open(win,cb->history);
        if(graphsOpen)graphWindow.open(win,*micropolis);
        overviewWindow.refresh(*micropolis,cb->demand,camX,camY,VIEW_TILES_W,VIEW_TILES_H);
        dragging=panDragging=false;buildCancelled=true;hoverVX=hoverVY=-1;
        cb->dialogShown=true;needRender=true;
    };
    /* Zoom keeps the tile at the view centre in place. */
    auto setZoom=[&](int tile) {
        if(tile==display->tile)return;
        const int centerX=camX+VIEW_TILES_W/2,centerY=camY+VIEW_TILES_H/2;
        const int previous=display->tile;
        display->tile=tile;
        if(!display->resize()) {
            display->tile=previous;
            snprintf(feedback,sizeof feedback,"ZOOM FAILED: not enough memory; %d%% kept",zoomPercent(previous));
        } else {
            view=display->view;offBm=display->bitmap;offRp=display->raster;
            camX=centerX-VIEW_TILES_W/2;camY=centerY-VIEW_TILES_H/2;
            view.clampCamera(camX,camY);
            snprintf(feedback,sizeof feedback,"Zoom %d%% (+/- or wheel, 0 = 100%%)",zoomPercent(tile));
        }
        dragging=panDragging=false;buildCancelled=true;hoverVX=hoverVY=-1;
        updateTitle(win,micropolis,cb,running,toolNames[tool],feedback);
        needRender=true;
    };
    updateTitle(win,micropolis,cb,running,toolNames[tool],feedback);
    while (!done) {
        struct IntuiMessage *msg;
        audio.poll();
        messageWindow.configurePreview(&previewContext,renderMessagePreview,cb->noticesEnabled);
        overviewWindow.setChalk(chalkVisible?&cb->chalk:nullptr);
        syncMenu();
        if(running || cb->earthquake.active())timer.start();else timer.stop();
        ULONG ports = timer.signal() | (1L << win->UserPort->mp_SigBit);
        if (tbWin) ports |= (1L << tbWin->UserPort->mp_SigBit);
        if (messageWindow.window()) ports |= (1UL << messageWindow.window()->UserPort->mp_SigBit);
        if (graphWindow.window()) ports |= (1UL << graphWindow.window()->UserPort->mp_SigBit);
        if (overviewWindow.window()) ports |= (1UL << overviewWindow.window()->UserPort->mp_SigBit);
        if(!uiCommand && !needRender && !quitRequested)Wait(ports);
        int gotoX=0,gotoY=0;
        // A command returned by a modal F1 menu must run before other ports.
        int messageAction=uiCommand ? 0 : messageWindow.poll(cb->history,gotoX,gotoY);
        if(messageAction==1) {
            camX=gotoX-VIEW_TILES_W/2;
            camY=gotoY-VIEW_TILES_H/2;
            view.clampCamera(camX,camY);
            dragging=panDragging=false;buildCancelled=true;hoverVX=hoverVY=-1;
            snprintf(feedback,sizeof feedback,"Message location (%d,%d); camera (%d,%d)",gotoX,gotoY,camX,camY);
            updateTitle(win,micropolis,cb,running,toolNames[tool],feedback);
            WindowToFront(win);
            ActivateWindow(win);
            needRender=true;
        } else if(messageAction>1) {
            uiCommand=forwardAction(messageAction);
            messageAction=0;
        }

        if(!uiCommand)uiCommand=forwardAction(graphWindow.poll(*micropolis));
        if(!uiCommand) {
            int action=overviewWindow.poll(*micropolis,cb->demand,camX,camY,
                                           VIEW_TILES_W,VIEW_TILES_H);
            if(action==1)needRender=true;
            else if(action==2){showCityEvaluation(win,*micropolis);cb->dialogShown=true;needRender=true;}
            else if(action>2)uiCommand=forwardAction(action);
        }

        /* Toolbar window: a click selects the slot; the map window keeps
         * the keyboard, so hand activation back after the click. */
        if (tbWin && !uiCommand) {
            while ((msg = (struct IntuiMessage *)GetMsg(tbWin->UserPort))) {
                ULONG tclass = msg->Class;
                UWORD tcode = msg->Code;
                WORD tx = msg->MouseX - tbWin->BorderLeft;
                WORD ty = msg->MouseY - tbWin->BorderTop;
                ReplyMsg((struct Message *)msg);

                if (tclass == IDCMP_REFRESHWINDOW) {
                    BeginRefresh(tbWin); renderToolbar(); EndRefresh(tbWin, TRUE);
                }
                if (tclass == IDCMP_MENUPICK && tcode != MENUNULL && !uiCommand)
                    uiCommand = forwardAction(menuPickResult(tcode));
                if (tclass == IDCMP_RAWKEY && tcode >= 0x50 && tcode <= 0x55) uiCommand=tcode;
                if (tclass == IDCMP_RAWKEY && tcode == 0x59) displayRequest = 1;
                if (tclass == IDCMP_RAWKEY && tcode == 0x58) displayRequest = 2;
                if (tclass == IDCMP_MOUSEBUTTONS && tcode == SELECTDOWN &&
                    tx >= 0 && tx < TB_W && ty >= TB_LABELH && ty < 56) {
                    if(ty<TB_LABELH+15)displayRequest=1;
                    else {
                        const int cx=win->LeftEdge+win->BorderLeft+view.width/2;
                        const int cy=win->TopEdge+win->BorderTop+view.height/2;
                        int chosen=showClassicPieLatched(win,cx,cy);
                        if(chosen>=0) {
                            selectSlot(chosen,win,micropolis,cb,running,
                                       toolNames,feedback,&footW,&footH);
                            tool=currentTool();needRender=true;
                        }
                        cb->dialogShown=true;
                    }
                } else if (tclass == IDCMP_MOUSEBUTTONS && tcode == SELECTDOWN &&
                    tx >= 0 && ty >= TB_LABELH && ty < TB_GRID_H) {
                    const int i=classicToolAt(tx,ty);
                    if(i>=0) {
                        selectSlot(i,win,micropolis,cb,running,toolNames,feedback,&footW,&footH);
                        tool=currentTool();needRender=true;
                    }
                    ActivateWindow(win);  /* keep the keyboard on the map */
                }
            }
        }

        while (uiCommand || (msg = (struct IntuiMessage *)GetMsg(win->UserPort))) {
            ULONG msgClass=IDCMP_RAWKEY;
            UWORD code=uiCommand,qualifier=0;WORD mx=0,my=0;
            if(uiCommand)uiCommand=0;
            else {
                msgClass=msg->Class;code=msg->Code;
                qualifier=msg->Qualifier;
                mx=msg->MouseX-win->BorderLeft;my=msg->MouseY-win->BorderTop;
                ReplyMsg((struct Message *)msg);
            }
            // A queued click must not use the old camera with the new window
            // coordinates while a resize/screen transition is pending.
            if ((resizePending || displayRequest) &&
                (msgClass == IDCMP_MOUSEBUTTONS || msgClass == IDCMP_MOUSEMOVE)) continue;
            switch (msgClass) {
                case IDCMP_MENUPICK: {
                    // Menus may open in the middle of an LMB gesture.
                    dragging=panDragging=false;buildCancelled=true;cb->chalk.finish();
                    GameCommand picked=gameMenu.pick(code);
                    if(picked!=GameCommand::None)uiCommand=gameCommandCode(picked);
                    break;
                }
                case IDCMP_NEWSIZE:
                    resizePending = true;
                    // Coordinates before and after resizing cannot share a gesture.
                    dragging = panDragging = false;
                    buildCancelled = true;
                    hoverVX = hoverVY = -1;
                    break;
                case IDCMP_INACTIVEWINDOW:
                    dragging = panDragging = false;
                    buildCancelled = true;
                    hoverVX = hoverVY = -1;
                    needRender = true;
                    break;
                case IDCMP_REFRESHWINDOW:
                    BeginRefresh(win); EndRefresh(win, TRUE);
                    needRender = true;
                    break;
                case IDCMP_CLOSEWINDOW:
                    quitRequested = true;
                    break;

                case IDCMP_RAWKEY: {
                    /* Classic Amiga rawkey codes, verified on AROS One
                     * 1.3: presses arrive as the plain code (a release
                     * follows with the IECODE_UP_PREFIX bit set). Arrows:
                     * 0x4c-0x4f (verified working); digits 1-8: 0x01-0x08
                     * (release 0x81 observed); b/s/n/l: 0x35/0x21/0x36/
                     * 0x28; space 0x40; ESC 0x45. */
                    const UWORD baseCode=code&~IECODE_UP_PREFIX;
                    if(baseCode==RAWKEY_LSHIFT || baseCode==RAWKEY_RSHIFT) {
                        shiftHeld=(code&IECODE_UP_PREFIX)==0;
                        break;
                    }
                    bool commandHandled=false;
                    switch(decodeGameCommand(code)) {
                        case GameCommand::About: {
                            EasyStruct about={sizeof(EasyStruct),0,(STRPTR)"About Micropolis",
                                (STRPTR)"Micropolis for AROS\nNative Intuition/CyberGraphics frontend\nEngine and classic artwork: GPLv3+ with EA additional terms",
                                (STRPTR)"OK"};EasyRequestArgs(win,&about,nullptr,nullptr);cb->dialogShown=true;commandHandled=true;break;
                        }
                        case GameCommand::Save:
                            if(cb->cityFileName.empty())code=0x21;
                            else {cb->dialogShown=true;std::string target=cb->cityFileName;
                                if(saveCitySafely(*micropolis,target.c_str(),feedback,sizeof feedback)){writeCityIcon(target.c_str());cb->unsaved.baseline=currentFingerprint(*micropolis);}
                                cb->cityFileName=target;updateTitle(win,micropolis,cb,running,toolNames[tool],feedback);commandHandled=true;}break;
                        case GameCommand::SaveAs:code=0x21;break;
                        case GameCommand::Load:code=0x28;break;
                        case GameCommand::Quit:quitRequested=true;commandHandled=true;break;
                        case GameCommand::ChooseCity: {
                            commandHandled=true;
                            if(!confirmLeave("choose another city"))break;
                            timer.stop();
                            std::string startupError;
                            auto next=showStartup(win,sheet,createCity,askFile,startupError,true);
                            // The chooser replies to the map window's NEWSIZE without acting on it.
                            resizePending=true;
                            if(next) {
                                adoptCity(std::move(next));
                                snprintf(feedback,sizeof feedback,"New city: %s",micropolis->cityName.c_str());
                            } else if(!startupError.empty())
                                snprintf(feedback,sizeof feedback,"CHOOSE CITY FAILED: %s; current city kept",startupError.c_str());
                            cb->dialogShown=true;needRender=true;
                            break;
                        }
                        case GameCommand::ZoomIn:setZoom(zoomTileStep(display->tile,1));commandHandled=true;break;
                        case GameCommand::ZoomOut:setZoom(zoomTileStep(display->tile,-1));commandHandled=true;break;
                        case GameCommand::ZoomNormal:setZoom(16);commandHandled=true;break;
                        case GameCommand::ChalkOverlay:chalkVisible=!chalkVisible;needRender=true;commandHandled=true;break;
                        case GameCommand::AutoBudget:micropolis->setAutoBudget(!micropolis->autoBudget);commandHandled=true;break;
                        case GameCommand::AutoBulldoze:micropolis->setAutoBulldoze(!micropolis->autoBulldoze);commandHandled=true;break;
                        case GameCommand::DisastersEnabled:micropolis->setEnableDisasters(!micropolis->enableDisasters);commandHandled=true;break;
                        case GameCommand::Sound:micropolis->setEnableSound(!micropolis->enableSound);commandHandled=true;break;
                        case GameCommand::Animation:micropolis->setDoAnimation(!micropolis->doAnimation);commandHandled=true;break;
                        case GameCommand::Messages:cb->messagesEnabled=!cb->messagesEnabled;commandHandled=true;break;
                        case GameCommand::Notices:cb->noticesEnabled=!cb->noticesEnabled;commandHandled=true;break;
                        case GameCommand::Monster:micropolis->makeMonster();needRender=true;commandHandled=true;break;
                        case GameCommand::Fire:micropolis->makeFire();needRender=true;commandHandled=true;break;
                        case GameCommand::Flood:micropolis->makeFlood();needRender=true;commandHandled=true;break;
                        case GameCommand::Meltdown:micropolis->makeMeltdown();needRender=true;commandHandled=true;break;
                        case GameCommand::Tornado:micropolis->makeTornado();needRender=true;commandHandled=true;break;
                        case GameCommand::Earthquake:micropolis->makeEarthquake();needRender=true;commandHandled=true;break;
                        case GameCommand::Pause:simulation.select(0);micropolis->setSpeed(simulation.speed);timer.stop();commandHandled=true;break;
                        case GameCommand::Slow:simulation.select(1);micropolis->setSpeed(simulation.speed);commandHandled=true;break;
                        case GameCommand::Medium:simulation.select(2);micropolis->setSpeed(simulation.speed);commandHandled=true;break;
                        case GameCommand::Fast:simulation.select(3);micropolis->setSpeed(simulation.speed);commandHandled=true;break;
                        case GameCommand::Budget:code=0x51;break;
                        case GameCommand::Evaluation:code=0x52;break;
                        case GameCommand::Graphs:code=0x54;break;
                        case GameCommand::Overview:code=0x56;break;
                        case GameCommand::MessageHistory:code=0x53;break;
                        case GameCommand::DisplayToggle:code=0x59;break;
                        case GameCommand::DisplayMode:code=0x58;break;
                        default:break;
                    }
                    if(commandHandled){updateTitle(win,micropolis,cb,running,toolNames[tool],feedback);break;}
                    switch (code) {
                        case 0x55: { // F6: original Pause/Slow/Medium/Fast choices.
                            int choice=showSimulationSpeed(win,running?simulation.speed:0);
                            simulation.select(choice);micropolis->setSpeed(simulation.speed);
                            cb->dialogShown=true;needRender=true;
                            break;
                        }
                        case 0x50: uiCommand=showGameCommands(win,micropolis->enableSound);cb->dialogShown=true;needRender=true;break; // F1
                        case 0x100: {
                            const bool enabled=toggleGameSound(*micropolis);
                            snprintf(feedback,sizeof feedback,"sound effects: %s",enabled?"on":"off");
                            updateTitle(win,micropolis,cb,running,toolNames[tool],feedback);
                            needRender=true;
                            break;
                        }
                        case 0x53:
                            if(!messageWindow.open(win,cb->history)) {
                                snprintf(feedback,sizeof feedback,"Cannot open messages; history retained");
                                updateTitle(win,micropolis,cb,running,toolNames[tool],feedback);
                            } else {
                                WindowToFront(messageWindow.window());
                                ActivateWindow(messageWindow.window());
                            }
                            break;
                        case 0x51: cb->budget(micropolis);needRender=true;break; // F2
                        case 0x54:
                            if(!graphWindow.open(win,*micropolis)) {
                                snprintf(feedback,sizeof feedback,"Cannot open graphs; city retained");
                                updateTitle(win,micropolis,cb,running,toolNames[tool],feedback);
                            } else {
                                WindowToFront(graphWindow.window());ActivateWindow(graphWindow.window());
                            }
                            break;
                        case 0x56: // F7: classic overview and live R/C/I.
                            if(!overviewWindow.open(win,*micropolis,cb->demand,camX,camY,VIEW_TILES_W,VIEW_TILES_H)) {
                                snprintf(feedback,sizeof feedback,"Cannot open overview");
                                updateTitle(win,micropolis,cb,running,toolNames[tool],feedback);
                            } else {WindowToFront(overviewWindow.window());ActivateWindow(overviewWindow.window());}
                            break;
                        case 0x52: showCityEvaluation(win,*micropolis);cb->dialogShown=true;needRender=true;break; // F3
                        case 0x59: displayRequest = 1; break;
                        case 0x58: displayRequest = 2; break;
                        case RAWKEY_NM_WHEEL_UP: wheelZoom++; break;
                        case RAWKEY_NM_WHEEL_DOWN: wheelZoom--; break;
                        case 0x4e:  /* cursor right */
                            if (camX < WORLD_W - VIEW_TILES_W) { camX++; needRender = true; }
                            break;
                        case 0x4f:  /* cursor left */
                            if (camX > 0) { camX--; needRender = true; }
                            break;
                        case 0x4d:  /* cursor down */
                            if (camY < WORLD_H - VIEW_TILES_H) { camY++; needRender = true; }
                            break;
                        case 0x4c:  /* cursor up */
                            if (camY > 0) { camY--; needRender = true; }
                            break;
                        case 0x01: case 0x02: case 0x03: case 0x04:
                        case 0x05: case 0x06: case 0x07: case 0x08:
                        case 0x09: {
                            /* Digit shortcuts select the same toolbar
                             * slots the palette shows - one path. */
                            static const int digitSlots[9] = {
                                0, 1, 2, 9, 8, 6, 13, 7, 4
                            };
                            int i = digitSlots[code - 0x01];
                            selectSlot(i, win, micropolis, cb, running,
                                       toolNames, feedback,
                                       &footW, &footH);
                            tool = currentTool();
                            needRender = true;
                            break;
                        }
                        case 0x36: {  /* N: pause and step one fast engine batch. */
                            running=false;timer.stop();
                            simulationBatch(*micropolis,3,[&]{return cb->dialogShown;});
                            micropolis->setSpeed(simulation.speed);
                            needRender = true;
                            updateTitle(win, micropolis, cb, running, toolNames[tool], feedback);
                            break;
                        }
                        case 0x40:  /* space: toggle auto-run */
                            simulation.toggle();
                            micropolis->setSpeed(simulation.speed);
                            timer.stop();
                            updateTitle(win, micropolis, cb, running, toolNames[tool], feedback);
                            break;
                        case 0x35: {  /* b: apply tool at view center */
                            bCount++;
                            buildAt(tool,
                                    camX + VIEW_TILES_W / 2,
                                    camY + VIEW_TILES_H / 2,
                                    win, micropolis, cb, running,
                                    toolNames, feedback);
                            needRender = true;
                            break;
                        }
                        case 0x21: {  /* s: save with a file requester */
                            cb->dialogShown=true;
                            char path[256];
                            if (askFile(win, true, path, sizeof(path))) {
                                std::string previous=cb->cityFileName;
                                if(saveCitySafely(*micropolis,path,feedback,sizeof feedback)) {
                                    cb->cityFileName=path;
                                    writeCityIcon(path);
                                    cb->unsaved.baseline=currentFingerprint(*micropolis);
                                } else cb->cityFileName=previous;
                                if (strncmp(feedback, "SAVE FAILED:", 12) == 0) {
                                    // A title is too short for recovery paths. Keep the
                                    // full diagnostic visible in a modal requester.
                                    struct EasyStruct request = {
                                        sizeof(struct EasyStruct), 0,
                                        (STRPTR)"Micropolis save failed",
                                        (STRPTR)"%s", (STRPTR)"OK"
                                    };
                                    IPTR args[] = { (IPTR)feedback };
                                    EasyRequestArgs(win, &request, NULL, (RAWARG)args);
                                }
                                updateTitle(win, micropolis, cb, running,
                                            toolNames[tool], feedback);
                            }
                            break;
                        }
                        case 0x28: {  /* l: load with a file requester */
                            cb->dialogShown=true;
                            if(!confirmLeave("load another city"))break;
                            char path[256];
                            if (askFile(win, false, path, sizeof(path))) {
                                if (micropolis->loadCity(path)) {
                                    cb->unsaved.baseline=currentFingerprint(*micropolis);
                                    const bool messagesOpen=messageWindow.window()!=nullptr;
                                    if(messagesOpen) messageWindow.refresh(cb->history);
                                    messageRevision=cb->history.revision();
                                    dragging=panDragging=false;buildCancelled=true;hoverVX=hoverVY=-1;
                                    micropolis->cityEvaluation();
                                    micropolis->doInitialEval=false;
                                    snprintf(feedback, sizeof(feedback),
                                             "loaded: %s", path);
                                    needRender = true;
                                } else {
                                    snprintf(feedback, sizeof(feedback),
                                             "LOAD FAILED: %s", path);
                                }
                                updateTitle(win, micropolis, cb, running,
                                            toolNames[tool], feedback);
                            }
                            break;
                        }
                    }
                    break;
                }

                case IDCMP_MOUSEBUTTONS:
                    if(code==SELECTDOWN &&
                       (shiftHeld ||
                        (qualifier&(IEQUALIFIER_LSHIFT|IEQUALIFIER_RSHIFT)))) {
                        // Menu input must never turn into a pending build stroke.
                        dragging=panDragging=false;buildCancelled=true;
                        hoverVX=hoverVY=-1;
                        int chosen=showClassicPie(win,win->LeftEdge+win->BorderLeft+mx,
                            win->TopEdge+win->BorderTop+my,true);
                        if(chosen>=0) {
                            selectSlot(chosen,win,micropolis,cb,running,toolNames,feedback,&footW,&footH);
                            tool=currentTool();
                        }
                        cb->dialogShown=true;needRender=true;
                        break;
                    }
                    if (code == MENUDOWN) {
                        /* The menu interrupts whatever LMB gesture is under way;
                         * its release must not build or keep painting. */
                        buildCancelled = true;
                        cb->chalk.finish();
                    }
                    if (code == MIDDLEDOWN) {
                        /* Original button 2: pan independently of the tool. */
                        panDragging = true;
                        if (dragging) buildCancelled = true;
                        cb->chalk.finish();
                        dragLastX = mx;
                        dragLastY = my;
                    } else if (code == MIDDLEUP) {
                        panDragging = false;
                    }
                    if (code == SELECTDOWN) {
                        // Partial-tile margins are not map cells. Never arm a
                        // stroke whose first engine coordinate would be -1.
                        dragging = view.contains(mx, my);
                        dragMoved = false;
                        /* LMB starting while middle is held: the gesture is
                         * cancelled from the start, like a middle press
                         * during an LMB gesture. */
                        buildCancelled = panDragging || !dragging;
                        dragLastX = mx;
                        dragLastY = my;
                        /* Chalk and eraser act on press and drag, in world
                         * pixels, like the classic ToolDown/ToolDrag pair. */
                        if (!buildCancelled && currentAnnotation() != ANNOTATE_NONE) {
                            const int wx = camX * TILE_SIZE + view.worldX(mx);
                            const int wy = camY * TILE_SIZE + view.worldY(my);
                            if (currentAnnotation() == ANNOTATE_CHALK) cb->chalk.start(wx, wy);
                            else cb->chalk.eraseAt(wx, wy);
                            needRender = true;
                        }
                        if (mx >= 0 && my >= 0 &&
                            mx < VIEW_W && my < VIEW_H) {
                            paintTileX = view.mapX(mx, camX);
                            paintTileY = view.mapY(my, camY);
                        } else {
                            paintTileX = -1;
                            paintTileY = -1;
                        }
                    } else if (code == SELECTUP) {
                        const bool hadPress = dragging;
                        dragging = false;
                        if (currentAnnotation() != ANNOTATE_NONE) {
                            cb->chalk.finish();
                            buildCancelled = false;
                            break;
                        }
                        /* buildCancelled is cleared AFTER the click
                         * test below, so a pan-cancelled gesture can
                         * never build. */
                        if (hadPress && !dragMoved && !panDragging && !buildCancelled &&
                            mx >= 0 && my >= 0 &&
                            mx < VIEW_W && my < VIEW_H) {
                            /* panDragging guard: with both buttons held,
                             * middle-button panning wins and LMB release must
                             * not build. */
                            /* a click, not a drag: build at the tile
                             * under the pointer, in view coordinates */
                            buildAt(tool,
                                    view.mapX(mx, camX),
                                    view.mapY(my, camY),
                                    win, micropolis, cb, running,
                                    toolNames, feedback);
                            needRender = true;
                        }
                        buildCancelled = false;
                    }
                    break;

                case IDCMP_MOUSEMOVE:
                    if (mx >= 0 && my >= 0 && mx < VIEW_W && my < VIEW_H) {
                        if (hoverVX != mx || hoverVY != my) {
                            hoverVX = mx; hoverVY = my;
                            needRender = true;
                        }
                    } else if (hoverVX != -1) {
                        hoverVX = -1; hoverVY = -1;
                        needRender = true;
                    }
                    if (panDragging) {
                        /* Middle button: pan with sub-tile remainder kept. */
                        int dx = mx - dragLastX;
                        int dy = my - dragLastY;
                        int dtx = dx / view.tile;
                        int dty = dy / view.tile;
                        if (dtx != 0 || dty != 0) {
                            camX -= dtx;
                            camY -= dty;
                            dragLastX += dtx * view.tile;
                            dragLastY += dty * view.tile;
                            if (camX < 0) camX = 0;
                            if (camY < 0) camY = 0;
                            if (camX > WORLD_W - VIEW_TILES_W) camX = WORLD_W - VIEW_TILES_W;
                            if (camY > WORLD_H - VIEW_TILES_H) camY = WORLD_H - VIEW_TILES_H;
                            needRender = true;
                        }
                    }
                    if (dragging && currentAnnotation() != ANNOTATE_NONE) {
                        if (!buildCancelled && !panDragging && view.contains(mx, my)) {
                            const int wx = camX * TILE_SIZE + view.worldX(mx);
                            const int wy = camY * TILE_SIZE + view.worldY(my);
                            if (currentAnnotation() == ANNOTATE_CHALK) cb->chalk.extend(wx, wy);
                            else cb->chalk.eraseAt(wx, wy);
                            needRender = true;
                        }
                    } else if (dragging) {
                        bool lineTool = (tool == TOOL_ROAD ||
                                         tool == TOOL_RAILROAD ||
                                         tool == TOOL_WIRE ||
                                         tool == TOOL_BULLDOZER);
                        const bool inside = mx >= 0 && my >= 0 &&
                                            mx < VIEW_W && my < VIEW_H;
                        int tx = inside ? view.mapX(mx, camX) : -1;
                        int ty = inside ? view.mapY(my, camY) : -1;
                        if (abs(mx - dragLastX) > 4 || abs(my - dragLastY) > 4) {
                            dragMoved = true;
                        }
                        /* Painting is tile-based: crossing a tile boundary
                         * is a real stroke even when the pixel delta is below
                         * the click threshold (for example, starting two
                         * pixels before an edge). */
                        if (lineTool && inside &&
                            (tx != paintTileX || ty != paintTileY)) {
                            dragMoved = true;
                        }
                        if (dragMoved) {
                            /* buildCancelled blocks painting as well as
                             * the final click: LMB+middle then middle-up with
                             * LMB still held must not keep painting or
                             * bulldozing. */
                            if (lineTool && !buildCancelled && inside) {
                                /* LMB with a line tool: drag-paint via
                                 * the engine's toolDrag(). */
                                if (tx != paintTileX || ty != paintTileY) {
                                    micropolis->toolDrag(tool,
                                        (short)paintTileX, (short)paintTileY,
                                        (short)tx, (short)ty);
                                    paintTileX = tx;
                                    paintTileY = ty;
                                    snprintf(feedback, sizeof(feedback),
                                             "%s drag ($%ld)",
                                             toolNames[tool],
                                             (long)micropolis->totalFunds);
                                    updateTitle(win, micropolis, cb,
                                                running, toolNames[tool],
                                                feedback);
                                    needRender = true;
                                }
                            }
                            /* Non-line tools: LMB drag deliberately does
                             * nothing - fixed gestures mean panning is
                             * the middle button's job. */
                        }
                    }
                    break;

                case IDCMP_INTUITICKS:
                    // Native window ticks never drive simulation.
                    break;
            }
            if(cb->dialogShown)break;
        }

        if(wheelZoom) {
            setZoom(zoomTileStep(display->tile,wheelZoom>0?1:-1));
            wheelZoom=0;
        }
        if(quitRequested && !done) {
            quitRequested=false;
            if(confirmLeave("quit"))done=true;
        }

        if(!done && !cb->dialogShown && timer.consume()) {
            // Presentation completes even when the player pauses the city.
            if(cb->earthquake.tick())needRender=true;
            if(running) {
                simulationBatch(*micropolis,simulation.speed,[&]{return cb->dialogShown;});
                needRender=true;
                updateTitle(win,micropolis,cb,running,toolNames[tool],feedback);
            }
        }

        if(cb->dialogShown) {
            const auto outcome=cb->scenario.take();
            if(outcome!=ScenarioPresentation::None && !done) {
                running=false;
                timer.stop();
                // The requester covers the city until the player responds.
                // Publish the paused state first so the visible title agrees
                // with the text in the modal result window.
                updateTitle(win,micropolis,cb,running,toolNames[tool],feedback);
                EasyStruct notice={sizeof(EasyStruct),0,(STRPTR)"Micropolis - scenario result",
                    (STRPTR)(outcome==ScenarioPresentation::Won
                        ? "Scenario won!\nYour city is preserved and paused.\nPress Space when you want to continue."
                        : "Scenario lost.\nYour city is preserved and paused.\nPress Space when you want to continue."),
                    (STRPTR)"Keep city"};
                EasyRequestArgs(win,&notice,nullptr,nullptr);
            }
            cb->dialogShown=false;
            discardModalInput();
            updateTitle(win,micropolis,cb,running,toolNames[tool],feedback);
        }

        if (!done && displayRequest) {
            RequestedDisplayMode requested;
            int selected = 1;
            bool returnToWindow = display->custom && displayRequest == 1;
            if (!returnToWindow) {
                selected = chooseRequestedDisplayMode(win, requested);
                // The ASL requester is outside the regular modal branch.
                // Cancel/failure must not replay commands queued behind it.
                discardModalInput();
                if(done)selected=0;
            }
            displayRequest = 0;
            dragging = panDragging = false;
            buildCancelled = true;
            hoverVX = hoverVY = -1;
            if (selected == 1) {
                const int fromKind = display->custom ? 1 : 0;
                const int toKind = returnToWindow ? 0 : 1;
                auto &from = layouts[fromKind];
                auto &to = layouts[toKind];
                rememberWindow(from.main, win);
                rememberWindow(from.tools, tbWin);
                std::unique_ptr<GameDisplay> candidate(new GameDisplay());
                candidate->tile = display->tile;
                Window *candidateTools = nullptr;
                if (candidate->open(returnToWindow ? nullptr : &requested, &to.main))
                    candidateTools = openToolbar(*candidate, to.tools);
                if (candidateTools) {
                    const bool messagesOpen=messageWindow.window()!=nullptr;
                    const bool graphsOpen=graphWindow.window()!=nullptr;
                    const bool overviewOpen=overviewWindow.window()!=nullptr;
                    messageWindow.close();
                    graphWindow.close();
                    overviewWindow.close();
                    from.messages=messageWindow.placement;
                    from.graphs=graphWindow.placement;
                    from.overview=overviewWindow.placement;
                    messageWindow.placement=to.messages;
                    graphWindow.placement=to.graphs;
                    overviewWindow.placement=to.overview;
                    toolbarMenu.detach();
                    CloseWindow(tbWin);
                    gameMenu.detach();
                    tbWin = candidateTools;
                    display.swap(candidate);
                    win = display->window;
                    if(!gameMenu.attach(win)||!toolbarMenu.attach(tbWin))
                        snprintf(feedback,sizeof feedback,"Display changed; application menu unavailable");
                    cb->parent=win;
                    offBm = display->bitmap;
                    offRp = display->raster;
                    view = display->view;
                    view.clampCamera(camX, camY);
                    resizePending = false;
                    snprintf(feedback, sizeof feedback, "%s %dx%d; view %dx%d; F9 mode / F10 window",
                             display->custom ? "Screen" : "Desktop",
                             display->screen->Width, display->screen->Height, VIEW_W, VIEW_H);
                    renderToolbar();
                    if(messagesOpen && !messageWindow.open(win,cb->history))
                        snprintf(feedback,sizeof feedback,"Display changed; reopen messages with F4 (history retained)");
                    if(graphsOpen && !graphWindow.open(win,*micropolis))
                        snprintf(feedback,sizeof feedback,"Display changed; reopen graphs with F5 (city retained)");
                    if(overviewOpen && !overviewWindow.open(win,*micropolis,cb->demand,camX,camY,VIEW_TILES_W,VIEW_TILES_H))
                        snprintf(feedback,sizeof feedback,"Display changed; reopen overview with F7 (city retained)");
                } else {
                    snprintf(feedback, sizeof feedback, "DISPLAY FAILED: previous display and city retained");
                }
                candidate.reset(); // close old screen only after its toolbar is closed
            } else if (selected < 0) {
                snprintf(feedback, sizeof feedback, "DISPLAY FAILED: cannot open mode requester");
            }
            ScreenToFront(display->screen);
            ActivateWindow(win);
            updateTitle(win, micropolis, cb, running, toolNames[tool], feedback);
            needRender = true;
        }
        if (!done && resizePending) {
            resizePending = false;
            if (display->resize()) {
                view = display->view;
                offBm = display->bitmap;
                offRp = display->raster;
                view.clampCamera(camX, camY);
                snprintf(feedback, sizeof feedback, "View %dx%d (%dx%d tiles); F10 screen/window",
                         VIEW_W, VIEW_H, VIEW_TILES_W, VIEW_TILES_H);
            } else {
                SizeWindow(win, VIEW_W + win->BorderLeft + win->BorderRight - win->Width,
                                VIEW_H + win->BorderTop + win->BorderBottom - win->Height);
                snprintf(feedback, sizeof feedback, "RESIZE FAILED: previous view retained");
            }
            updateTitle(win, micropolis, cb, running, toolNames[tool], feedback);
            needRender = true;
        }
        messageWindow.refresh(cb->history);
        graphWindow.refresh(*micropolis);
        if(messageRevision!=cb->history.revision()) {
            messageRevision=cb->history.revision();
            if(!cb->history.entries().empty()) {
                const auto &notice=cb->history.entries().back();
                snprintf(feedback,sizeof feedback,"%s%s (F4 messages)",
                         notice.important?"IMPORTANT: ":"",notice.text.c_str());
                updateTitle(win,micropolis,cb,running,toolNames[tool],feedback);
            }
        }
        if (!done && needRender) {
            // Everything is composed in the 16 px world frame, then scaled once.
            auto &world = display->world;
            const int worldW = view.worldWidth(), worldH = view.worldHeight();
            renderCityTiles(*micropolis, sheet, world, worldW, worldH, camX, camY);
            renderSprites(spriteArt,micropolis->spriteList,world,worldW,worldH,camX,camY);
            if (chalkVisible)
                drawChalk(cb->chalk, world.data(), worldW, worldH, camX * TILE_SIZE, camY * TILE_SIZE);
            /* Tool footprint preview, tile-aligned. Zone tools (3x3)
             * land with their origin one tile up-left of the pointed
             * tile - the engine places zones that way - so the outline
             * must be centered on the pointed tile. */
            if (hoverVX >= 0 && hoverVY >= 0 && currentAnnotation() == ANNOTATE_ERASER) {
                // EraserTo removes strokes touching this 17x17 box.
                drawFootprint(world.data(), worldW, worldH,
                              view.worldX(hoverVX) - 8, view.worldY(hoverVY) - 8,
                              17, 17, 0xffffffff);
            } else if (hoverVX >= 0 && hoverVY >= 0 && footW > 0) {
                int htx = hoverVX / view.tile;
                int hty = hoverVY / view.tile;
                /* The engine places the footprint origin one tile up-left
                 * of the pointed tile for every multi-tile tool
                 * (3x3 zones and 4x4/6x6 buildings alike). */
                int ox = htx - ((footW > 1) ? 1 : 0);
                int oy = hty - ((footH > 1) ? 1 : 0);
                drawFootprint(world.data(), worldW, worldH, ox * TILE_SIZE, oy * TILE_SIZE,
                              footW * TILE_SIZE, footH * TILE_SIZE, 0xffffffff);
            }
            shiftEarthquakeFrame(world.data(), worldW, worldH, cb->earthquake.x(), cb->earthquake.y(), earthquakeScratch);
            scaleWorldFrame(world.data(), worldW, worldH, display->pixels.data(), view.tile);
            /* RECTFMT_BGRA32, not RECTFMT_ARGB: AROS x86_64 reads the
             * RECTFMT_ARGB source in big-endian [A][R][G][B] byte order,
             * so a native little-endian 0xAARRGGBB buffer must be handed
             * over as BGRA32 (the same choice the SDL2 AROS backend
             * makes). Verified 2026-09-15: ARGB painted an all-blue
             * image, BGRA32 paints the palette-perfect map. */
            SetDrMd(&offRp, JAM1);
            ULONG wpa = WritePixelArray(display->pixels.data(), 0, 0, VIEW_W * 4,
                                        &offRp, 0, 0,
                                        VIEW_W, VIEW_H, RECTFMT_BGRA32);
            if (wpa == 0) {
                /* WritePixelArray writes nothing on this AROS One build
                 * (reason still open: the HIDD bitmap is right, the blit
                 * works, graphics primitives work). Fallback: direct
                 * pixel access through LockBitMapTags, the same data the
                 * WPA path would have handed to HIDD_BM_PutImage. */
                APTR base = NULL;
                ULONG bpp = 0, bpr = 0;
                APTR lock = LockBitMapTags(offBm,
                    LBMI_BYTESPERPIX, (IPTR)&bpp,
                    LBMI_BYTESPERROW, (IPTR)&bpr,
                    LBMI_BASEADDRESS, (IPTR)&base,
                    TAG_DONE);
                if (lock && base && bpp == 4 && bpr >= (ULONG)(VIEW_W * 4)) {
                    for (int y = 0; y < VIEW_H; y++) {
                        memcpy((UBYTE *)base + (size_t)y * bpr,
                               display->pixels.data() + (size_t)y * VIEW_W,
                               VIEW_W * 4);
                    }
                    UnLockBitMap(lock);
                } else {
                    if (lock) UnLockBitMap(lock);
                    /* everything failed: black window as the visible
                     * diagnostic. Remove when the render path is
                     * understood. */
                    SetAPen(&offRp, 1);
                    RectFill(&offRp, 0, 0, VIEW_W - 1, VIEW_H - 1);
                }
            }
            // Clear the partial-tile margins left by non-multiple-of-16 sizes.
            // Never clear under the map: that exposes the background between
            // RectFill and the completed-frame blit on every hover update.
            SetAPen(win->RPort, 0);
            const int innerRight=win->Width-win->BorderRight;
            const int innerBottom=win->Height-win->BorderBottom;
            const int mapRight=win->BorderLeft+VIEW_W;
            const int mapBottom=win->BorderTop+VIEW_H;
            if(mapRight<innerRight)
                RectFill(win->RPort,mapRight,win->BorderTop,innerRight-1,innerBottom-1);
            if(mapBottom<innerBottom)
                RectFill(win->RPort,win->BorderLeft,mapBottom,innerRight-1,innerBottom-1);
            BltBitMapRastPort(offBm, 0, 0, win->RPort,
                              win->BorderLeft, win->BorderTop,
                              VIEW_W, VIEW_H, 0xc0);
            needRender = false;
            overviewWindow.refresh(*micropolis,cb->demand,camX,camY,VIEW_TILES_W,VIEW_TILES_H);
        }
    }

    timer.stop();
    messageWindow.close();
    graphWindow.close();
    overviewWindow.close();
    if (tbWin) { toolbarMenu.detach(); CloseWindow(tbWin); }
    tbWin = nullptr;
    gameMenu.detach();
    display.reset();
    selected.reset();
    printf("micropolis: done\n");
    return 0;
}

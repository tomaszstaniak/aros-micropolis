#ifndef MICROPOLIS_CLASSIC_TOOL_UI_H
#define MICROPOLIS_CLASSIC_TOOL_UI_H
struct RastPort;
struct Window;
void drawClassicTool(RastPort *rp,int slot,bool selected,bool enabled,int x,int y);
// Screen coordinates of the triggering press; returns palette slot or -1 on cancel.
int showClassicPie(Window *parent,int screenX,int screenY,bool leftButton);
// Open already latched (no triggering mouse button is currently held).
int showClassicPieLatched(Window *parent,int screenX,int screenY);
#endif

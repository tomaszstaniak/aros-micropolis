#include "game-command.h"
#include <cstdio>
int main(){int fail=0,total=0;auto c=[&](bool ok,const char*n){++total;if(!ok){++fail;std::printf("FAIL: %s\n",n);}};
c(gameCommandForRawKey(0x51)==GameCommand::Budget,"F2 and Budget menu share one command");
c(gameCommandForRawKey(0x56)==GameCommand::Overview,"F7 and Overview menu share one command");
c(gameCommandForRawKey(0x21)==GameCommand::SaveAs,"keyboard S retains requester behavior");
c(gameCommandForRawKey(0x45)==GameCommand::Quit,"Escape routes through Quit command");
c(gameCommandForRawKey(0x01)==GameCommand::None,"tool digit is not swallowed by application dispatcher");
c(gameCommandForRawKey(0x0c)==GameCommand::ZoomIn && gameCommandForRawKey(0x5e)==GameCommand::ZoomIn,"= and keypad + zoom in");
c(gameCommandForRawKey(0x0b)==GameCommand::ZoomOut && gameCommandForRawKey(0x4a)==GameCommand::ZoomOut,"- and keypad - zoom out");
c(gameCommandForRawKey(0x0a)==GameCommand::ZoomNormal && gameCommandForRawKey(0x0f)==GameCommand::ZoomNormal,"0 and keypad 0 restore 100%");
for(unsigned k=0x01;k<=0x09;++k)c(gameCommandForRawKey(k)==GameCommand::None,"digits 1-9 stay tool shortcuts");
c(gameCommandForRawKey(0x8c)==GameCommand::None && gameCommandForRawKey(0x8b)==GameCommand::None,"key releases never zoom");
c(decodeGameCommand(gameCommandCode(GameCommand::ChooseCity))==GameCommand::ChooseCity,"Choose City menu command round-trips");
c(decodeGameCommand(gameCommandCode(GameCommand::ChalkOverlay))==GameCommand::ChalkOverlay,"Chalk Overlay menu command round-trips");
std::printf("RESULT: %d/%d passed\n",total-fail,total);return fail?1:0;}

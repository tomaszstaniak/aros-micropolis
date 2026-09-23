#ifndef MICROPOLIS_GAME_MENU_H
#define MICROPOLIS_GAME_MENU_H
#include "game-command.h"
struct Window;struct Menu;
class GameMenu {
public:
    ~GameMenu();
    bool attach(Window *window);
    void detach();
    GameCommand pick(unsigned short menuCode) const;
    void checked(GameCommand command,bool value);
    Menu *strip() const{return menu_;}
private:
    Window *window_=nullptr;Menu *menu_=nullptr;void *visual_=nullptr;
};
// Poll results from auxiliary windows that carry a menu code.
constexpr int menuPickResult(unsigned short code){return 0x10000|code;}
constexpr bool isMenuPickResult(int value){return (value&0x10000)!=0;}
#endif

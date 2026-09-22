/*
 * Host-side map dump for choosing build coordinates: prints the tile
 * values of the same 40x28 view the game shows, with W marking water
 * (canals/rivers/sea) and . land. Debug utility, not shipped.
 */

#include "micropolis.h"

#include <cstdio>
#include <string>

#include <cstdlib>

class NoopCallback : public Callback {
public:
    virtual ~NoopCallback() {}
    void autoGoto(Micropolis *, emscripten::val, int, int, std::string) {}
    void didGenerateMap(Micropolis *, emscripten::val, int) {}
    void didLoadCity(Micropolis *, emscripten::val, std::string) {}
    void didLoadScenario(Micropolis *, emscripten::val, std::string, std::string) {}
    void didLoseGame(Micropolis *, emscripten::val) {}
    void didSaveCity(Micropolis *, emscripten::val, std::string) {}
    void didTool(Micropolis *, emscripten::val, std::string, int, int) {}
    void didWinGame(Micropolis *, emscripten::val) {}
    void didntLoadCity(Micropolis *, emscripten::val, std::string) {}
    void didntSaveCity(Micropolis *, emscripten::val, std::string) {}
    void makeSound(Micropolis *, emscripten::val, std::string, std::string, int, int) {}
    void newGame(Micropolis *, emscripten::val) {}
    void saveCityAs(Micropolis *, emscripten::val, std::string) {}
    void sendMessage(Micropolis *, emscripten::val, int, int, int, bool, bool) {}
    void showBudgetAndWait(Micropolis *, emscripten::val) {}
    void showZoneStatus(Micropolis *, emscripten::val, int, int, int, int, int, int, int, int) {}
    void simulateRobots(Micropolis *, emscripten::val) {}
    void simulateChurch(Micropolis *, emscripten::val, int, int, int) {}
    void startEarthquake(Micropolis *, emscripten::val, int) {}
    void startGame(Micropolis *, emscripten::val) {}
    void startScenario(Micropolis *, emscripten::val, int) {}
    void updateBudget(Micropolis *, emscripten::val) {}
    void updateCityName(Micropolis *, emscripten::val, std::string) {}
    void updateDate(Micropolis *, emscripten::val, int, int) {}
    void updateDemand(Micropolis *, emscripten::val, float, float, float) {}
    void updateEvaluation(Micropolis *, emscripten::val) {}
    void updateFunds(Micropolis *, emscripten::val, int) {}
    void updateGameLevel(Micropolis *, emscripten::val, int) {}
    void updateHistory(Micropolis *, emscripten::val) {}
    void updateMap(Micropolis *, emscripten::val) {}
    void updateOptions(Micropolis *, emscripten::val) {}
    void updatePasses(Micropolis *, emscripten::val, int) {}
    void updatePaused(Micropolis *, emscripten::val, bool) {}
    void updateSpeed(Micropolis *, emscripten::val, int) {}
    void updateTaxRate(Micropolis *, emscripten::val, int) {}
};

/* Tile bit values from the engine's Tiles enum (tool.h). */
static bool isWaterish(int tile)
{
    /* RIVER, canals and sea tiles are 30/31-family: use the two low
     * animation bits (RIVER merges into 30+2..) - print the raw value
     * instead and interpret here. */
    return false;
}

int main(int argc, char **argv)
{
    const char *city = (argc > 1) ? argv[1] : "CITY.CTY";
    int camX = (argc > 2) ? atoi(argv[2]) : 40;
    int camY = (argc > 3) ? atoi(argv[3]) : 36;

    Micropolis *m = new Micropolis();
    // Callback before init(): init() fires setters that dereference it.
    m->setCallback(new NoopCallback(), emscripten::val());
    m->init();
    if (!m->loadCity(city)) {
        printf("cannot load %s\n", city);
        return 1;
    }

    for (int ty = 0; ty < 28; ty++) {
        for (int tx = 0; tx < 40; tx++) {
            int x = camX + tx;
            int y = camY + ty;
            int t = (x >= 0 && x < WORLD_W && y >= 0 && y < WORLD_H)
                        ? (m->map[x][y] & 0x03ff) : -1;
            printf("%4d", t);
        }
        printf("\n");
    }
    return 0;
}
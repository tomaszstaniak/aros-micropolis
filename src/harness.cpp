/*
 * Headless harness for the MicropolisCore engine on AROS.
 *
 * Loads a city file, runs a fixed number of simulation phases, and writes
 * a status summary both to stdout and to MICRES.TXT in the current
 * directory. Output goes to a file because an emulated guest usually has
 * no reliable guest-to-host text path; stdout is still printed so a Shell
 * screendump can confirm it.
 *
 * Hard tick limit by design: the test always ends by itself. Usage: micropolis-harness <city.cty> [phases]
 */

#include "micropolis.h"

#include <cstdio>
#include <cstdlib>
#include <string>

/* Minimal Callback implementation: everything is a no-op except the few
 * events worth reporting from a headless run. */
class HeadlessCallback : public Callback {

public:

    virtual ~HeadlessCallback() {}

    void updateDate(Micropolis *, emscripten::val, int cityYear, int cityMonth) {
        fprintf(stdout, "date: year=%d month=%d\n", cityYear, cityMonth);
    }

    void updateFunds(Micropolis *, emscripten::val, int totalFunds) {
        fprintf(stdout, "funds: %d\n", totalFunds);
    }

    void didLoadCity(Micropolis *, emscripten::val, std::string filename) {
        fprintf(stdout, "loaded: %s\n", filename.c_str());
    }

    void sendMessage(Micropolis *, emscripten::val, int messageIndex, int x, int y, bool, bool) {
        fprintf(stdout, "message: index=%d at (%d,%d)\n", messageIndex, x, y);
    }

    void didLoseGame(Micropolis *, emscripten::val) {
        fprintf(stdout, "LOST GAME\n");
    }

    void didWinGame(Micropolis *, emscripten::val) {
        fprintf(stdout, "WON GAME\n");
    }

    /* The remaining events do not matter headlessly. */
    void autoGoto(Micropolis *, emscripten::val, int, int, std::string) {}
    void didGenerateMap(Micropolis *, emscripten::val, int) {}
    void didLoadScenario(Micropolis *, emscripten::val, std::string, std::string) {}
    void didSaveCity(Micropolis *, emscripten::val, std::string) {}
    void didTool(Micropolis *, emscripten::val, std::string, int, int) {}
    void didntLoadCity(Micropolis *, emscripten::val, std::string filename) {
        fprintf(stdout, "didntLoadCity: %s\n", filename.c_str());
    }
    void didntSaveCity(Micropolis *, emscripten::val, std::string) {}
    void makeSound(Micropolis *, emscripten::val, std::string, std::string, int, int) {}
    void newGame(Micropolis *, emscripten::val) {}
    void saveCityAs(Micropolis *, emscripten::val, std::string) {}
    void showBudgetAndWait(Micropolis *, emscripten::val) {}
    void showZoneStatus(Micropolis *, emscripten::val, int, int, int, int, int, int, int, int) {}
    void simulateRobots(Micropolis *, emscripten::val) {}
    void simulateChurch(Micropolis *, emscripten::val, int, int, int) {}
    void startEarthquake(Micropolis *, emscripten::val, int strength) {
        fprintf(stdout, "earthquake: strength=%d\n", strength);
    }
    void startGame(Micropolis *, emscripten::val) {}
    void startScenario(Micropolis *, emscripten::val, int) {}
    void updateBudget(Micropolis *, emscripten::val) {}
    void updateCityName(Micropolis *, emscripten::val, std::string) {}
    void updateDemand(Micropolis *, emscripten::val, float r, float c, float i) {
        fprintf(stdout, "demand: r=%.1f c=%.1f i=%.1f\n", r, c, i);
    }
    void updateEvaluation(Micropolis *, emscripten::val) {}
    void updateGameLevel(Micropolis *, emscripten::val, int) {}
    void updateHistory(Micropolis *, emscripten::val) {}
    void updateMap(Micropolis *, emscripten::val) {}
    void updateOptions(Micropolis *, emscripten::val) {}
    void updatePasses(Micropolis *, emscripten::val, int) {}
    void updatePaused(Micropolis *, emscripten::val, bool) {}
    void updateSpeed(Micropolis *, emscripten::val, int) {}
    void updateTaxRate(Micropolis *, emscripten::val, int) {}

};

int main(int argc, char **argv)
{
    const char *cityFile = (argc > 1) ? argv[1] : "CITY.CTY";
    /* 16 phases per cityTime unit; 16*12 = one in-game year. */
    int phases = (argc > 2) ? atoi(argv[2]) : (16 * 12);
    if (phases < 0 || phases > 16 * 12 * 10) {
        phases = 16 * 12 * 10;  // hard limit: never hang a shared machine
    }

    fprintf(stdout, "micropolis-harness: file=%s phases=%d\n", cityFile, phases);

    Micropolis *micropolis = new Micropolis();
    // Callback must be installed before init(): init() fires setters that
    // dereference callback (e.g. setCityTax -> updateTaxRate), and the
    // reference frontend does it in this order (MicropolisSimulator.ts
    // setCallback at 124 before init at 137). Verified the hard way on
    // AROS One: NULL callback crashed in setCityTax during init().
    micropolis->setCallback(new HeadlessCallback(), emscripten::val());
    micropolis->init();

    bool loaded = micropolis->loadCity(cityFile);
    fprintf(stdout, "loadCity: %s\n", loaded ? "ok" : "FAILED");

    int result = 1;
    if (loaded) {
        micropolis->setSpeed(3);
        for (int i = 0; i < phases; i++) {
            micropolis->simTick();
        }

        Quad population = micropolis->cityPop;
        fprintf(stdout,
                "RESULT: cityTime=%ld year=%ld month=%ld funds=%ld population=%ld\n",
                (long)micropolis->cityTime,
                (long)micropolis->cityYear,
                (long)micropolis->cityMonth,
                (long)micropolis->totalFunds,
                (long)population);
        result = 0;
    }

    fprintf(stdout, "micropolis-harness: done result=%d\n", result);
    fflush(stdout);

    /* Also write the file copy for whatever transport exists. */
    FILE *f = fopen("MICRES.TXT", "w");
    if (f) {
        fprintf(f, "micropolis-harness: file=%s phases=%d\n", cityFile, phases);
        fprintf(f, "loadCity: %s\n", loaded ? "ok" : "FAILED");
        if (loaded) {
            fprintf(f, "RESULT: cityTime=%ld year=%ld month=%ld funds=%ld population=%ld\n",
                    (long)micropolis->cityTime, (long)micropolis->cityYear,
                    (long)micropolis->cityMonth, (long)micropolis->totalFunds,
                    (long)micropolis->cityPop);
        }
        fprintf(f, "micropolis-harness: done result=%d\n", result);
        fclose(f);
    }

    return result;
}
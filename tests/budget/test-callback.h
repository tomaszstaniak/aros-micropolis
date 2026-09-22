class TestCallback : public Callback {

public:

    virtual ~TestCallback() {}

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

#ifndef MICROPOLIS_CITY_WINDOWS_H
#define MICROPOLIS_CITY_WINDOWS_H
struct Window;
class Micropolis;
// Modal windows pause frontend stepping; cancel leaves the budget unchanged.
bool showCityBudget(Window *parent, Micropolis &city);
// Returns a game command (usually an existing rawkey code), or zero on cancel.
int showGameCommands(Window *parent,bool soundEnabled);
// -1 cancels; 0 pauses; 1/2/3 are the classic slow/medium/fast engine levels.
int showSimulationSpeed(Window *parent,int selected);
void showCityEvaluation(Window *parent, const Micropolis &city);
#endif

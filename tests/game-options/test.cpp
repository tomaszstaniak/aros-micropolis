#include "game-options.h"

#include <cassert>
#include <cstdio>

struct City {
    bool enableSound = true;
    int updates = 0;
    void setEnableSound(bool enabled) { enableSound = enabled; ++updates; }
};

int main()
{
    City city;
    assert(!toggleGameSound(city));
    assert(!city.enableSound && city.updates == 1);
    assert(toggleGameSound(city));
    assert(city.enableSound && city.updates == 2);
    std::puts("RESULT: 2/2 passed");
}

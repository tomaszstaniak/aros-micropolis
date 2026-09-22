#ifndef MICROPOLIS_GAME_OPTIONS_H
#define MICROPOLIS_GAME_OPTIONS_H

template<class City>
bool toggleGameSound(City& city)
{
    city.setEnableSound(!city.enableSound);
    return city.enableSound;
}

#endif

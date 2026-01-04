/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ENVIRONMENT_WEATHER_STATES_H
#define COMS_ENVIRONMENT_WEATHER_STATES_H

#include "../../stdlib/Stdlib.h"

enum WeatherStates : byte {
    WEATHER_STATES_SUNNY = 1 << 0,
    WEATHER_STATES_RAINING = 1 << 2,
    WEATHER_STATES_STORMING = 1 << 3,
    WEATHER_STATES_HAIL = 1 << 4,
    WEATHER_STATES_SNOWING = 1 << 6,
    WEATHER_STATES_BLIZZARD = 1 << 7,
};

#endif
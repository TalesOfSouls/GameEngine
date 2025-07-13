/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_MOB_SECONDARY_STATS_POINTS_H
#define COMS_MODELS_MOB_SECONDARY_STATS_POINTS_H

#include "../stdlib/Types.h"

struct FixedStats {
    // Movement
    // Additional speeds may be defined for Mobs
    f32 speed_walk1;
    f32 speed_swim1;
    f32 speed_fly1;

    f32 speed_jump;
    f32 speed_dodge;
    f32 speed_turn;
};

#endif

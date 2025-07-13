/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_MOB_STATE_H
#define COMS_MODELS_MOB_STATE_H

#include "../Location.h"
#include "../../stdlib/Types.h"
#include "MobAction.h"

/**
 * @todo optimize order of struct members to ensure optimal struct size
 */

// @todo consider to have an array of mob_location which contains position+orientation probably much better cache wise?
// but these locations are only chunk relative?! Well the absolute position comes from the chunk id.
// The whole world is split into a x b x c chunks

struct MobState {
    Location location;

    f32 t;

    // Action performed
    // first byte = action category
    // last 3 bytes = animation to use
    uint32 action = (MOB_ACTION_INACTIVE << 24);

    int32 chunk_id;

    bool in_battle;

    byte environment; // dungeon/raid, pvp-openworld, pvp, pvp-tournament, open-world, instance-private, instance-invite, housing,
};

#endif
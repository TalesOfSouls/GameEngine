/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_MOB_H
#define COMS_MODELS_MOB_H

#include "../../stdlib/Types.h"

#include "../settings/Settings.h"
#include "../item/Equipment.h"

#include "MobState.h"

// @todo consider to create 3 mob state arrays instead and have every array ordered in the same way

struct Mob {
    byte category;
    byte mclass;

    byte level;
    byte rank;

    unsigned int id;
    unsigned int guild_id;

    MobState state;
    MobState state1;
    MobState state2;

    // Data layout
    // 12223444
    //      1: scale sign
    //      2: scale factor (8)
    //      3: weight sign
    //      4: weight factor (8)
    byte scale;
    byte weight;

    // Data layout
    // 11111112222222333333344444445555
    //      1: Base color (skin color)
    //      2: Second color (head hair color)
    //      3: Third color (head hair color)
    //      4: Fourth color (head hair color)
    //      5: Fifth color (head hair color)

    SecondaryStatsValues max;
    SecondaryStatsValues current;
};

#endif
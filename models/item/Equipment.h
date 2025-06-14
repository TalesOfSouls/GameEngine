/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_EQUIPMENT_H
#define COMS_MODELS_EQUIPMENT_H

#include "../../stdlib/Types.h"

#include "../mob/MobStats.h"
#include "../mob/skill/StatsTarget.h"

// @todo when to use points and when to use values?

struct Equipment {
    byte type;
    char* name;
};

struct SEquipmentStatsPoints {
    // Item requirements
    PrimaryStatsPoints requirements;

    // @todo Find a way to add/multiply stats on conditions
    //      e.g. x% or x amount of health/resource

    // Item stats
    // items cannot have stats like str, they can only modify primary stats of chars (see below)
    SecondaryStatsPoints secondary_item;

    // Modifies the char stats
    // @todo A character cannot do for example fire damage (only items and skills can do that)
    //  This means these stats are unused and just use up memory
    PrimaryStatsPoints primary_char_add;
    PrimaryStatsPoints primary_char_mul;

    SecondaryStatsPoints secondary_char_add;
    SecondaryStatsPoints secondary_char_mul;

    // Modifies the skills
    // only modifies skills that have these stats != 0
    // @question is primary for skill necessary?
    PrimaryStatsPoints primary_skill_add;
    PrimaryStatsPoints primary_skill_mul;

    SecondaryStatsPoints secondary_skill_add;
    SecondaryStatsPoints secondary_skill_mul;
};

#endif
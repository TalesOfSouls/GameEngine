/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_MOB_STATS_H
#define COMS_MODELS_MOB_STATS_H

#include "../../stdlib/Types.h"
#include "PrimaryStatsPoints.h"
#include "SecondaryStatsPoints.h"

/**
 * @todo optimize order of struct members to ensure optimal struct size
 */
struct SMobStatsPoints {
    // @todo Add min max for dmg
    //      Every attack should have a damage range (maybe 5%?)

    // Self stats
    PrimaryStatsPoints primary_stats;
    SecondaryStatsPoints secondary_stats; // @todo this is bad, a char doesn't have fire dmg but might have crit chance ... needs to split?

    // Item modifiers
    PrimaryStatsPoints item_primary_add;
    PrimaryStatsPoints item_primary_mul;

    SecondaryStatsPoints item_secondary_add;
    SecondaryStatsPoints item_secondary_mul;

    // Skill modifiers
    PrimaryStatsPoints skill_primary_add;
    PrimaryStatsPoints skill_primary_mul;

    SecondaryStatsPoints skill_secondary_add;
    SecondaryStatsPoints skill_secondary_mul;
};

#endif
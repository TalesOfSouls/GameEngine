/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_CONSUMABLE_TYPE_H
#define COMS_MODELS_CONSUMABLE_TYPE_H

#include "../../stdlib/Types.h"

#include "../mob/PrimaryStatsPoints.h"
#include "../mob/SecondaryStatsPoints.h"

struct ConsumableType {
    byte target;
    f32 range;

    // Character
    PrimaryStatsPoints primary_char_add;
    SecondaryStatsPoints secondary_char_add;

    PrimaryStatsPoints primary_char_mul;
    SecondaryStatsPoints secondary_char_mul;

    // Skill
    PrimaryStatsPoints primary_skill_add;
    SecondaryStatsPoints secondary_skill_add;

    PrimaryStatsPoints primary_skill_mul;
    SecondaryStatsPoints secondary_skill_mul;
};

#endif
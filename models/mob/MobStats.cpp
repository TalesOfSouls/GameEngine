/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_MOB_STATS_C
#define COMS_MODELS_MOB_STATS_C

#include "MobStats.h"
#include "../../stdlib/Simd.h"

// Calculate whenever character points or items change
// 1. combine primary Item points with character points
// 2. combine secondary item points with character points
// 3. calculate semi static character values from points

// Calculated for every skill and then cached whenever that skill is used
inline
void calculate_modify_values(
    const Skill* skill,
    const SMobStatsTotal* mob_stats,
    const SEquipmentStatsTotal* item_stats,
    SecondaryStatsValues* modify_values
)
{

}

// @todo check what to do??

inline
void handle_character_fight_action(
    const SecondaryStatsValues* modify_values,
    const Skill* skill,
    SecondaryStatsValues *mob_values,
    int steps
)
{
    // @todo not correct since the modify_value needs further modification in relation to the effected mob (e.g. defense consideration)
    // but this means i also need the skill as parameter
    int32_4_add((int32 *) modify_values, (int32 *) mob_values, (int32 *) mob_values, 10, steps);
    f32_4_add((f32 *) (modify_values + 1), (f32 *) (mob_values + 1), (f32 *) (mob_values + 1), 10, steps);
}

#endif
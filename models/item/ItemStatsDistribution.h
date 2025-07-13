/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_ITEM_STATS_DISTRIBUTION_H
#define COMS_MODELS_ITEM_STATS_DISTRIBUTION_H

#include "ItemAffixDistribution.h"

// This is only used in EquipmentType
// @question Should we also use it in Rarity and replace the current impl. in Rarity?
//  Implementing it also in rarity would basically give us more detailed control in rarities as well
struct ItemStatsDistribution {
    // item
    int item_secondary_count_min;
    int item_secondary_count_max;

    int item_flags_dmg_count_min;
    int item_flags_dmg_count_max;

    int item_flags_def_count_min;
    int item_flags_def_count_max;

    // character
    // can be add and mul
    int char_count_min;
    int char_count_max;

    int char_primary_count_min;
    int char_primary_count_max;

    int char_secondary_count_min;
    int char_secondary_count_max;
    ItemAffixDistribution char_secondary_distribution;

    // skill
    // can be add and mul
    int skill_count_min;
    int skill_count_max;

    int skill_primary_count_min;
    int skill_primary_count_max;

    int skill_secondary_count_min;
    int skill_secondary_count_max;
    ItemAffixDistribution skill_secondary_distribution;
};

#endif
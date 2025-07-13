/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_ITEM_AFFIX_DISTRIBUTION_H
#define COMS_MODELS_ITEM_AFFIX_DISTRIBUTION_H

// WARNING: The sum of all mins must be smaller than max count of the whole category
// This allows us to define how many phys dmg stats an item should have etc.
// the sum of all min is most likely lower than the parent min/max -> we can randomely assign additional
// stat categories to the item as long as their min/max value is not 0 = which means not allowed
struct ItemAffixDistribution {
    // damage
    int dmg_count_min;
    int dmg_count_max;

    int phys_dmg_count_min;
    int phys_dmg_count_max;

    int elemental_dmg_count_min;
    int elemental_dmg_count_max;

    int magic_dmg_count_min;
    int magic_dmg_count_max;

    // defense
    int def_count_min;
    int def_count_max;

    int phys_def_count_min;
    int phys_def_count_max;

    int elemental_def_count_min;
    int elemental_def_count_max;

    int magic_def_count_min;
    int magic_def_count_max;

    // other
    int other_count_min;
    int other_count_max;

    int health_count_min;
    int health_count_max;

    int resource_count_min;
    int resource_count_max;

    int movement_count_min;
    int movement_count_max;

    int modifier_count_min;
    int modifier_count_max;

    int special_count_min;
    int special_count_max;
};

#endif
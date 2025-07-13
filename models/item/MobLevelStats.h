/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_ITEM_MOB_LEVEL_STATS_H
#define COMS_MODELS_ITEM_MOB_LEVEL_STATS_H

/**
 * This defines the bounds for what items can drop based on the mob level.
 * Additional LootTables may further restrict or modify this.
 * This also functions as a guard for typos in mob definitions to avoid ludicrous gold and xp drops
 */
struct MobLevelStats {
    int xp;

    int gold_min;
    int gold_max;

    int item_level_min;
    int item_level_max;
};

inline
int get_random_item_level(const MobLevelStats* level_data, int mob_level)
{
    const MobLevelStats* mob_stats = level_data + mob_level - 1;
    return mob_stats->item_level_min + rand() % (mob_stats->item_level_max - mob_stats->item_level_min + 1);;
}

#endif
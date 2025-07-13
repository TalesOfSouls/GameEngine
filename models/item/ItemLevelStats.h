/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_ITEM_LEVEL_STATS_H
#define COMS_MODELS_ITEM_LEVEL_STATS_H

#include "ItemRarityStats.h"
#include "MobLevelStats.h"
#include "_item_rarity.h"

struct ItemLevelStats {
    ItemRarityStats rarity_stats[RARITY_TYPE_SIZE];
};

#endif
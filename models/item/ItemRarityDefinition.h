/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_ITEM_RARITY_DEFINITION_H
#define COMS_MODELS_ITEM_RARITY_DEFINITION_H

#include "ItemStatsDistribution.h"

#define ITEM_RARITY_100 10000000000

// This puts a hard limit on StatsDistribution
struct RarityDefinition {
    int drop_chance;

    int potential_min;
    int potential_max;

    ItemStatsDistribution stats_distribution;
};

int get_random_item_rarity(const RarityDefinition* rarities, int rarity_count)
{
    uint32 random_rarity = rand() % (ITEM_RARITY_100 + 1);
    uint32 current_rarity = 0;
    for (int i = 0; i < rarity_count - 1; ++i) {
        current_rarity += rarities[i].drop_chance;

        if (current_rarity < random_rarity) {
            return i;
        }
    }

    return rarity_count - 1;
}
#endif
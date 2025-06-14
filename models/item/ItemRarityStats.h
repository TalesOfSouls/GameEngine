/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_ITEM_RARITY_STATS_H
#define COMS_MODELS_ITEM_RARITY_STATS_H

/**
 * This defines the basic stat range and averge for all item rarities.
 */
struct ItemRarityStats {
    int stats_avg;
    int stats_min;
    int stats_max;

    float shift_min;
    float shift_max;
};

#endif
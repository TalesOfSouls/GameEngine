/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_MOB_LOOT_TABLE_H
#define COMS_MODELS_MOB_LOOT_TABLE_H

#include "../../../stdlib/Types.h"
#include "../../../utils/Utils.h"
#include "Drop.h"

// @todo how to do class specific loot table?
struct LootTable {
    // Chance this table becomes effective at all
    // Useful to define multiple loot tables for a mob e.g. normal drop + 1 rare guarantueed
    f32 table_chance;

    uint64* items;
    int32 table_size;

    // If drop chance = -1 -> use default rarity drop chance
    f32* item_drop_chances;

    // How many stacks of that item should be dropped
    // Usually only used for consumables
    int32* item_min_drop_count;
    int32* item_max_drop_count;
    bool item_unique;

    // How many "different" items should be dropped
    uint32 item_min_count;
    uint32 item_max_count;

    // How much gold should be dropped
    uint32 gold_min_count;
    uint32 gold_max_count;

    // How much xp should be dropped
    uint32 xp_min_count;
    uint32 xp_max_count;
};

// 1. check if table comes into effect
// 2. check if max item drop count is exceeded
void loot_table_drop(const LootTable* table, Drop* drop, uint32 counter = 0)
{
    f32 rand = fast_rand_percentage();
    if (counter >= table->item_max_count
        || rand > table->table_chance
    ) {
        drop = NULL;
        return;
    }

    f32 range_value = 0;
    int32 i = 0;
    for (i = 0; i < table->table_size; ++i) {
        range_value += table->item_drop_chances[i];

        if (range_value < rand) {
            drop->item = table->items[i];
            break;
        }
    }

    if (i >= table->table_size) {
        drop = NULL;
        return;
    }

    drop->quantity = 1;
    if (table->item_max_drop_count[i] > 1) {
        rand = fast_rand_percentage();
        drop->quantity = OMS_MAX(table->item_min_drop_count[i], (int) ((f32) table->item_max_count * rand));
    }
}

uint64 loot_table_drop_gold(const LootTable* table)
{
    if (table->gold_max_count == 0) {
        return 0;
    }

    f32 rand = fast_rand_percentage();
    if (rand > table->table_chance) {
        return 0;
    }

    return OMS_MAX(table->gold_min_count, (uint64) ((f32) table->gold_max_count * rand));

    // WARNING: This is mathematically WRONG!
    //      The expected value of the version above is higher than what you would actually expect
    //      The correct version would be the one below (although slower)
    /*
    uint32 r = rand();
    if (r > table->table_chance * RAND_MAX) {
        return 0;
    }

    return (r % (table->gold_max_count - table->gold_min_count + 1)) + table->gold_min_count;
    */
}

#endif
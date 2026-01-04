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

#include "../../../stdlib/Stdlib.h"
#include "../../../utils/Utils.h"
#include "../../../utils/RandomUtils.h"
#include "Drop.h"
#include "../../item/ItemRarity.h"

// Loot tables are 2-layered
//      1. basic item selection
//      2. only if defined individual drop distributions

// @todo how to do class specific loot table?
struct LootTable {
    uint64* items;
    int32 item_count;

    RarityDropChance item_drop_chances;
    RarityDropCount item_drop_count;

    // should item drops be unique?
    bool item_unique;

    // @performance
    // @todo We don't want the stuff below in here since this means we have to reserve the space
    // It would be nicer if we could make it optional

    // How much gold should be dropped
    // Only used if defined otherwise global list is used
    // This is important so we can create a gold leprechaun / pixiu
    f32 gold_drop_probability;
    v2_int32 gold;

    // How much xp should be dropped
    // Only used if defined otherwise global list is used
    // This is important so we can create a xp leprechaun / maybe owl as symbol instead
    // -1 use general xp
    v2_int16 xp;
};

// 1. check if table comes into effect
// 2. check if max item drop count is exceeded
void loot_table_drop(const LootTable* table, Drop* drop, uint32 counter = 0)
{
    f32 rand = rand_fast_percent();
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
        rand = rand_fast_percent();
        drop->quantity = OMS_MAX(table->item_min_drop_count[i], (int) ((f32) table->item_max_count * rand));
    }
}

uint64 loot_table_drop_gold(const LootTable* table)
{
    if (table->gold_max_count == 0) {
        return 0;
    }

    f32 rand = rand_fast_percent();
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
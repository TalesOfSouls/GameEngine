/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_BACKPACK_H
#define COMS_MODELS_BACKPACK_H

#include "../../../stdlib/Types.h"
#include "../../item/Item.h"

#define MAX_BACKPACK_ITEM_STACK 1024

struct Backpack {
    uint32 size;
    Item* items;
    uint32* quantities;
};

bool backpack_split_items(Backpack* backpack, uint32 index, uint32 quantity)
{
    if (quantity > backpack->quantities[index] && quantity > 0) {
        return false;
    }

    // Find empty slot
    int32 empty_slot = -1;
    for (uint32 i = 0; i < backpack->size; ++i) {
        if (backpack->quantities[i] == 0) {
            empty_slot = i;
        }
    }

    if (empty_slot < 0) {
        return false;
    }

    memcpy(backpack->items + empty_slot, backpack->items + index, sizeof(Item));
    backpack->quantities[empty_slot] = quantity;

    return true;
}

void backpack_move_item(Backpack* backpack, uint32 from, uint32 to)
{
    if (backpack->quantities[from] == 0) {
        return;
    }

    // Handle item stacking
    // @todo only allow stacking for consumables or items that have a stackable flag defined?!
    if (backpack->items[to].id == backpack->items[from].id) {
        // Only change stack up to MAX_BACKPACK_ITEM_STACK
        uint32 stack_change = OMS_MIN(MAX_BACKPACK_ITEM_STACK - backpack->quantities[to], backpack->quantities[from]);
        backpack->quantities[to] += stack_change;
        backpack->quantities[from] -= stack_change;

        return;
    }

    Item temp_item = {};
    memcpy(&temp_item, backpack->items + to, sizeof(Item));
    int32 temp_quantity = backpack->quantities[to];

    memcpy(backpack->items + to, backpack->items + from, sizeof(Item));

    if (temp_quantity == 0) {
        backpack->quantities[from] = 0;

        return;
    }

    memcpy(backpack->items + from, &temp_item, sizeof(Item));
    backpack->quantities[from] = temp_quantity;
}

inline
int32 backpack_find_empty(const Backpack* backpack)
{
    for (uint32 i = 0; i < backpack->size; ++i) {
        if (backpack->quantities[i] == 0) {
            return i;
        }
    }

    return -1;
}

int32 backpack_add_item(Backpack* backpack, const Item* item, int32 quantities)
{
    for (uint32 i = 0; i < backpack->size; ++i) {
        if (backpack->quantities[i] == 0) {
            // @performance Do We have to memcpy?
            //      It could be MUCH better to just store pointers in the backpack and have a second unordered backpack
            //      This would make changing orders much more efficient.
            //      At the same time how often are we really changing the order?!
            memcpy(backpack->items + i, item, sizeof(Item));
            backpack->quantities[i] = quantities;

            return i;
        }
    }

    return -1;
}

void backpack_remove_item(Backpack* backpack, uint32 index)
{
    if (backpack->quantities[index] == 0) {
        return;
    }

    backpack->quantities[index] = 0;
}

#endif
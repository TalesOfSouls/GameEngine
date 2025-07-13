/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_EQUIPMENT_TYPE_H
#define COMS_MODELS_EQUIPMENT_TYPE_H

#include "../../stdlib/Types.h"

#include "../mob/MobStats.h"
#include "ItemStatsDistribution.h"

struct EquipmentType {
    byte slot;
    uint32 char_class;
    bool is_dual;
    bool is_throwing;
    bool is_projectile;
    bool is_damage;
    bool is_armor;
    bool is_supporting;
    bool is_beam;
    bool is_ranged;

    byte potential_min;
    byte potential_max;

    // @question Do we want a equipment specific potential? currently only rarity dependent!

    // This defines how many stats can be asigned to an item based on the item type
    // @todo for the correct algorithm we however also need to consider rarity of the item defining how many
    //      total stats/affixes/enchantments can actually be assigned.
    //      we chust need to define that static struct/array (we already have a draft in the excel file)
    //      of course this should be probably automatically generated from the database at compile time as a pre_compile program
    ItemStatsDistribution stats_distribution;

    // The min/max point range is calculated by checking the rarity values + item level
    // The values stored in the structs below are the "average" value which then gets randomely shifted by the rarity+item level
    PrimaryStatsPoints primary_item_req_min;
    PrimaryStatsPoints primary_item_req_max;

    SecondaryStatsPoints secondary_item_min;
    SecondaryStatsPoints secondary_item_max;

    // Character
    // add
    PrimaryStatsPoints primary_char_add_min;
    PrimaryStatsPoints primary_char_add_max;

    SecondaryStatsPoints secondary_char_add_min;
    SecondaryStatsPoints secondary_char_add_max;

    // mul
    PrimaryStatsPoints primary_char_mul_min;
    PrimaryStatsPoints primary_char_mul_max;

    SecondaryStatsPoints secondary_char_mul_min;
    SecondaryStatsPoints secondary_char_mul_max;

    // Skill
    // add
    // @question is primary for skill necessary?
    PrimaryStatsPoints primary_skill_add_min;
    PrimaryStatsPoints primary_skill_add_max;

    SecondaryStatsPoints secondary_skill_add_min;
    SecondaryStatsPoints secondary_skill_add_max;

    // mul
    PrimaryStatsPoints primary_skill_mul_min;
    PrimaryStatsPoints primary_skill_mul_max;

    SecondaryStatsPoints secondary_skill_mul_min;
    SecondaryStatsPoints secondary_skill_mul_max;
};

#endif
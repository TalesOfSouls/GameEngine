/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_ITEM_GEN_STAT_H
#define COMS_MODELS_ITEM_GEN_STAT_H

#include "../../stdlib/Types.h"

// Stats for generating random item
struct EquipmentGenStat {
    v2_int8 potential;

    v2_int8 req_str;
    v2_int8 req_agi;
    v2_int8 req_int;
    v2_int8 req_dex;
    v2_int8 req_acc;
    v2_int8 req_sta;

    // @question Should these limits below also apply to crafting?
    // This is used to limit the total amount of flags per category
    v2_int8 flags_total;
    v2_int8 flags_damage;
    v2_int8 flags_defense;
    v2_int8 flags_other;

    // Character stats (primary)
    v2_int8 flags_primary_total;
    v2_int8 flags_primary_damage;
    v2_int8 flags_primary_defense;
    v2_int8 flags_primary_other;

    // Other (secondary)
    v2_int8 flags_secondary_total;
    v2_int8 flags_secondary_damage;
    v2_int8 flags_secondary_defense;
    v2_int8 flags_secondary_other;

    // Bit field that describes which stats are allowed
    uint64 allowed_stats[3];

    // Allows to define 2 min-max ranges per modifier (most of the time only one is used)
    v4_int8 stats[ITEM_MODIFIER_COUNT];
};

#endif
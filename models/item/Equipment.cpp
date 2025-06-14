/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_EQUIPMENT_C
#define COMS_MODELS_EQUIPMENT_C

#include <stdlib.h>
#include <string.h>

#include "../../stdlib/Types.h"
#include "../mob/monster/LootTable.h"

#include "Equipment.h"
#include "EquipmentType.h"
#include "ItemRarityDefinition.h"
#include "MobLevelStats.h"
#include "_equipment_types.h"
#include "_equipment_slots.h"
#include "_item_rarity.h"

int generate_random_equipment(
    const EquipmentType* equipments, const RarityDefinition* rarities, const MobLevelStats* mob_levels,
    SEquipmentStatsPoints* equipment, int mob_level, byte cclass = 0, int equipment_slot = 0
)
{
    // find random equipment type
    int valid_indices[EQUIPMENT_TYPE_SIZE];
    int valid_count = 0;

    if(cclass != 0 && equipment_slot == 0) {
        for (int i = 0; i < EQUIPMENT_TYPE_SIZE; ++i) {
            if (is_bit_set(equipments[i].char_class, cclass)) {
                valid_indices[valid_count++] = i;
            }
        }
    } else if(cclass != 0 && equipment_slot != 0) {
        for (int i = 0; i < EQUIPMENT_TYPE_SIZE; ++i) {
            if (is_bit_set(equipments[i].char_class, cclass) && equipments[i].slot == equipment_slot) {
                valid_indices[valid_count++] = i;
            }
        }
    } else if (cclass == 0 && equipment_slot == 0) {
        valid_count = EQUIPMENT_TYPE_SIZE;
    } else if(cclass == 0 && equipment_slot != 0) {
        for (int i = 0; i < EQUIPMENT_TYPE_SIZE; ++i) {
            if (equipments[i].slot == equipment_slot) {
                valid_indices[valid_count++] = i;
            }
        }
    }

    const EquipmentType* equipment_type = valid_count == EQUIPMENT_TYPE_SIZE
        ? equipments + (rand() % EQUIPMENT_TYPE_SIZE)
        : equipments + valid_indices[rand() % valid_count];

    // find random item rarity
    int item_rarity = get_random_item_rarity(rarities, RARITY_TYPE_SIZE);

    // find random item drop level
    int item_level = get_random_item_level(mob_levels, mob_level);

    // generate stats
    // requirements
    equipment->requirements.stat_str = equipment_type->primary_item_req_min.stat_str + rand() % (equipment_type->primary_item_req_max.stat_str - equipment_type->primary_item_req_min.stat_str + 1);
    equipment->requirements.stat_agi = equipment_type->primary_item_req_min.stat_agi + rand() % (equipment_type->primary_item_req_max.stat_agi - equipment_type->primary_item_req_min.stat_agi + 1);
    equipment->requirements.stat_int = equipment_type->primary_item_req_min.stat_int + rand() % (equipment_type->primary_item_req_max.stat_int - equipment_type->primary_item_req_min.stat_int + 1);
    equipment->requirements.stat_dex = equipment_type->primary_item_req_min.stat_dex + rand() % (equipment_type->primary_item_req_max.stat_dex - equipment_type->primary_item_req_min.stat_dex + 1);
    equipment->requirements.stat_acc = equipment_type->primary_item_req_min.stat_acc + rand() % (equipment_type->primary_item_req_max.stat_acc - equipment_type->primary_item_req_min.stat_acc + 1);
    equipment->requirements.stat_sta = equipment_type->primary_item_req_min.stat_sta + rand() % (equipment_type->primary_item_req_max.stat_sta - equipment_type->primary_item_req_min.stat_sta + 1);
    equipment->requirements.stat_def = equipment_type->primary_item_req_min.stat_def + rand() % (equipment_type->primary_item_req_max.stat_def - equipment_type->primary_item_req_min.stat_def + 1);

    int primary_indices_random[PRIMARY_STAT_SIZE];
    int secondary_indices_random[SECONDARY_STAT_SIZE];

    int stat_iter;

    // @todo in the area below we only handle the broad definitions, not the details

    // item stats
    // @todo handle item details here now only then apply the remaining free stats

    memcpy(secondary_indices_random, SECONDARY_STAT_INDICES, SECONDARY_STAT_SIZE * sizeof(int));
    random_unique(secondary_indices_random, SECONDARY_STAT_SIZE);

    stat_iter = equipment_type->stats_distribution.item_secondary_count_min + rand() % (equipment_type->stats_distribution.item_secondary_count_max - equipment_type->stats_distribution.item_secondary_count_min + 1);
    for (int i = 0; i < stat_iter; ++i) {
        *(equipment->secondary_item.dmg + secondary_indices_random[i]) = *(equipment_type->secondary_item_min.dmg + secondary_indices_random[i]);
    }

    // char stats
    memcpy(primary_indices_random, PRIMARY_STAT_INDICES, PRIMARY_STAT_SIZE * sizeof(int));
    random_unique(primary_indices_random, PRIMARY_STAT_SIZE);

    stat_iter = equipment_type->stats_distribution.char_primary_count_min + rand() % (equipment_type->stats_distribution.char_primary_count_max - equipment_type->stats_distribution.char_primary_count_min + 1);
    for (int i = 0; i < stat_iter; ++i) {
        // add and mul are equally distributed
        if (fast_rand1() < FAST_RAND_MAX / 2) {
            *(&equipment->primary_char_add.stat_str + primary_indices_random[i]) = *(&equipment_type->primary_char_add_min.stat_str + primary_indices_random[i]);
        } else {
            *(&equipment->primary_char_mul.stat_str + primary_indices_random[i]) = *(&equipment_type->primary_char_mul_min.stat_str + primary_indices_random[i]);
        }
    }

    // @todo handle char_secondary_distribution skill_count_min/max here now

    memcpy(secondary_indices_random, SECONDARY_STAT_INDICES, SECONDARY_STAT_SIZE * sizeof(int));
    random_unique(secondary_indices_random, SECONDARY_STAT_SIZE);

    stat_iter = equipment_type->stats_distribution.char_secondary_count_min + rand() % (equipment_type->stats_distribution.char_secondary_count_max - equipment_type->stats_distribution.char_secondary_count_min + 1);
    for (int i = 0; i < stat_iter; ++i) {
        // add and mul are equally distributed
        if (fast_rand1() < FAST_RAND_MAX / 2) {
            *(equipment->secondary_char_add.dmg + secondary_indices_random[i]) = *(equipment_type->secondary_char_add_min.dmg + secondary_indices_random[i]);
        } else {
            *(equipment->secondary_char_mul.dmg + secondary_indices_random[i]) = *(equipment_type->secondary_char_mul_min.dmg + secondary_indices_random[i]);
        }
    }

    // @todo handle remaining char_count_min/max here now

    // skill
    // @question is primary for skill necessary?
    memcpy(primary_indices_random, PRIMARY_STAT_INDICES, PRIMARY_STAT_SIZE * sizeof(int));
    random_unique(primary_indices_random, PRIMARY_STAT_SIZE);

    stat_iter = equipment_type->stats_distribution.skill_primary_count_min + rand() % (equipment_type->stats_distribution.skill_primary_count_max - equipment_type->stats_distribution.skill_primary_count_min + 1);
    for (int i = 0; i < stat_iter; ++i) {
        // add and mul are equally distributed
        if (fast_rand1() < FAST_RAND_MAX / 2) {
            *(&equipment->primary_skill_add.stat_str + primary_indices_random[i]) = *(&equipment_type->primary_skill_add_min.stat_str + primary_indices_random[i]);
        } else {
            *(&equipment->primary_skill_mul.stat_str + primary_indices_random[i]) = *(&equipment_type->primary_skill_mul_min.stat_str + primary_indices_random[i]);
        }
    }

    // @todo handle skill_secondary_distribution skill_count_min/max here now

    memcpy(secondary_indices_random, SECONDARY_STAT_INDICES, SECONDARY_STAT_SIZE * sizeof(int));
    random_unique(secondary_indices_random, SECONDARY_STAT_SIZE);

    stat_iter = equipment_type->stats_distribution.skill_secondary_count_min + rand() % (equipment_type->stats_distribution.skill_secondary_count_max - equipment_type->stats_distribution.skill_secondary_count_min + 1);
    for (int i = 0; i < stat_iter; ++i) {
        // add and mul are equally distributed
        if (fast_rand1() < FAST_RAND_MAX / 2) {
            *(equipment->secondary_skill_add.dmg + secondary_indices_random[i]) = *(equipment_type->secondary_skill_add_min.dmg + secondary_indices_random[i]);
        } else {
            *(equipment->secondary_skill_mul.dmg + secondary_indices_random[i]) = *(equipment_type->secondary_skill_mul_min.dmg + secondary_indices_random[i]);
        }
    }

    // @todo handle remaining skill_count_min/max here now

    return equipment_type->slot;
}

int generate_random_equipment(SEquipmentStatsPoints* equipment, int mob_level, byte cclass, const LootTable* table)
{
    return -1;
}

#endif
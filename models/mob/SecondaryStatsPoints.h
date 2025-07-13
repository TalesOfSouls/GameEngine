/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_MOB_SECONDARY_STATS_POINTS_H
#define COMS_MODELS_MOB_SECONDARY_STATS_POINTS_H

#include "../../stdlib/Types.h"
#include "MobStatsType.h"

#define SECONDARY_STAT_SIZE 90
static const int SECONDARY_STAT_INDICES[] = {
    0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
    40, 41, 42, 43, 44, 45, 46, 47, 48, 49,
    50, 51, 52, 53, 54, 55, 56, 57, 58, 59,
    60, 61, 62, 63, 64, 65, 66, 67, 68, 69,
    70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89,
};

/**
 * @todo optimize order of struct members to ensure optimal struct size
 */

// Character stats modifiable through skill tree?
struct SecondaryStatsPoints {
    /*
    @todo
    Composite damage types would allow us to combine skills of different players (e.g. arrow flies through fire -> adds fire damage flag)
    Obviously this doesn't increase the damage directly but can have a positive impact if the enemy has low fire resistance for example

    @question what happens if a skill has two flags (fire&slashing) and the enemy has high resistance vs slashing.
        Does this mean the damage is reduced, does it reduce by "50%" or does it only reduce the min of fire&slashing resistance.
            -> if you have no slashing resistance you still take full damage
    */

    // Damage types
    // This allows us to create skills with multiple additive damage types AND composite damage that has multiple types at the same time
    uint16 dmg[MOB_STATS_TYPE_SIZE];

    // @question is this a damage number or is this a % number of the total damage?
    uint16 dmg_crit;
    uint16 dmg_crit_chance;

    // @question is this similar to the different damage categories, is this a % of the total damage or should this just be a flag
    uint16 dmg_piercing;

    // Health & Resource
    uint16 health;
    uint16 health_on_dmg_dealt;
    uint16 health_on_dmg_taken;

    uint16 health_regen;
    uint16 health_regen_rel;
    uint16 health_regen_on_dmg_dealt;
    uint16 health_regen_on_dmg_taken;

    uint16 resource;
    uint16 resource_on_dmg_dealt;
    uint16 resource_on_dmg_taken;

    uint16 resource_regen;
    uint16 resource_regen_rel;
    uint16 resource_regen_on_dmg_dealt;
    uint16 resource_regen_on_dmg_taken;

    uint16 resource_loss;
    uint16 resource_loss_on_dmg_dealt;
    uint16 resource_loss_on_dmg_taken;

    // Defense types (resistances, armor, or whatever you want to call it)
    uint16 defense[MOB_STATS_TYPE_SIZE];

    // Accuracy
    uint16 block_chance;
    uint16 block_amount;

    uint16 dmg_reflection;
    uint16 dmg_reflection_chance;

    uint16 dodge_chance;
    uint16 cc_protection;
    uint16 miss_chance;

    // Movement
    // Additional speeds may be defined for Mobs
    uint16 speed_walk1;
    uint16 speed_swim1;
    uint16 speed_fly1;

    // Fighting speed
    uint16 speed_cast;
    uint16 speed_attack;

    uint16 pickup_range;

    uint16 shield;

    // modifier
    uint16 aoe_scale;
    uint16 resource_cost;
    uint16 health_cost;
    uint16 attack_range;
    uint16 melee_range;
    uint16 projectile_speed;
    uint16 projectile_count;
    uint16 shatter_probability;
    uint16 shatter_range;
    uint16 shatter_dmg;
    uint16 shatter_count;
    uint16 passthrough_damage;
    uint16 passthrough_count;
    uint16 dot_duration;
    uint16 dot_count;
    uint16 bleeding_dot;
    uint16 poison_dot;
    uint16 burn_dot;
    uint16 ice_dot;
    uint16 resource_drain;
    uint16 shatter_dot;
    uint16 minion_duration;
    uint16 minion_count;
    uint16 effect_spreading_probability;
    uint16 effect_spreading_radius;
    uint16 effect_spreading_max_count;
    uint16 effect_duration;
    uint16 aura_range;
    uint16 cast_duration;

    // special
    uint16 aggro_range;
};

struct SecondaryStatsPoints2 {
    /*
    @todo
    Composite damage types would allow us to combine skills of different players (e.g. arrow flies through fire -> adds fire damage flag)
    Obviously this doesn't increase the damage directly but can have a positive impact if the enemy has low fire resistance for example

    @question what happens if a skill has two flags (fire&slashing) and the enemy has high resistance vs slashing.
        Does this mean the damage is reduced, does it reduce by "50%" or does it only reduce the min of fire&slashing resistance.
            -> if you have no slashing resistance you still take full damage
    */

    // Damage types
    // This allows us to create skills with multiple additive damage types AND composite damage that has multiple types at the same time
    byte damage[3];
    byte damage_flag[3 * 5]; // 3 * 5 = 15, every damage component can have up to 3 damage types and a limited amount from others

    byte dmg_reflection;
    byte dmg_reflection_chance;

    // @question is this a damage number or is this a % number of the total damage?
    byte dmg_crit;
    byte dmg_crit_chance;

    // @question is this similar to the different damage categories, is this a % of the total damage or should this just be a flag
    byte dmg_piercing;

    // Health & Resource
    byte health;
    byte health_on_dmg_dealt;
    byte health_on_dmg_taken;

    byte health_regen;
    byte health_regen_rel;
    byte health_regen_on_dmg_dealt;
    byte health_regen_on_dmg_taken;

    byte resource;
    byte resource_on_dmg_dealt;
    byte resource_on_dmg_taken;

    byte resource_regen;
    byte resource_regen_rel;
    byte resource_regen_on_dmg_dealt;
    byte resource_regen_on_dmg_taken;

    byte resource_loss;
    byte resource_loss_on_dmg_dealt;
    byte resource_loss_on_dmg_taken;

    // Defense types (resistances, armor, or whatever you want to call it)
    byte defense[MOB_STATS_TYPE_SIZE];

    // Accuracy
    byte block_chance;
    byte block_amount;

    byte dodge_chance;
    byte cc_protection;
    byte miss_chance;

    // Movement
    // Additional speeds may be defined for Mobs
    byte speed_walk1;
    byte speed_swim1;
    byte speed_fly1;

    // Fighting speed
    byte speed_cast;
    byte speed_attack;

    byte pickup_range;

    byte shield;

    // modifier
    byte aoe_scale;
    byte resource_cost;
    byte health_cost;
    byte attack_range;
    byte melee_range;
    byte projectile_speed;
    byte projectile_count;
    byte shatter_probability;
    byte shatter_range;
    byte shatter_dmg;
    byte shatter_count;
    byte passthrough_damage;
    byte passthrough_count;
    byte dot_duration;
    byte dot_count;
    byte bleeding_dot;
    byte poison_dot;
    byte burn_dot;
    byte ice_dot;
    byte resource_drain;
    byte shatter_dot;
    byte minion_duration;
    byte minion_count;
    byte effect_spreading_probability;
    byte effect_spreading_radius;
    byte effect_spreading_max_count;
    byte effect_duration;
    byte aura_range;
    byte cast_duration;

    // special
    byte aggro_range;
};

struct SecondaryStatsRelPoints2 {
    // Damage types
    byte damage[3];

    byte dmg_reflection;
    byte dmg_reflection_chance;

    byte dmg_crit;
    byte dmg_crit_chance;

    byte dmg_piercing;

    // Health & Resource
    byte health;
    byte health_on_dmg_dealt;
    byte health_on_dmg_taken;

    byte health_regen;
    byte health_regen_on_dmg_dealt;
    byte health_regen_on_dmg_taken;

    byte resource;
    byte resource_on_dmg_dealt;
    byte resource_on_dmg_taken;

    byte resource_regen;
    byte resource_regen_on_dmg_dealt;
    byte resource_regen_on_dmg_taken;

    byte resource_loss;
    byte resource_loss_on_dmg_dealt;
    byte resource_loss_on_dmg_taken;

    // Defense types
    //      think about it as armor and/or resistance if it helps
    byte defense[MOB_STATS_TYPE_SIZE];

    // Accuracy
    byte block_chance;
    byte block_amount;

    byte dodge_chance;
    byte cc_protection;
    byte miss_chance;

    // Movement
    // Additional speeds may be defined for Mobs
    byte speed_walk1;
    byte speed_swim1;
    byte speed_fly1;

    // Fighting speed
    byte speed_cast;
    byte speed_attack;

    byte pickup_range;

    byte shield;

    byte aoe_scale;
    byte resource_cost;
    byte health_cost;
    byte attack_range;
    byte melee_range;
    byte projectile_speed;
    byte projectile_count;
    byte shatter_probability;
    byte shatter_range;
    byte shatter_dmg;
    byte shatter_count;
    byte passthrough_damage;
    byte passthrough_count;
    byte dot_duration;
    byte dot_count;
    byte bleeding_dot;
    byte poison_dot;
    byte burn_dot;
    byte ice_dot;
    byte resource_drain;
    byte shatter_dot;
    byte minion_duration;
    byte minion_count;
    byte effect_spreading_probability;
    byte effect_spreading_radius;
    byte effect_spreading_max_count;
    byte effect_duration;
    byte aura_range;
    byte cast_duration;

    byte aggro_range;
};

struct SecondaryStatsValues {
};

struct SecondaryStatsRelValues {
};

#endif
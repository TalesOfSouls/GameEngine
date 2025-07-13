#ifndef COMS_MODELS_SKILL_H
#define COMS_MODELS_SKILL_H

#include "../../../stdlib/Types.h"
#include "ProjectileDistribution.h"
#include "SkillLocation.h"
#include "AoeDistribution.h"
#include "StatsTarget.h"
#include "../MobStats.h"

#define MAX_SKILL_NAME 32
#define MAX_SKILL_DESCRIPTION 128

/**
 * @todo optimize order of struct members to ensure optimal struct size
 */

// @todo We adjusted SMobStats -> we can remove a lot of variables below
struct Skill
{
    // not required for server
    // const char name[MAX_SKILL_NAME];
    // const char description[MAX_SKILL_DESCRIPTION];

    // @todo implement charged skills
    //      e.g. you gain one charge every: x seconds, x mob kills, x dmg, ...
    //      max charges
    //      you can then use these charges
    //      -> we could then have things that also reduce charges

    int id;

    // @todo animations
    void* animation_casting;
    void* animation_channeling;
    void* icon;

    // @todo e.g. attack command, movement command, etc. for totems and minions
    void* commands;

    // @todo configuration e.g. which minions to pick, release minion, select new minion...
    void* options;

    bool projectile_animation_uses_item; // Use the equiped item for the projectile

    byte target; // what is targatable (self, enemies, allies, ground, none?) Has to be bitset 2^n to define multiple
    byte default_target;

    bool has_hitbox; // some skills have hitboxes (e.g. totems, minions, some breakable prisons, ...)

    // @todo how to assign life and resource to skill (e.g. life to destroyable prison, totems or resources to minions)

    f32 visual_scale; // make visual larger/smaller

    // Scales skill over channeling time (e.g. makes balls larger and after reaching peak makes them smaller)
    byte scale_type;

    // the following scale factors can be considered key frames
    f32 scale1;
    f32 scale2;
    f32 scale3;

    // @todo when to use points and when to use values?
    // Stats of the skill itself
    SecondaryStatsPoints skill_stats;
    StatsTarget skill_target;

    // @todo when to use points and when to use values?
    // Modifies stats
    // You can have 2 stats for 2 target types (e.g. you could create a buff and debuff in one skill)
    // 1
    PrimaryStatsPoints stats1_primary_add;
    PrimaryStatsPoints stats1_primary_mul;

    SecondaryStatsPoints stats1_secondary_add;
    SecondaryStatsPoints stats1_secondary_mul;
    StatsTarget stats1_target;

    // 2
    PrimaryStatsPoints stats2_primary_add;
    PrimaryStatsPoints stats2_primary_mul;

    SecondaryStatsPoints stats2_secondary_add;
    SecondaryStatsPoints stats2_secondary_mul;
    StatsTarget stats2_target;

    // Modifiers
    // Char
    PrimaryStatsPoints primary_char_add;
    PrimaryStatsPoints primary_char_mul;

    SecondaryStatsPoints secondary_char_add;
    SecondaryStatsPoints secondary_char_mul;

    // Item
    PrimaryStatsPoints primary_item_add;
    PrimaryStatsPoints primary_item_mul;

    SecondaryStatsPoints secondary_item_add;
    SecondaryStatsPoints secondary_item_mul;

    int skill_movement; // none, follows target, random moevement, random movement in aoe
    // @todo how to make specific custom movement pattern for boss fights

    // Useful to cast multiple totems:
    byte count; // how often is the skill executed @question can this also be used as tick count for heals? or is this in the MobStats?
    byte count_delay; // delay between executions
    byte count_manual; // you can manually perform the skill count * times. Manual = wherever you want.
    byte count_radius; // radius in which they are randomly distributed
    byte count_distance; // distance between sub skills
    // @todo maybe combine with aoe? we could remove radius and distance and just use them from aoe

    // General
    int resource_cost;
    int health_cost;

    bool is_range;
    void* attack_anim;
    int movement_pattern; // the skill moves in a specific pattern (e.g. straight line, random, circular motion, left/right wave, ...)

    bool is_melee;

    bool async_cast; // Can be casted with other skills at the same time/another skill can be cast while casting this

    // Projectile data
    bool is_projectile;
    int projectile_shape; // 3d Model

    int projectile_distribution;
    bool projectile_angle; // Projectiles fan out from start point

    // Either creates new sub-projectiles on shatter OR
    // shatter is basically a second hit
    bool shatter;
    int shatter_projectiles;
    f32 shatter_duration;
    uint32 shatter_damage;

    bool is_retargatable; // If multiple you may be able to switch targets in between
    bool is_boomerang;

    // Casting happens before the skill
    // This can also be used for telegraphs
    bool is_cast_cancalable;
    bool is_cast_disruptable;
    bool is_cast_movable;

    // Channeling happens during the skill.
    // even a normal punch attack is a channelled skill
    f32 channeling_duration; // -1.0 = infinite (e.g. aura), 0 = no duration = instant
    bool is_channeling; // duration = skill_duration + it shows the channeling timeline
    bool is_channeling_cancalable;
    bool is_channeling_disruptable;
    bool is_channeling_movable;

    byte channeling_distribution; // beginning, on hit, end, intervals, or use damage distribution -> also effects dmg if channeling is cancled
    byte channeling_ticks; // how often is dmg dealt

    byte damage_distribution_location; // const, linear-increase, linear-decrease according to the shape of the skill.. e.g. more damage in center of aoe or more damage at the end of a end of a wave like skill
    byte damage_distribution_time; // similar to above but it might increase over time not based on location

    // AOE
    bool is_aoe;
    int aoe_shape; // circle, square, donut, movement (e.g. fire trail on dodge)
    AoeDistribution aoe_fill_pattern;
    f32 aoe_dim1;
    f32 aoe_dim2;
    f32 aoe_dim3;
    int aoe_shatter;
    SkillLocation aoe_location;
    byte aoe_apply; // Applies while in aoe, applies even after moving out of aoe

    // DOT
    byte dot_state; // only when moving, standing still, always
    byte dot_effective; // always; on move; on stand still

    byte dot_buff_effective; // buff modifier (e.g. increase by/decrease by)
    float dot_buff; // buff dot when on move or on standstill

    // Minion (and totems)
    int minion_type;
    int minion_summon_count; // How many summons per summon cast @question still required with general skill count? I don't think so.

    // Effects
    bool is_effect_spreading;
    f32 effect_spreading_probability;
    f32 effect_spreading_radius; // What is the maximum distance for spreading
    f32 effect_spreading_max_count; // How many mobs can be effected at maximum
    f32 effect_duration;

    // Push/pull
    byte movement_type; // push, pull, dodge, jump, sprint, fly
    byte movement_distance;
    byte movement_direction; // e.g. target, view/orientation, opposite of view/orientation
    byte movement_shape; // circle, pov = fan like; > 0 = push, < 0 = pull
    // @todo maybe use location change? this way we could also implement a teleport

    byte mob_visibility; // = e.g. sneak skill

    // @todo skill jumps over to close by enemies = chain lightning

    // @todo illusion skill that multiplies the targeted mob and also shuffels the position

    // example: limits_movement = 0 + skill_disabled 0xFFFFFFFF = all = stun
    byte limits_movement; // limits_movement @question maybe use character stat and set movement to -INF
    uint32 limits_skills; // skill types disabled (bit field required), 0 = none
    // @todo how to implement skill limits only within certain range?
    //  maybe just set it to is_aoe?!?!

    byte locks_stat_type; // 0 = none, 1 for example could be health = useful to make someone invulnerable

    // @todo how to implement fear? make mobs run away a short distance.
    // maybe somhow use push? because it is like a push but by making the mob walk.

    // @todo how to implement a skill that shoots multiple large ball projectiles in a radial or linar pattern
    // from the caster with random distances in between each other but also in waves.
    // e.g. first wave sends out x balls , then second wave sends x balls but in different locations

    // @todo how to implement a beam? e.g. arcane beam or mana drain? maybe a type of projectile?
};

#endif
#ifndef COMS_MODELS_ITEM_RARITY_H
#define COMS_MODELS_ITEM_RARITY_H

#include "../../stdlib/Stdlib.h"

enum ItemRarity : uint8 {
    ITEM_RARITY_NONE,
    ITEM_RARITY_WHITE,
    ITEM_RARITY_BLUE,
    ITEM_RARITY_YELLOW,
    ITEM_RARITY_ORANGE,
    ITEM_RARITY_GREEN,
    ITEM_RARITY_PURPLE,
    ITEM_RARITY_RED,
    ITEM_RARITY_BLACK,
    ITEM_RARITY_SIZE,
};

union RarityDropChance {
    f32 rarity[9];
    struct {
        f32 none;
        f32 white;
        f32 blue;
        f32 yellow;
        f32 orange;
        f32 green;
        f32 purple;
        f32 red;
        f32 black;
    }
};

union RarityDropCount {
    v2_int8 rarity[9];
    struct {
        v2_int8 none;
        v2_int8 white;
        v2_int8 blue;
        v2_int8 yellow;
        v2_int8 orange;
        v2_int8 green;
        v2_int8 purple;
        v2_int8 red;
        v2_int8 black;
    }
};

struct OpenWorldRarityDropList {
    // This allows us to also define drops from chests and chest like containers
    RarityDropChance normal_chest;
    RarityDropChance rare_chest;
    RarityDropChance ultra_rare_chest;

    // Mobs
    RarityDropChance regular;
    RarityDropChance elite;
    RarityDropChance champion;
    RarityDropChance boss;
    RarityDropChance world_boss;
    RarityDropChance singularity;
};

struct RaidRarityDropList {
    // This allows us to also define drops from chests and chest like containers
    RarityDropChance normal_chest;
    RarityDropChance rare_chest;
    RarityDropChance ultra_rare_chest;

    // Mobs
    RarityDropChance regular;
    RarityDropChance elite;
    RarityDropChance champion;
    RarityDropChance boss;
    RarityDropChance final_boss;
};
typedef RaidRarityDropList DungeonRarityDropList;

struct PvPRarityDropList {
    // Rank
    RarityDropChance novice;
    RarityDropChance adventurer;
    RarityDropChance veteran;
    RarityDropChance legendary;
    RarityDropChance vooid;
    RarityDropChance infernal;
};
typedef PvPRarityDropList PuzzleRarityDropList;
typedef PvPRarityDropList RaceRarityDropList;

struct TournamentRarityDropList {
    RarityDropChance loss; // loss in first game
    RarityDropChance win; // win in lower rounds
    RarityDropChance quarter;
    RarityDropChance semi;
    RarityDropChance final;
};

#endif
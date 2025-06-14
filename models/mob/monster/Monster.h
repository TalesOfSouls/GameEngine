/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_MOB_MONSTER_H
#define COMS_MODELS_MOB_MONSTER_H

#include "../../../stdlib/Types.h"

#include "../Mob.h"
#include "LootTable.h"
#include "MonsterStats.h"

#if SERVER
    struct SMonster {
        Mob mob;
        SMonsterStats monster_stats;

        LootTable loot;

        // Data layout
        // 12223444
        //      1: scale sign
        //      2: scale factor (8)
        //      3: weight sign
        //      4: weight factor (8)
        byte custom_size;

        // Data layout
        // 122222??
        //      1: body type (2)
        //      2: skin color (32)
        byte custom_body;
    };
#endif

struct CMonster {
    Mob mob;
    CMonsterStats monster_stats;

    char name[MAX_CHAR_NAME_LENGTH];
    char title[MAX_CHAR_TITLE_LENGTH];

    // Data layout
    // 12223444
    //      1: scale sign
    //      2: scale factor (8)
    //      3: weight sign
    //      4: weight factor (8)
    byte custom_size;

    // Data layout
    // 1122222?
    //      1: body type (4)
    //      2: skin color (32)
    byte custom_body;
};

#endif
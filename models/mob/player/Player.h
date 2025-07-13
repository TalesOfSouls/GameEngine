/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_MOB_PLAYER_H
#define COMS_MODELS_MOB_PLAYER_H

#include "../../../stdlib/Types.h"

#include "../../item/Equipment.h"
#include "../Mob.h"
#include "../MobState.h"
#include "../monster/LootTable.h"
#include "Backpack.h"
#include "../MobStats.h"

#ifndef MAX_CHAR_NAME_LENGTH
    #define MAX_CHAR_NAME_LENGTH 32
#endif

#ifndef MAX_CHAR_TITLE_LENGTH
    #define MAX_CHAR_TITLE_LENGTH 16
#endif

#if SERVER
    struct SPlayer {
        Mob mob;
        SMobStatsPoints player_stats;

        char name[MAX_CHAR_NAME_LENGTH];
        char title[MAX_CHAR_TITLE_LENGTH];

        Equipment equipment[14];

        PrimaryStatsPoints stat_points;

        PSettings settings;

        Backpack* packpack;

        // Data layout
        // 12223444
        //      1: scale sign
        //      2: scale factor (8)
        //      3: weight sign
        //      4: weight factor (8)
        byte scale;
        byte weight;

        // Data layout
        // 1122222?
        //      1: body type (4)
        //      2: skin color (32)
        byte body_type;
        byte body_color;

        // Data layout
        // 1-4 race (16)
        //
        // 5-9 face type (32)
        //
        // 10-15 hair style (64)
        // 16-20 hair color (32)
        //
        // 21-26 beard style (32)
        //
        // 27-30 eye style (32)
        // 31-35 eye color (32)
        //
        // 36-40 face scar (32)
        // 41-45 body scar (32)
        //
        // 46-51 tattoo (64)
        // 52-56 tattoo color (32)
        byte race;
        byte face_type;
        byte hair_style;
        byte hair_color;
        byte beard_style;
        byte eye_style;
        byte eye_color;
        byte face_scar;
        byte body_scar;
        byte tattoo;
        byte tattoo_color;
    };
#endif

struct CPlayer {
    Mob mob;
    SMobStatsPoints player_stats;

    char name[MAX_CHAR_NAME_LENGTH];
    char title[MAX_CHAR_TITLE_LENGTH];

    // Equipment id used for rendering (this could be also the transmog item id)
    uint32 equipment[14];

    // Equipment transmog data
    // 11111222223333344444444455555???
    //      1: primary color (32)
    //      2: secondary color (32)
    //      3: tertiary color (32)
    //      4: effect (512)
    //      5: effect color (32)
    //      ?: FREE
    uint32 equipmentTransmog[14];

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

    // Data layout
    // 1-4 race (16)
    //
    // 5-9 face type (32)
    //
    // 10-15 hair style (64)
    // 16-20 hair color (32)
    //
    // 21-26 beard style (32)
    //
    // 27-30 eye style (32)
    // 31-35 eye color (32)
    //
    // 36-40 face scar (32)
    // 41-45 body scar (32)
    //
    // 46-51 tattoo (64)
    // 52-56 tattoo color (32)
    int64 customization;
};

#endif
#ifndef COMS_NETWORK_PACKET_MOB_PLAYER_INFO_H
#define COMS_NETWORK_PACKET_MOB_PLAYER_INFO_H

#include <stdio.h>

#include "../../../../stdlib/Types.h"
#include "../../../../config.h"

struct PlayerInfoPacketSnapshot {
    byte* data;
};

// 32+32+256+128+8+8+7+7+7+7+56+406+448+64=184bytes
struct PlayerInfoPacketSnapshotUnpacked {
    uint32 mob_id;
    uint32 chunk;
    byte name[MAX_CHAR_NAME_LENGTH];
    byte title[MAX_CHAR_TITLE_LENGTH];
    byte xp;

    // Data layout
    // 12223444
    //      1: scale sign
    //      2: scale factor (8)
    //      3: weight sign
    //      4: weight factor (8)
    byte scale;
    byte weight;

    byte level; // 2^7
    byte health; // 2^7
    byte resource; // 2^7

    // Data layout 2^7
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

    // Equipment transmog data
    // 11111222223333344444444455555???
    //      1: primary color (32)
    //      2: secondary color (32)
    //      3: tertiary color (32)
    //      4: effect (512)
    //      5: effect color (32)
    //      ?: FREE
    uint32 equipmentTransmog[14];
    uint32 equipment[14];

    uint64 time;
};

#endif
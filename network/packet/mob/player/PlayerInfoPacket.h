/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_PACKET_MOB_PLAYER_INFO_H
#define COMS_NETWORK_PACKET_MOB_PLAYER_INFO_H

#include "../../../../stdlib/Stdlib.h"
#include "../../../../utils/BitUtils.h"
#include "../MobInfoPacket.h"

/**
 * Player looks (name, title, customization, equipment), sent when a player
 * becomes visible or changes its looks (PACKET_TYPE_MOB_LOOKS)
 *
 * Strings are only sent with their used length, empty equipment slots cost 1 bit.
 *
 *      field               bits                    unpacked
 *      mob_id, chunk       64                      64
 *      name                5 + 8 * length          256
 *      title               4 + 8 * length          128
 *      xp, scale, weight   8 + 4 + 4               24
 *      level/health/res.   3 x 7                   24
 *      body                2 + 5                   16
 *      race                4                       8
 *      face/hair/beard     5 + 6 + 5 + 5           32
 *      eye                 5 + 5                   16
 *      scars               5 + 5                   16
 *      tattoo              6 + 5                   16
 *      transmog            14 x 29                 14 x 64
 *      equipment           14 x (1 [+ 32])         14 x 32
 *      ----------------------------------------------------
 *      fully equipped, 6 character name, no title: 1096 bits (137 bytes) instead of 244 bytes
 */

#define PLAYER_NAME_LENGTH 32
#define PLAYER_TITLE_LENGTH 16
#define PLAYER_EQUIPMENT_SLOTS 14

struct PlayerTransmog {
    // 32 colors each
    uint8 primary_color;
    uint8 secondary_color;
    uint8 tertiary_color;

    // 512 effects
    uint16 effect;
    uint8 effect_color;
};

template <typename Stream>
inline
bool net_packet_serialize(Stream* stream, PlayerTransmog* msg) NO_EXCEPT
{
    if (!serialize_uint(stream, &msg->primary_color, 5)
        || !serialize_uint(stream, &msg->secondary_color, 5)
        || !serialize_uint(stream, &msg->tertiary_color, 5)
        || !serialize_uint(stream, &msg->effect, 9)
        || !serialize_uint(stream, &msg->effect_color, 5)
    ) {
        return false;
    }

    return true;
}

struct PlayerInfo {
    uint32 mob_id;
    uint32 chunk;

    char name[PLAYER_NAME_LENGTH];
    char title[PLAYER_TITLE_LENGTH];

    uint8 xp;
    int8 scale;
    int8 weight;

    uint8 level;
    uint8 health;
    uint8 resource;

    uint8 body_type;
    uint8 body_color;

    uint8 race;         // 16
    uint8 face_type;    // 32
    uint8 hair_style;   // 64
    uint8 hair_color;   // 32
    uint8 beard_style;  // 32
    uint8 eye_style;    // 32
    uint8 eye_color;    // 32
    uint8 face_scar;    // 32
    uint8 body_scar;    // 32
    uint8 tattoo;       // 64
    uint8 tattoo_color; // 32

    PlayerTransmog transmog[PLAYER_EQUIPMENT_SLOTS];

    // Item ids, 0 = empty slot
    uint32 equipment[PLAYER_EQUIPMENT_SLOTS];
};

template <typename Stream>
inline
bool net_packet_serialize(Stream* stream, PlayerInfo* msg) NO_EXCEPT
{
    if (!serialize_uint(stream, &msg->mob_id, 32)
        || !serialize_uint(stream, &msg->chunk, 32)
        || !serialize_string(stream, msg->name, PLAYER_NAME_LENGTH)
        || !serialize_string(stream, msg->title, PLAYER_TITLE_LENGTH)
    ) {
        return false;
    }

    if (!serialize_uint(stream, &msg->xp, 8)
        || !serialize_int_range(stream, &msg->scale, -MOB_SIZE_FACTOR_MAX, MOB_SIZE_FACTOR_MAX)
        || !serialize_int_range(stream, &msg->weight, -MOB_SIZE_FACTOR_MAX, MOB_SIZE_FACTOR_MAX)
    ) {
        return false;
    }

    if (!serialize_uint(stream, &msg->level, MOB_STAT_BITS)
        || !serialize_uint(stream, &msg->health, MOB_STAT_BITS)
        || !serialize_uint(stream, &msg->resource, MOB_STAT_BITS)
    ) {
        return false;
    }

    if (!serialize_uint(stream, &msg->body_type, MOB_BODY_TYPE_BITS)
        || !serialize_uint(stream, &msg->body_color, MOB_COLOR_BITS)
    ) {
        return false;
    }

    if (!serialize_uint(stream, &msg->race, 4)
        || !serialize_uint(stream, &msg->face_type, 5)
        || !serialize_uint(stream, &msg->hair_style, 6)
        || !serialize_uint(stream, &msg->hair_color, 5)
        || !serialize_uint(stream, &msg->beard_style, 5)
        || !serialize_uint(stream, &msg->eye_style, 5)
        || !serialize_uint(stream, &msg->eye_color, 5)
        || !serialize_uint(stream, &msg->face_scar, 5)
        || !serialize_uint(stream, &msg->body_scar, 5)
        || !serialize_uint(stream, &msg->tattoo, 6)
        || !serialize_uint(stream, &msg->tattoo_color, 5)
    ) {
        return false;
    }

    for (int32 i = 0; i < PLAYER_EQUIPMENT_SLOTS; ++i) {
        if (!net_packet_serialize(stream, &msg->transmog[i])) {
            return false;
        }
    }

    for (int32 i = 0; i < PLAYER_EQUIPMENT_SLOTS; ++i) {
        bool equipped = msg->equipment[i] != 0;
        if (!serialize_bool(stream, &equipped)) {
            return false;
        }

        if (equipped) {
            if (!serialize_uint(stream, &msg->equipment[i], 32)) {
                return false;
            }
        } else {
            msg->equipment[i] = 0;
        }
    }

    return true;
}

#endif

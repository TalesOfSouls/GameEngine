/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_PACKET_MOB_INFO_H
#define COMS_NETWORK_PACKET_MOB_INFO_H

#include "../../../stdlib/Stdlib.h"
#include "../../../utils/BitUtils.h"

/**
 * Mob info (level, health, size, body), sent when it changes
 *
 *      field       bits    unpacked
 *      mob_id      32      32
 *      mob_type    8       8
 *      chunk       32      32
 *      level       7       8
 *      health      7       8
 *      resource    7       8
 *      xp          8       8
 *      scale       4       8       [-7, 7]
 *      weight      4       8       [-7, 7]
 *      body_type   2       8       4 types
 *      skin_color  5       8       32 colors
 *      ------------------------------
 *      total       116 bits (15 bytes) instead of 20 bytes
 */

#define MOB_STAT_BITS 7
#define MOB_SIZE_FACTOR_MAX 7
#define MOB_BODY_TYPE_BITS 2
#define MOB_COLOR_BITS 5

struct MobInfo {
    uint32 mob_id;
    uint8 mob_type;
    uint32 chunk;

    // 0 - 127
    uint8 level;
    uint8 health;
    uint8 resource;
    uint8 xp;

    // [-MOB_SIZE_FACTOR_MAX, MOB_SIZE_FACTOR_MAX]
    int8 scale;
    int8 weight;

    uint8 body_type;
    uint8 skin_color;
};

template <typename Stream>
inline
bool net_packet_serialize(Stream* stream, MobInfo* msg) NO_EXCEPT
{
    if (!serialize_uint(stream, &msg->mob_id, 32)
        || !serialize_uint(stream, &msg->mob_type, 8)
        || !serialize_uint(stream, &msg->chunk, 32)
        || !serialize_uint(stream, &msg->level, MOB_STAT_BITS)
        || !serialize_uint(stream, &msg->health, MOB_STAT_BITS)
        || !serialize_uint(stream, &msg->resource, MOB_STAT_BITS)
        || !serialize_uint(stream, &msg->xp, 8)
        || !serialize_int_range(stream, &msg->scale, -MOB_SIZE_FACTOR_MAX, MOB_SIZE_FACTOR_MAX)
        || !serialize_int_range(stream, &msg->weight, -MOB_SIZE_FACTOR_MAX, MOB_SIZE_FACTOR_MAX)
        || !serialize_uint(stream, &msg->body_type, MOB_BODY_TYPE_BITS)
        || !serialize_uint(stream, &msg->skin_color, MOB_COLOR_BITS)
    ) {
        return false;
    }

    return true;
}

#endif

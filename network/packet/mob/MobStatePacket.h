/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_NETWORK_PACKET_MOB_STATE_H
#define COMS_NETWORK_PACKET_MOB_STATE_H

#include "../../../stdlib/Stdlib.h"
#include "../../../utils/BitUtils.h"

/**
 * Mob state (position, rotation, state flags), the most frequent game message
 *
 * The struct is the unpacked form used by the game, net_packet_serialize() packs/unpacks it
 * (utils/BitUtils.h). Sent in a NetBatch (NetBatch.h) on the unreliable channel:
 * the next snapshot replaces a lost one.
 *
 *      field       bits    unpacked
 *      mob_id      32      32
 *      mob_type    8       8
 *      chunk       32      32
 *      position    3 x 13  3 x 32  (chunk relative, precision ~0.002)
 *      rotation    29      128     (smallest three quaternion)
 *      state_flag  32      32
 *      ------------------------------
 *      snapshot    172 bits (22 bytes) instead of 64 bytes
 *
 * A delta only contains the fields that changed compared to a snapshot both sides
 * know (e.g. the last acknowledged one).
 */

// Size of a chunk in world units, positions are relative to their chunk
#define MOB_CHUNK_EXTENT 32.0f

#define MOB_POSITION_BITS 13
#define MOB_ROTATION_BITS 9

struct MobStateSnapshot {
    uint32 mob_id;
    uint8 mob_type;
    uint32 chunk;

    // Relative to the chunk origin, [0, MOB_CHUNK_EXTENT]
    v3_f32 position;
    quaternion rotation;

    uint32 state_flag;
};

template <typename Stream>
inline
bool net_packet_serialize(Stream* stream, MobStateSnapshot* msg) NO_EXCEPT
{
    if (!serialize_uint(stream, &msg->mob_id, 32)
        || !serialize_uint(stream, &msg->mob_type, 8)
        || !serialize_uint(stream, &msg->chunk, 32)
        || !serialize_v3_range(stream, &msg->position, 0.0f, MOB_CHUNK_EXTENT, MOB_POSITION_BITS)
        || !serialize_quaternion(stream, &msg->rotation, MOB_ROTATION_BITS)
        || !serialize_uint(stream, &msg->state_flag, 32)
    ) {
        return false;
    }

    return true;
}

enum MobStateChanged : uint8 {
    MOB_STATE_CHANGED_CHUNK = 1 << 0,
    MOB_STATE_CHANGED_POSITION = 1 << 1,
    MOB_STATE_CHANGED_ROTATION = 1 << 2,
    MOB_STATE_CHANGED_STATE = 1 << 3,
};

#define MOB_STATE_CHANGED_BITS 4

struct MobStateDelta {
    uint32 mob_id;

    // MobStateChanged, only these fields are sent
    uint8 changed;

    uint32 chunk;
    v3_f32 position;
    quaternion rotation;
    uint32 state_flag;
};

template <typename Stream>
inline
bool net_packet_serialize(Stream* stream, MobStateDelta* msg) NO_EXCEPT
{
    if (!serialize_uint(stream, &msg->mob_id, 32)
        || !serialize_uint(stream, &msg->changed, MOB_STATE_CHANGED_BITS)
    ) {
        return false;
    }

    if ((msg->changed & MOB_STATE_CHANGED_CHUNK) && !serialize_uint(stream, &msg->chunk, 32)) {
        return false;
    }

    if ((msg->changed & MOB_STATE_CHANGED_POSITION) && !serialize_v3_range(stream, &msg->position, 0.0f, MOB_CHUNK_EXTENT, MOB_POSITION_BITS)) {
        return false;
    }

    if ((msg->changed & MOB_STATE_CHANGED_ROTATION) && !serialize_quaternion(stream, &msg->rotation, MOB_ROTATION_BITS)) {
        return false;
    }

    if ((msg->changed & MOB_STATE_CHANGED_STATE) && !serialize_uint(stream, &msg->state_flag, 32)) {
        return false;
    }

    return true;
}

/**
 * Two positions are equal if they have the same quantized value (float noise below
 * the precision doesn't create a delta)
 */
inline
bool mob_position_equal(const v3_f32* const a, const v3_f32* const b) NO_EXCEPT
{
    for (int32 i = 0; i < 3; ++i) {
        if (quantize_float(a->vec[i], 0.0f, MOB_CHUNK_EXTENT, MOB_POSITION_BITS)
            != quantize_float(b->vec[i], 0.0f, MOB_CHUNK_EXTENT, MOB_POSITION_BITS)
        ) {
            return false;
        }
    }

    return true;
}

inline
bool mob_rotation_equal(const quaternion* const a, const quaternion* const b) NO_EXCEPT
{
    // q and -q are the same rotation
    const f32 dot = a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;

    return fabsf(dot) > 0.99999f;
}

/**
 * Creates the delta between a known snapshot and the current state
 *
 * @return bool False if nothing changed (nothing needs to be sent)
 */
inline
bool mob_state_delta_create(
    const MobStateSnapshot* const __restrict base,
    const MobStateSnapshot* const __restrict current,
    MobStateDelta* const __restrict delta
) NO_EXCEPT
{
    memset(delta, 0, sizeof(*delta));
    delta->mob_id = current->mob_id;

    if (base->chunk != current->chunk) {
        delta->changed |= MOB_STATE_CHANGED_CHUNK;
        delta->chunk = current->chunk;
    }

    // A chunk change always sends the position (it is relative to the chunk)
    if ((delta->changed & MOB_STATE_CHANGED_CHUNK) || !mob_position_equal(&base->position, &current->position)) {
        delta->changed |= MOB_STATE_CHANGED_POSITION;
        delta->position = current->position;
    }

    if (!mob_rotation_equal(&base->rotation, &current->rotation)) {
        delta->changed |= MOB_STATE_CHANGED_ROTATION;
        delta->rotation = current->rotation;
    }

    if (base->state_flag != current->state_flag) {
        delta->changed |= MOB_STATE_CHANGED_STATE;
        delta->state_flag = current->state_flag;
    }

    return delta->changed != 0;
}

#endif

/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_STDLIB_GAME_MATH_TYPES_H
#define COMS_STDLIB_GAME_MATH_TYPES_H

#include "Stdlib.h"

// @question Game math is so specific that we should probably move it out of stdlib and to math/game/GameMathDefines.h


struct AABB_f32 {
    v3_f32 min;
    v3_f32 max;
};

struct AABB_int32 {
    v3_int32 min;
    v3_int32 max;
};

struct AABB_cube_f32 {
    v3_f32 coord;
    f32 half_size;
};

struct AABB_cube_int32 {
    v3_int32 coord;
    int32 half_size;
};

CONSTEXPR inline
AABB_f32 aabb_cast(const AABB_int32& a) NO_EXCEPT
{
    return {
        { (f32) a.min.x, (f32) a.min.y, (f32) a.min.z },
        { (f32) a.max.x, (f32) a.max.y, (f32) a.max.z }
    };
}

CONSTEXPR inline
AABB_int32 aabb_loosen(const AABB_int32& b, f32 s) NO_EXCEPT
{
    const v3_int32 c = {
        (int32) ((b.min.x + b.max.x) * 0.5f),
        (int32) ((b.min.y + b.max.y) * 0.5f),
        (int32) ((b.min.z + b.max.z) * 0.5f)
    };

    const v3_int32 e = {
        (int32) ((b.max.x - b.min.x) * 0.5f * s),
        (int32) ((b.max.y - b.min.y) * 0.5f * s),
        (int32) ((b.max.z - b.min.z) * 0.5f * s)
    };

    return {
        {c.x - e.x, c.y - e.y, c.z - e.z},
        {c.x + e.x, c.y + e.y, c.z + e.z}
    };
}

CONSTEXPR inline
AABB_f32 aabb_loosen(const AABB_f32& b, f32 s) NO_EXCEPT
{
    const v3_f32 c = {
        (b.min.x + b.max.x) * 0.5f,
        (b.min.y + b.max.y) * 0.5f,
        (b.min.z + b.max.z) * 0.5f
    };

    const v3_f32 e = {
        (b.max.x - b.min.x) * 0.5f * s,
        (b.max.y - b.min.y) * 0.5f * s,
        (b.max.z - b.min.z) * 0.5f * s
    };

    return {
        {c.x - e.x, c.y - e.y, c.z - e.z},
        {c.x + e.x, c.y + e.y, c.z + e.z}
    };
}

CONSTEXPR inline
v3_int32 aabb_center(const AABB_int32& box) NO_EXCEPT
{
    v3_int32 c;
    c.x = (box.min.x + box.max.x) / 2;
    c.y = (box.min.y + box.max.y) / 2;
    c.z = (box.min.z + box.max.z) / 2;

    return c;
}

CONSTEXPR inline
v3_f32 aabb_center(const AABB_f32& box) NO_EXCEPT
{
    v3_f32 c;
    c.x = (box.min.x + box.max.x) * 0.5f;
    c.y = (box.min.y + box.max.y) * 0.5f;
    c.z = (box.min.z + box.max.z) * 0.5f;

    return c;
}

CONSTEXPR FORCE_INLINE
bool aabb_overlap(const AABB_int32& a, const AABB_int32& b) NO_EXCEPT
{
    if (a.max.x < b.min.x || a.min.x > b.max.x
        || a.max.y < b.min.y || a.min.y > b.max.y
        || a.max.z < b.min.z || a.min.z > b.max.z
    ) {
        return false;
    }

    return true;
}

#endif
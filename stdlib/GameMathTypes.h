/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_GAME_MATH_TYPES_H
#define COMS_STDLIB_GAME_MATH_TYPES_H

#include "Stdlib.h"

// @question Game math is so specific that we should probably move it out of stdlib and to math/game/GameMathDefines.h

#define OMS_LERP(a, b, t) ((1 - (t)) * (a) + (t) * (b))
#define OMS_INVERSE_LERP(a, b, x) (((x) - (a)) / ((b) - (a)))

// Remap x from [inMin, inMax] to [outMin, outMax]
#define OMS_REMAP(x, inMin, inMax, outMin, outMax) (OMS_LERP((outMin), (outMax), OMS_INVERSE_LERP((inMin), (inMax), (x))))

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

inline
AABB_f32 aabb_cast(AABB_int32 a) NO_EXCEPT
{
    return {
        { (f32) a.min.x, (f32) a.min.y, (f32) a.min.z },
        { (f32) a.max.x, (f32) a.max.y, (f32) a.max.z }
    };
}

inline
AABB_int32 aabb_loosen(AABB_int32 b, f32 s) NO_EXCEPT
{
    v3_int32 c = {
        (int32) ((b.min.x + b.max.x) * 0.5f),
        (int32) ((b.min.y + b.max.y) * 0.5f),
        (int32) ((b.min.z + b.max.z) * 0.5f)
    };

    v3_int32 e = {
        (int32) ((b.max.x - b.min.x) * 0.5f * s),
        (int32) ((b.max.y - b.min.y) * 0.5f * s),
        (int32) ((b.max.z - b.min.z) * 0.5f * s)
    };

    return {
        {c.x - e.x, c.y - e.y, c.z - e.z},
        {c.x + e.x, c.y + e.x, c.z + e.z}
    };
}

inline
AABB_f32 aabb_loosen(AABB_f32 b, f32 s) NO_EXCEPT
{
    v3_f32 c = {
        (b.min.x + b.max.x) * 0.5f,
        (b.min.y + b.max.y) * 0.5f,
        (b.min.z + b.max.z) * 0.5f
    };

    v3_f32 e = {
        (b.max.x - b.min.x) * 0.5f * s,
        (b.max.y - b.min.y) * 0.5f * s,
        (b.max.z - b.min.z) * 0.5f * s
    };

    return {
        {c.x - e.x, c.y - e.y, c.z - e.z},
        {c.x + e.x, c.y + e.x, c.z + e.z}
    };
}

inline
v3_int32 aabb_center(AABB_int32 box) NO_EXCEPT
{
    v3_int32 c;
    c.x = (box.min.x + box.max.x) / 2;
    c.y = (box.min.y + box.max.y) / 2;
    c.z = (box.min.z + box.max.z) / 2;

    return c;
}

inline
v3_f32 aabb_center(AABB_f32 box) NO_EXCEPT
{
    v3_f32 c;
    c.x = (box.min.x + box.max.x) * 0.5f;
    c.y = (box.min.y + box.max.y) * 0.5f;
    c.z = (box.min.z + box.max.z) * 0.5f;

    return c;
}

FORCE_INLINE
bool aabb_overlap(AABB_int32 a, AABB_int32 b) NO_EXCEPT
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
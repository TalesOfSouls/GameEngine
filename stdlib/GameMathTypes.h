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

#include "Types.h"

struct AABB_f32 {
    v3_f32 min;
    v3_f32 max;
};

struct AABB_int32 {
    v3_int32 min;
    v3_int32 max;
};

struct Frustum {
    // A frustum consists of 6 planes where every plane has the form ax + by + cz + d = 0
    // This means every plane requires 4 parameters
    union {
        f32 plane[6 * 4];
        v4_f32 eq[6];
    };
};

#endif
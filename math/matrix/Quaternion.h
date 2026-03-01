/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MATH_MATRIX_QUATERNION_H
#define COMS_MATH_MATRIX_QUATERNION_H

#include "../../stdlib/Stdlib.h"
#include "../../utils/Assert.h"
#include "../../architecture/Intrinsics.h"
#include "Matrix.h"

static inline
quaternion quat_mul(quaternion a, quaternion b)
{
    return {
        a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
        a.w*b.y - a.x*b.z + a.y*b.w + a.z*b.x,
        a.w*b.z + a.x*b.y - a.y*b.x + a.z*b.w,
        a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z
    };
}

static inline
quaternion quat_normalize(quaternion q)
{
    const f32 len = sqrtf(q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w);
    const f32 inv = 1.0f / len;

    return { q.x*inv, q.y*inv, q.z*inv, q.w*inv };
}

static inline
quaternion quat_axis_angle(v3_f32 axis, f32 angle_rad)
{
    const f32 s = sinf(angle_rad * 0.5f);

    return { axis.x*s, axis.y*s, axis.z*s, cosf(angle_rad*0.5f) };
}

static inline
v3_f32 quat_rotate_vec3(quaternion q, v3_f32 v)
{
    // v' = q * (v,0) * q^-1
    quaternion p = { v.x, v.y, v.z, 0.0f };
    // inverse for unit quat
    quaternion qi = { -q.x, -q.y, -q.z, q.w };
    quaternion r = quat_mul(quat_mul(q, p), qi);

    return { r.x, r.y, r.z };
}

#endif
/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga->app
 */
#ifndef COMS_MATH_MATRIX_INT32_H
#define COMS_MATH_MATRIX_INT32_H

#include <string.h>
#include <math.h>
#include "../../stdlib/Types.h"
#include "../../utils/Assert.h"
#include "../../architecture/Intrinsics.h"
#include "../../compiler/CompilerUtils.h"

FORCE_INLINE
int32 vec2_sum(v2_int32* vec) NO_EXCEPT
{
    return vec->x + vec->y;
}

FORCE_INLINE
f32 vec2_sum(v2_int32 vec) NO_EXCEPT
{
    return vec.x + vec.y;
}

FORCE_INLINE
void vec2_add(v2_int32* __restrict vec, const v2_int32* a, const v2_int32* b) NO_EXCEPT
{
    vec->x = a->x + b->x;
    vec->y = a->y + b->y;
}

FORCE_INLINE
v2_int32 vec2_add(v2_int32 a, v2_int32 b) NO_EXCEPT
{
    return {a.x + b.x, a.y + b.y};
}

FORCE_INLINE
void vec2_add(v2_int32* __restrict vec, const v2_int32* b) NO_EXCEPT
{
    vec->x += b->x;
    vec->y += b->y;
}

FORCE_INLINE
void vec2_sub(v2_int32* __restrict vec, const v2_int32* __restrict a, const v2_int32* __restrict b) NO_EXCEPT
{
    vec->x = a->x - b->x;
    vec->y = a->y - b->y;
}

FORCE_INLINE
v2_int32 vec2_sub(v2_int32 a, v2_int32 b) NO_EXCEPT
{
    return {a.x - b.x, a.y - b.y};
}

FORCE_INLINE
void vec2_sub(v2_int32* __restrict vec, const v2_int32* __restrict b) NO_EXCEPT
{
    vec->x -= b->x;
    vec->y -= b->y;
}

FORCE_INLINE
void vec2_mul(v2_int32* __restrict vec, const v2_int32* __restrict a, int32 s) NO_EXCEPT
{
    vec->x = a->x * s;
    vec->y = a->y * s;
}

FORCE_INLINE
v2_int32 vec2_mul(v2_int32 a, f32 s) NO_EXCEPT
{
    return {a.x * s, a.y * s};
}

FORCE_INLINE
void vec2_mul(v2_int32* vec, int32 s) NO_EXCEPT
{
    vec->x *= s;
    vec->y *= s;
}

FORCE_INLINE
int32 vec2_mul(const v2_int32* a, const v2_int32* b) NO_EXCEPT
{
    return a->x * b->x + a->y * b->y;
}

FORCE_INLINE
void vec2_mul(v2_int32* __restrict vec, const v2_int32* a, const v2_int32* b) NO_EXCEPT
{
    vec->x = a->x * b->x;
    vec->y = a->y * b->y;
}

FORCE_INLINE
v2_int32 vec2_mul(v2_int32 a, v2_int32 b) NO_EXCEPT
{
    return {a.x * b.x, a.y * b.y};
}

FORCE_INLINE
void vec2_mul(v2_int32* vec, const v2_int32* b) NO_EXCEPT
{
    vec->x *= b->x;
    vec->y *= b->y;
}

FORCE_INLINE
void vec2_fma(v2_int32* __restrict vec, const v2_int32* a, const v2_int32* b, int32 scalar) NO_EXCEPT {
    vec->x = a->x + b->x * scalar;
    vec->y = a->y + b->y * scalar;
}

FORCE_INLINE
v2_int32 vec2_fma(v2_int32 a, v2_int32 b, f32 scalar) NO_EXCEPT {
    return {a.x + b.x * scalar, a.y + b.y * scalar};
}

FORCE_INLINE
int32 vec2_cross(const v2_int32* a, const v2_int32* b) NO_EXCEPT
{
    return a->x * b->y - a->y * b->x;
}

FORCE_INLINE
f32 vec2_cross(v2_int32 a, v2_int32 b) NO_EXCEPT
{
    return a.x * b.y - a.y * b.x;
}

FORCE_INLINE
int32 vec2_dot(const v2_int32* a, const v2_int32* b) NO_EXCEPT
{
    return a->x * b->x + a->y * b->y;
}

FORCE_INLINE
f32 vec2_dot(v2_int32 a, v2_int32 b) NO_EXCEPT
{
    return a.x * b.x + a.y * b.y;
}

FORCE_INLINE
int32 vec3_length(f32 x, int32 y, int32 z) NO_EXCEPT
{
    return intrin_sqrt_f32(x * x + y * y + z * z);
}

FORCE_INLINE
int32 vec3_length(v3_int32* vec) NO_EXCEPT
{
    return intrin_sqrt_f32(vec->x * vec->x + vec->y * vec->y + vec->z * vec->z);
}

FORCE_INLINE
f32 vec3_length(v3_int32 vec) NO_EXCEPT
{
    return intrin_sqrt_f32(vec.x * vec.x + vec.y * vec.y + vec.z * vec.z);
}

FORCE_INLINE
void vec3_normalize(f32* __restrict x, f32* __restrict y, f32* __restrict z) NO_EXCEPT
{
    int32 d = intrin_rsqrt_int32((*x) * (*x) + (*y) * (*y) + (*z) * (*z));

    *x *= d;
    *y *= d;
    *z *= d;
}

FORCE_INLINE
void vec3_normalize(v3_int32* vec) NO_EXCEPT
{
    int32 d = intrin_rsqrt_int32(vec->x * vec->x + vec->y * vec->y + vec->z * vec->z);

    vec->x *= d;
    vec->y *= d;
    vec->z *= d;
}

FORCE_INLINE
int32 vec3_sum(v3_int32* vec) NO_EXCEPT
{
    return vec->x + vec->y + vec->z;
}

FORCE_INLINE
f32 vec3_sum(v3_int32 vec) NO_EXCEPT
{
    return vec.x + vec.y + vec.z;
}

FORCE_INLINE
void vec3_add(v3_int32* __restrict vec, const v3_int32* a, const v3_int32* b) NO_EXCEPT
{
    vec->x = a->x + b->x;
    vec->y = a->y + b->y;
    vec->z = a->z + b->z;
}

FORCE_INLINE
v3_int32 vec3_add(v3_int32 a, v3_int32 b) NO_EXCEPT
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

FORCE_INLINE
void vec3_add(v3_int32* __restrict vec, const v3_int32* b) NO_EXCEPT
{
    vec->x += b->x;
    vec->y += b->y;
    vec->z += b->z;
}

FORCE_INLINE
void vec3_sub(v3_int32* __restrict vec, const v3_int32* __restrict a, const v3_int32* __restrict b) NO_EXCEPT
{
    vec->x = a->x - b->x;
    vec->y = a->y - b->y;
    vec->z = a->z - b->z;
}

FORCE_INLINE
v3_int32 vec3_sub(v3_int32 a, v3_int32 b) NO_EXCEPT
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

FORCE_INLINE
void vec3_sub(v3_int32* __restrict vec, const v3_int32* __restrict b) NO_EXCEPT
{
    vec->x -= b->x;
    vec->y -= b->y;
    vec->z -= b->z;
}

FORCE_INLINE
void vec3_mul(v3_int32* __restrict vec, const v3_int32* __restrict a, int32 s) NO_EXCEPT
{
    vec->x = a->x * s;
    vec->y = a->y * s;
    vec->z = a->z * s;
}

FORCE_INLINE
v3_int32 vec3_mul(v3_int32 a, v3_int32 b) NO_EXCEPT
{
    return {a.x * b.x, a.y * b.y, a.z * b.z};
}

FORCE_INLINE
void vec3_mul(v3_int32* vec, int32 s) NO_EXCEPT
{
    vec->x *= s;
    vec->y *= s;
    vec->z *= s;
}

FORCE_INLINE
int32 vec3_mul(const v3_int32* a, const v3_int32* b) NO_EXCEPT
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

FORCE_INLINE
void vec3_mul(v3_int32* __restrict vec, const v3_int32* a, const v3_int32* b) NO_EXCEPT
{
    vec->x = a->x * b->x;
    vec->y = a->y * b->y;
    vec->z = a->z * b->z;
}

FORCE_INLINE
void vec3_mul(v3_int32* vec, const v3_int32* b) NO_EXCEPT
{
    vec->x *= b->x;
    vec->y *= b->y;
    vec->z *= b->z;
}

FORCE_INLINE
void vec3_fma(v3_int32* __restrict vec, const v3_int32* a, const v3_int32* b, int32 scalar) NO_EXCEPT {
    vec->x = a->x + b->x * scalar;
    vec->y = a->y + b->y * scalar;
    vec->z = a->z + b->z * scalar;
}

FORCE_INLINE
v3_int32 vec3_fma(v3_int32 a, v3_int32 b, f32 scalar) NO_EXCEPT {
    return {a.x + b.x * scalar, a.x + b.x * scalar, a.x + b.x * scalar};
}

FORCE_INLINE
void vec3_cross(v3_int32* __restrict vec, const v3_int32* a, const v3_int32* b) NO_EXCEPT
{
    vec->x = a->y * b->z - a->z * b->y;
    vec->y = a->z * b->x - a->x * b->z;
    vec->z = a->x * b->y - a->y * b->x;
}

FORCE_INLINE
v3_int32 vec3_cross(v3_int32 a, v3_int32 b) NO_EXCEPT
{
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

FORCE_INLINE
int32 vec3_dot(const v3_int32* a, const v3_int32* b) NO_EXCEPT
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

FORCE_INLINE
f32 vec3_dot(v3_int32 a, v3_int32 b) NO_EXCEPT
{
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

FORCE_INLINE
void vec4_normalize(f32* __restrict x, f32* __restrict y, f32* __restrict z, f32* __restrict w) NO_EXCEPT
{
    int32 d = intrin_rsqrt_int32((*x) * (*x) + (*y) * (*y) + (*z) * (*z) + (*w) * (*w));

    *x *= d;
    *y *= d;
    *z *= d;
    *w *= d;
}

FORCE_INLINE
void vec4_add(v4_int32* __restrict vec, const v4_int32* a, const v4_int32* b) NO_EXCEPT
{
    vec->x = a->x + b->x;
    vec->y = a->y + b->y;
    vec->z = a->z + b->z;
    vec->w = a->w + b->w;
}

FORCE_INLINE
v4_int32 vec4_add(v4_int32 a, v4_int32 b) NO_EXCEPT
{
    return {a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w};
}

FORCE_INLINE
void vec4_add(v4_int32* __restrict vec, const v4_int32* b) NO_EXCEPT
{
    vec->x += b->x;
    vec->y += b->y;
    vec->z += b->z;
    vec->w += b->w;
}

FORCE_INLINE
void vec4_sub(v4_int32* __restrict vec, const v4_int32* __restrict a, const v4_int32* __restrict b) NO_EXCEPT
{
    vec->x = a->x - b->x;
    vec->y = a->y - b->y;
    vec->z = a->z - b->z;
    vec->w = a->w - b->w;
}

FORCE_INLINE
v4_int32 vec4_sub(v4_int32 a, v4_int32 b) NO_EXCEPT
{
    return {a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w};
}

FORCE_INLINE
void vec4_sub(v4_int32* __restrict vec, const v4_int32* __restrict b) NO_EXCEPT
{
    vec->x -= b->x;
    vec->y -= b->y;
    vec->z -= b->z;
    vec->w -= b->w;
}

FORCE_INLINE
void vec4_mul(v4_int32* __restrict vec, const v4_int32* __restrict a, int32 s) NO_EXCEPT
{
    vec->x = a->x * s;
    vec->y = a->y * s;
    vec->z = a->z * s;
    vec->w = a->w * s;
}

FORCE_INLINE
void vec4_mul(v4_int32* vec, int32 s) NO_EXCEPT
{
    vec->x *= s;
    vec->y *= s;
    vec->z *= s;
    vec->w *= s;
}

FORCE_INLINE
int32 vec4_mul(const v4_int32* a, const v4_int32* b) NO_EXCEPT
{
    return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

FORCE_INLINE
void vec4_mul(v4_int32* __restrict vec, const v4_int32* a, const v4_int32* b) NO_EXCEPT
{
    vec->x = a->x * b->x;
    vec->y = a->y * b->y;
    vec->z = a->z * b->z;
    vec->w = a->w * b->w;
}

FORCE_INLINE
v4_int32 vec4_mul(v4_int32 vec, f32 s) NO_EXCEPT
{
    return {vec.x * s, vec.y * s, vec.z * s, vec.w * s};
}

FORCE_INLINE
void vec4_mul(v4_int32* vec, const v4_int32* b) NO_EXCEPT
{
    vec->x *= b->x;
    vec->y *= b->y;
    vec->z *= b->z;
    vec->w *= b->w;
}

FORCE_INLINE
int32 vec4_dot(const v4_int32* a, const v4_int32* b) NO_EXCEPT
{
    return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

FORCE_INLINE
f32 vec4_dot(v4_int32 a, v4_int32 b) NO_EXCEPT
{
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

FORCE_INLINE
void vec4_cross(v4_int32* __restrict vec, const v4_int32* a, const v4_int32* b, const v4_int32* c) NO_EXCEPT
{
    const int32 d1 = b.z * c.w - b.w * c.z;
    const int32 d2 = b.y * c.w - b.w * c.y;
    const int32 d3 = b.y * c.z - b.z * c.y;
    const int32 d4 = b.x * c.w - b.w * c.x;
    const int32 d5 = b.x * c.z - b.z * c.x;
    const int32 d6 = b.x * c.y - b.y * c.x;

    vec->x = a->y * d1 - a->z * d2 + a->w * d3;
    vec->y = -(a->x * d1 - a->z * d4 + a->w * d5);
    vec->z = a->x * d2 - a->y * d4 + a->w * d6;
    vec->w = -(a->x * d3 - a->y * d5 + a->z * d6);
}

FORCE_INLINE
v4_int32 vec4_cross(v4_int32 a, v4_int32 b, v4_int32 c) NO_EXCEPT
{
    const int32 d1 = b.z * c.w - b.w * c.z;
    const int32 d2 = b.y * c.w - b.w * c.y;
    const int32 d3 = b.y * c.z - b.z * c.y;
    const int32 d4 = b.x * c.w - b.w * c.x;
    const int32 d5 = b.x * c.z - b.z * c.x;
    const int32 d6 = b.x * c.y - b.y * c.x;

    return {
        a.y * d1 - a.z * d2 + a.w * d3,
        -(a.x * d1 - a.z * d4 + a.w * d5),
        a.x * d2 - a.y * d4 + a.w * d6,
        -(a.x * d3 - a.y * d5 + a.z * d6)
    };
}

FORCE_INLINE
void mat3_identity(f32* matrix) NO_EXCEPT
{
    matrix[0] = 1.0f; matrix[1] = 0.0f; matrix[2] = 0.0f;
    matrix[3] = 0.0f; matrix[4] = 1.0f; matrix[5] = 0.0f;
    matrix[6] = 0.0f; matrix[7] = 0.0f; matrix[8] = 1.0f;
}

#endif
/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MATH_MATRIX_FLOAT32_H
#define COMS_MATH_MATRIX_FLOAT32_H

#include <string.h>
#include <math.h>
#include "../../utils/TestUtils.h"
#include "../../architecture/Intrinsics.h"

// INFO: I thought we could remove some of the functions.
//      Sometimes we have a function that modifies the original value
//      and then we also have the same function that fills a new result value.
//      On gcc the optimized code creates the same assembly if we would just choose to return the new value
//      vs. modifying a value by pointer.
//      However, on MSVC this is not the case and the pointer version has more and slower assembly code vs. the
//      pass-by-value function

inline
void vec2_normalize(f32* __restrict x, f32* __restrict y) noexcept
{
    f32 d = intrin_rsqrt_f32((*x) * (*x) + (*y) * (*y));

    *x *= d;
    *y *= d;
}

inline
void vec2_add(v2_f32* __restrict vec, const v2_f32* a, const v2_f32* b) noexcept
{
    vec->x = a->x + b->x;
    vec->y = a->y + b->y;
}

inline
void vec2_add(v2_f32* __restrict vec, const v2_f32* b) noexcept
{
    vec->x += b->x;
    vec->y += b->y;
}

inline
void vec2_sub(v2_f32* __restrict vec, const v2_f32* __restrict a, const v2_f32* __restrict b) noexcept
{
    vec->x = a->x - b->x;
    vec->y = a->y - b->y;
}

inline
void vec2_sub(v2_f32* __restrict vec, const v2_f32* __restrict b) noexcept
{
    vec->x -= b->x;
    vec->y -= b->y;
}

inline
void vec2_mul(v2_f32* __restrict vec, const v2_f32* __restrict a, f32 s) noexcept
{
    vec->x = a->x * s;
    vec->y = a->y * s;
}

inline
void vec2_mul(v2_f32* vec, f32 s) noexcept
{
    vec->x *= s;
    vec->y *= s;
}

inline
f32 vec2_mul(const v2_f32* a, const v2_f32* b) noexcept
{
    return a->x * b->x + a->y * b->y;
}

inline
void vec2_mul(v2_f32* __restrict vec, const v2_f32* a, const v2_f32* b) noexcept
{
    vec->x = a->x * b->x;
    vec->y = a->y * b->y;
}

inline
void vec2_mul(v2_f32* vec, const v2_f32* b) noexcept
{
    vec->x *= b->x;
    vec->y *= b->y;
}

inline
f32 vec2_cross(const v2_f32* a, const v2_f32* b) noexcept
{
    return a->x * b->y - a->y * b->x;
}

inline
f32 vec2_dot(const v2_f32* a, const v2_f32* b) noexcept
{
    return a->x * b->x + a->y * b->y;
}

inline
f32 vec3_length(f32 x, f32 y, f32 z) noexcept
{
    return sqrtf(x * x + y * y + z * z);
}

inline
f32 vec3_length(v3_f32* vec) noexcept
{
    return sqrtf(vec->x * vec->x + vec->y * vec->y + vec->z * vec->z);
}

inline
void vec3_normalize(f32* __restrict x, f32* __restrict y, f32* __restrict z) noexcept
{
    f32 d = intrin_rsqrt_f32((*x) * (*x) + (*y) * (*y) + (*z) * (*z));

    *x *= d;
    *y *= d;
    *z *= d;
}

inline
void vec3_normalize(v3_f32* vec) noexcept
{
    f32 d = intrin_rsqrt_f32(vec->x * vec->x + vec->y * vec->y + vec->z * vec->z);

    vec->x *= d;
    vec->y *= d;
    vec->z *= d;
}

inline
void vec3_add(v3_f32* __restrict vec, const v3_f32* a, const v3_f32* b) noexcept
{
    vec->x = a->x + b->x;
    vec->y = a->y + b->y;
    vec->z = a->z + b->z;
}

inline
void vec3_add(v3_f32* __restrict vec, const v3_f32* b) noexcept
{
    vec->x += b->x;
    vec->y += b->y;
    vec->z += b->z;
}

inline
void vec3_sub(v3_f32* __restrict vec, const v3_f32* __restrict a, const v3_f32* __restrict b) noexcept
{
    vec->x = a->x - b->x;
    vec->y = a->y - b->y;
    vec->z = a->z - b->z;
}

inline
void vec3_sub(v3_f32* __restrict vec, const v3_f32* __restrict b) noexcept
{
    vec->x -= b->x;
    vec->y -= b->y;
    vec->z -= b->z;
}

inline
void vec3_mul(v3_f32* __restrict vec, const v3_f32* __restrict a, f32 s) noexcept
{
    vec->x = a->x * s;
    vec->y = a->y * s;
    vec->z = a->z * s;
}

inline
void vec3_mul(v3_f32* vec, f32 s) noexcept
{
    vec->x *= s;
    vec->y *= s;
    vec->z *= s;
}

inline
f32 vec3_mul(const v3_f32* a, const v3_f32* b) noexcept
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

inline
void vec3_mul(v3_f32* __restrict vec, const v3_f32* a, const v3_f32* b) noexcept
{
    vec->x = a->x * b->x;
    vec->y = a->y * b->y;
    vec->z = a->z * b->z;
}

inline
void vec3_mul(v3_f32* vec, const v3_f32* b) noexcept
{
    vec->x *= b->x;
    vec->y *= b->y;
    vec->z *= b->z;
}

void vec3_cross(v3_f32* __restrict vec, const v3_f32* a, const v3_f32* b) noexcept
{
    vec->x = a->y * b->z - a->z * b->y;
    vec->y = a->z * b->x - a->x * b->z;
    vec->z = a->x * b->y - a->y * b->x;
}

inline
f32 vec3_dot(const v3_f32* a, const v3_f32* b) noexcept
{
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

void vec4_normalize(f32* __restrict x, f32* __restrict y, f32* __restrict z, f32* __restrict w) noexcept
{
    f32 d = intrin_rsqrt_f32((*x) * (*x) + (*y) * (*y) + (*z) * (*z) + (*w) * (*w));

    *x *= d;
    *y *= d;
    *z *= d;
    *w *= d;
}

inline
void vec4_add(v4_f32* __restrict vec, const v4_f32* a, const v4_f32* b) noexcept
{
    vec->x = a->x + b->x;
    vec->y = a->y + b->y;
    vec->z = a->z + b->z;
    vec->w = a->w + b->w;
}

inline
void vec4_add(v4_f32* __restrict vec, const v4_f32* b) noexcept
{
    vec->x += b->x;
    vec->y += b->y;
    vec->z += b->z;
    vec->w += b->w;
}

inline
void vec4_sub(v4_f32* __restrict vec, const v4_f32* __restrict a, const v4_f32* __restrict b) noexcept
{
    vec->x = a->x - b->x;
    vec->y = a->y - b->y;
    vec->z = a->z - b->z;
    vec->w = a->w - b->w;
}

inline
void vec4_sub(v4_f32* __restrict vec, const v4_f32* __restrict b) noexcept
{
    vec->x -= b->x;
    vec->y -= b->y;
    vec->z -= b->z;
    vec->w -= b->w;
}

inline
void vec4_mul(v4_f32* __restrict vec, const v4_f32* __restrict a, f32 s) noexcept
{
    vec->x = a->x * s;
    vec->y = a->y * s;
    vec->z = a->z * s;
    vec->w = a->w * s;
}

inline
void vec4_mul(v4_f32* vec, f32 s) noexcept
{
    vec->x *= s;
    vec->y *= s;
    vec->z *= s;
    vec->w *= s;
}

inline
f32 vec4_mul(const v4_f32* a, const v4_f32* b) noexcept
{
    return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

inline
void vec4_mul(v4_f32* __restrict vec, const v4_f32* a, const v4_f32* b) noexcept
{
    vec->x = a->x * b->x;
    vec->y = a->y * b->y;
    vec->z = a->z * b->z;
    vec->w = a->w * b->w;
}

inline
void vec4_mul(v4_f32* vec, const v4_f32* b) noexcept
{
    vec->x *= b->x;
    vec->y *= b->y;
    vec->z *= b->z;
    vec->w *= b->w;
}

inline
f32 vec4_dot(const v4_f32* a, const v4_f32* b) noexcept
{
    return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

inline
void vec4_cross(v4_f32* __restrict vec, const v4_f32* a, const v4_f32* b, const v4_f32* c) noexcept
{
    vec->x = a->y * (b->z * c->w - b->w * c->z) - a->z * (b->y * c->w - b->w * c->y) + a->w * (b->y * c->z - b->z * c->y);
    vec->y = -(a->x * (b->z * c->w - b->w * c->z) - a->z * (b->x * c->w - b->w * c->x) + a->w * (b->x * c->z - b->z * c->x));
    vec->z = a->x * (b->y * c->w - b->w * c->y) - a->y * (b->x * c->w - b->w * c->x) + a->w * (b->x * c->y - b->y * c->x);
    vec->w = -(a->x * (b->y * c->z - b->z * c->y) - a->y * (b->x * c->z - b->z * c->x) + a->z * (b->x * c->y - b->y * c->x));
}

inline
void mat3_identity(f32* matrix) noexcept
{
    matrix[0] = 1.0f; matrix[1] = 0.0f; matrix[2] = 0.0f;
    matrix[3] = 0.0f; matrix[4] = 1.0f; matrix[5] = 0.0f;
    matrix[6] = 0.0f; matrix[7] = 0.0f; matrix[8] = 1.0f;
}

inline
void mat3_identity_sparse(f32* matrix) noexcept
{
    matrix[0] = 1.0f; matrix[4] = 1.0f; matrix[8] = 1.0f;
}

inline
void mat3_identity(__m128* matrix) noexcept
{
    matrix[0] = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
    matrix[1] = _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f);
    matrix[2] = _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f);
}

inline
void mat4_identity(f32* matrix) noexcept
{
    matrix[0] = 1.0f;  matrix[1] = 0.0f;  matrix[2] = 0.0f;  matrix[3] = 0.0f;
    matrix[4] = 0.0f;  matrix[5] = 1.0f;  matrix[6] = 0.0f;  matrix[7] = 0.0f;
    matrix[8] = 0.0f;  matrix[9] = 0.0f;  matrix[10] = 1.0f; matrix[11] = 0.0f;
    matrix[12] = 0.0f; matrix[13] = 0.0f; matrix[14] = 0.0f; matrix[15] = 1.0f;
}

inline
void mat4_identity_sparse(f32* matrix) noexcept
{
    matrix[0] = 1.0f; matrix[5] = 1.0f; matrix[10] = 1.0f; matrix[15] = 1.0f;
}

inline
void mat4_identity(__m128* matrix) noexcept
{
    matrix[0] = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
    matrix[1] = _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f);
    matrix[2] = _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f);
    matrix[3] = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);
}

// x, y, z need to be normalized
// https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
void mat4_rotation(f32* matrix, f32 x, f32 y, f32 z, f32 angle) noexcept
{
    ASSERT_SIMPLE(OMS_ABS_F32(x * x + y * y + z * z - 1.0f) < 0.01);

    // @todo replace with quaternions
    f32 s = sinf(angle);
    f32 c = cosf(angle);
    f32 m = 1 - c;

    f32 mx = m * x;
    f32 my = m * y;
    f32 mz = m * z;

    f32 xs = x * s;
    f32 ys = y * s;
    f32 zs = z * s;

    f32 mxy = mx * y;
    f32 mzx = mz * x;
    f32 myz = my * z;

    matrix[0] = mx * x + c;
    matrix[1] = mxy - zs;
    matrix[2] = mzx + ys;
    matrix[3] = 0.0f;

    matrix[4] = mxy + zs;
    matrix[5] = my * y + c;
    matrix[6] = myz - xs;
    matrix[7] = 0.0f;

    matrix[8] = mzx - ys;
    matrix[9] = myz + xs;
    matrix[10] = mz * z + c;
    matrix[11] = 0.0f;

    matrix[12] = 0.0f;
    matrix[13] = 0.0f;
    matrix[14] = 0.0f;
    matrix[15] = 1.0f;
}

void mat4_rotation(f32* matrix, f32 pitch, f32 yaw, f32 roll) noexcept
{
    f32 cos_pitch = cosf(pitch);
    f32 sin_pitch = sinf(pitch);
    f32 cos_yaw = cosf(yaw);
    f32 sin_yaw = sinf(yaw);
    f32 cos_roll = cosf(roll);
    f32 sin_roll = sinf(roll);

    matrix[0] = cos_yaw * cos_roll;
    matrix[1] = cos_yaw * sin_roll;
    matrix[2] = -sin_yaw;
    matrix[3] = 0.0f;

    matrix[4] = sin_pitch * sin_yaw * cos_roll - cos_pitch * sin_roll;
    matrix[5] = sin_pitch * sin_yaw * sin_roll + cos_pitch * cos_roll;
    matrix[6] = sin_pitch * cos_yaw;
    matrix[7] = 0.0f;

    matrix[8] = cos_pitch * sin_yaw * cos_roll + sin_pitch * sin_roll;
    matrix[9] = cos_pitch * sin_yaw * sin_roll - sin_pitch * cos_roll;
    matrix[10] = cos_pitch * cos_yaw;
    matrix[11] = 0.0f;

    matrix[12] = 0.0f;
    matrix[13] = 0.0f;
    matrix[14] = 0.0f;
    matrix[15] = 1.0f;
}

inline
void mat3vec3_mult(const f32* __restrict matrix, const f32* __restrict vector, f32* __restrict result) noexcept
{
    result[0] = matrix[0] * vector[0] + matrix[1] * vector[1] + matrix[2] * vector[2];
    result[1] = matrix[3] * vector[0] + matrix[4] * vector[1] + matrix[5] * vector[2];
    result[2] = matrix[6] * vector[0] + matrix[7] * vector[1] + matrix[8] * vector[2];
}

inline
void mat4vec4_mult(const f32* __restrict matrix, const f32* __restrict vector, f32* __restrict result) noexcept
{
    result[0] = matrix[0] * vector[0] + matrix[1] * vector[1] + matrix[2] * vector[2] + matrix[3] * vector[3];
    result[1] = matrix[4] * vector[0] + matrix[5] * vector[1] + matrix[6] * vector[2] + matrix[7] * vector[3];
    result[2] = matrix[8] * vector[0] + matrix[9] * vector[1] + matrix[10] * vector[2] + matrix[11] * vector[3];
    result[3] = matrix[12] * vector[0] + matrix[13] * vector[1] + matrix[14] * vector[2] + matrix[15] * vector[3];
}

// @question could simple mul add sse be faster?
void mat4vec4_mult_sse(const f32* __restrict matrix, const f32* __restrict vector, f32* __restrict result) noexcept
{
    __m128 vec = _mm_load_ps(vector);

    for (int32 i = 0; i < 4; ++i) {
        __m128 row = _mm_load_ps(&matrix[i * 4]);
        __m128 dot = _mm_dp_ps(row, vec, 0xF1);

        result[i] = _mm_cvtss_f32(dot);
    }
}

// @question could simple mul add sse be faster?
void mat4vec4_mult_sse(const __m128* __restrict matrix, const __m128* __restrict vector, f32* __restrict result) noexcept
{
    for (int32 i = 0; i < 4; ++i) {
        __m128 dot = _mm_dp_ps(matrix[i], *vector, 0xF1);

        result[i] = _mm_cvtss_f32(dot);
    }
}

// @question could simple mul add sse be faster?
void mat4vec4_mult_sse(const __m128* __restrict matrix, const __m128* __restrict vector, __m128* __restrict result) noexcept
{
    for (int32 i = 0; i < 4; ++i) {
        result[i] = _mm_dp_ps(matrix[i], *vector, 0xF1);
    }
}

inline
void mat4mat4_mult(const f32* __restrict a, const f32* __restrict b, f32* __restrict result) noexcept
{
    result[0] = a[0] * b[0] + a[1] * b[4] + a[2] * b[8] + a[3] * b[12];
    result[1] = a[0] * b[1] + a[1] * b[5] + a[2] * b[9] + a[3] * b[13];
    result[2] = a[0] * b[2] + a[1] * b[6] + a[2] * b[10] + a[3] * b[14];
    result[3] = a[0] * b[3] + a[1] * b[7] + a[2] * b[11] + a[3] * b[15];

    result[4] = a[4] * b[0] + a[5] * b[4] + a[6] * b[8] + a[7] * b[12];
    result[5] = a[4] * b[1] + a[5] * b[5] + a[6] * b[9] + a[7] * b[13];
    result[6] = a[4] * b[2] + a[5] * b[6] + a[6] * b[10] + a[7] * b[14];
    result[7] = a[4] * b[3] + a[5] * b[7] + a[6] * b[11] + a[7] * b[15];

    result[8] = a[8] * b[0] + a[9] * b[4] + a[10] * b[8] + a[11] * b[12];
    result[9] = a[8] * b[1] + a[9] * b[5] + a[10] * b[9] + a[11] * b[13];
    result[10] = a[8] * b[2] + a[9] * b[6] + a[10] * b[10] + a[11] * b[14];
    result[11] = a[8] * b[3] + a[9] * b[7] + a[10] * b[11] + a[11] * b[15];

    result[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];
    result[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];
    result[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];
    result[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
}

inline
void mat4mat4_mult_simd(const f32* __restrict a, const f32* __restrict b, f32* __restrict result) noexcept
{
    __m128 row1 = _mm_load_ps(&b[0]);
    __m128 row2 = _mm_load_ps(&b[4]);
    __m128 row3 = _mm_load_ps(&b[8]);
    __m128 row4 = _mm_load_ps(&b[12]);

    for (int32 i = 3; i >= 0; --i) {
        __m128 vW = _mm_load_ps(&a[i * 4]);

        __m128 vX = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(0, 0, 0, 0));
        __m128 vY = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(1, 1, 1, 1));
        __m128 vZ = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(2, 2, 2, 2));
        vW = _mm_shuffle_ps(vW, vW, _MM_SHUFFLE(3, 3, 3, 3));

        vX = _mm_mul_ps(vX, row1);
        vY = _mm_mul_ps(vY, row2);
        vZ = _mm_mul_ps(vZ, row3);
        vW = _mm_mul_ps(vW, row4);

        vX = _mm_add_ps(vX, vZ);
        vY = _mm_add_ps(vY, vW);
        vX = _mm_add_ps(vX, vY);

        _mm_store_ps(&result[i * 4], vX);
    }
}

// @performance Consider to replace with 1d array
void mat4_frustum_planes(f32 planes[6][4], f32 radius, f32 *matrix) noexcept
{
    // @todo make this a setting
    // @bug fix to row-major system
    // @todo don't use 2d arrays
    f32 znear = 0.125;
    f32 zfar = radius * 32 + 64;

    f32 *m = matrix;

    planes[0][0] = m[3] + m[0];
    planes[0][1] = m[7] + m[4];
    planes[0][2] = m[11] + m[8];
    planes[0][3] = m[15] + m[12];

    planes[1][0] = m[3] - m[0];
    planes[1][1] = m[7] - m[4];
    planes[1][2] = m[11] - m[8];
    planes[1][3] = m[15] - m[12];

    planes[2][0] = m[3] + m[1];
    planes[2][1] = m[7] + m[5];
    planes[2][2] = m[11] + m[9];
    planes[2][3] = m[15] + m[13];

    planes[3][0] = m[3] - m[1];
    planes[3][1] = m[7] - m[5];
    planes[3][2] = m[11] - m[9];
    planes[3][3] = m[15] - m[13];

    planes[4][0] = znear * m[3] + m[2];
    planes[4][1] = znear * m[7] + m[6];
    planes[4][2] = znear * m[11] + m[10];
    planes[4][3] = znear * m[15] + m[14];

    planes[5][0] = zfar * m[3] - m[2];
    planes[5][1] = zfar * m[7] - m[6];
    planes[5][2] = zfar * m[11] - m[10];
    planes[5][3] = zfar * m[15] - m[14];
}

void mat4_frustum_sparse_rh(
    f32 *matrix,
    f32 left, f32 right, f32 bottom, f32 top,
    f32 znear, f32 zfar
 ) noexcept {
    f32 temp = 2.0f * znear;
    f32 rl_delta = right - left;
    f32 tb_delta = top - bottom;
    f32 fn_delta = zfar - znear;

    matrix[0] = temp / rl_delta;
    //matrix[1] = 0.0f;
    //matrix[2] = 0.0f;
    //matrix[3] = 0.0f;

    //matrix[4] = 0.0f;
    matrix[5] = temp / tb_delta;
    //matrix[6] = 0.0f;
    //matrix[7] = 0.0f;

    matrix[8] = (right + left) / rl_delta;
    matrix[9] = (top + bottom) / tb_delta;
    matrix[10] = -(zfar + znear) / fn_delta;
    matrix[11] = -1.0f;

    //matrix[12] = 0.0f;
    //matrix[13] = 0.0f;
    matrix[14] = (-temp * zfar) / fn_delta;
    //matrix[15] = 0.0f;
}

void mat4_frustum_sparse_lh(
    f32 *matrix,
    f32 left, f32 right, f32 bottom, f32 top,
    f32 znear, f32 zfar
 ) noexcept {
    f32 temp = 2.0f * znear;
    f32 rl_delta = right - left;
    f32 tb_delta = top - bottom;
    f32 fn_delta = zfar - znear;

    matrix[0] = temp / rl_delta;
    //matrix[1] = 0.0f;
    //matrix[2] = 0.0f;
    //matrix[3] = 0.0f;

    //matrix[4] = 0.0f;
    matrix[5] = temp / tb_delta;
    //matrix[6] = 0.0f;
    //matrix[7] = 0.0f;

    matrix[8] = (right + left) / rl_delta;
    matrix[9] = (top + bottom) / tb_delta;
    matrix[10] = (zfar + znear) / fn_delta;
    matrix[11] = 1.0f;

    //matrix[12] = 0.0f;
    //matrix[13] = 0.0f;
    matrix[14] = (temp * zfar) / fn_delta;
    //matrix[15] = 0.0f;
}

// fov needs to be in rad
inline
void mat4_perspective_sparse_lh(
    f32 *matrix, f32 fov, f32 aspect,
    f32 znear, f32 zfar
) noexcept {
    ASSERT_SIMPLE(znear > 0.0f);

    f32 ymax, xmax;
    ymax = znear * tanf(fov * 0.5f);
    xmax = ymax * aspect;

    mat4_frustum_sparse_lh(matrix, -xmax, xmax, -ymax, ymax, znear, zfar);
}

inline
void mat4_perspective_sparse_rh(
    f32 *matrix, f32 fov, f32 aspect,
    f32 znear, f32 zfar
) noexcept {
    ASSERT_SIMPLE(znear > 0.0f);

    f32 ymax, xmax;
    ymax = znear * tanf(fov * 0.5f);
    xmax = ymax * aspect;

    mat4_frustum_sparse_rh(matrix, -xmax, xmax, -ymax, ymax, znear, zfar);
}

void mat4_ortho_sparse_lh(
    f32 *matrix,
    f32 left, f32 right, f32 bottom, f32 top,
    f32 znear, f32 zfar
) noexcept {
    f32 rl_delta = right - left;
    f32 tb_delta = top - bottom;
    f32 fn_delta = zfar - znear;

    matrix[0] = 2.0f / rl_delta;
    //matrix[1] = 0.0f;
    //matrix[2] = 0.0f;
    matrix[3] = -(right + left) / rl_delta;

    //matrix[4] = 0.0f;
    matrix[5] = 2.0f / tb_delta;
    //matrix[6] = 0.0f;
    matrix[7] = -(top + bottom) / tb_delta;

    //matrix[8] = 0.0f;
    //matrix[9] = 0.0f;
    matrix[10] = 2.0f / fn_delta;
    matrix[11] = -(zfar + znear) / fn_delta;

    //matrix[12] = 0.0f;
    //matrix[13] = = 0.0f;
    //matrix[14] = 0.0f;
    matrix[15] = 1.0f;
}

void mat4_ortho_sparse_rh_opengl(
    f32 *matrix,
    f32 left, f32 right, f32 bottom, f32 top,
    f32 znear, f32 zfar
) noexcept {
    f32 rl_delta = right - left;
    f32 tb_delta = top - bottom;
    f32 fn_delta = zfar - znear;

    matrix[0] = 2.0f / rl_delta;
    //matrix[1] = 0.0f;
    //matrix[2] = 0.0f;
    //matrix[3] = 0.0f;

    //matrix[4] = 0.0f;
    matrix[5] = 2.0f / tb_delta;
    //matrix[6] = 0.0f;
    //matrix[7] = 0.0f;

    //matrix[8] = 0.0f;
    //matrix[9] = 0.0f;
    matrix[10] = 2.0f / fn_delta;
    //matrix[11] = 0.0f;

    matrix[12] = -(right + left) / rl_delta;
    matrix[13] = -(top + bottom) / tb_delta;
    matrix[14] = -(zfar + znear) / fn_delta;
    matrix[15] = 1.0f;
}

void mat4_ortho_sparse_rh_vulkan(
    f32 *matrix,
    f32 left, f32 right, f32 bottom, f32 top,
    f32 znear, f32 zfar
) noexcept {
    f32 rl_delta = right - left;
    f32 tb_delta = top - bottom;
    f32 fn_delta = zfar - znear;

    matrix[0] = 2.0f / rl_delta;
    //matrix[1] = 0.0f;
    //matrix[2] = 0.0f;
    //matrix[3] = 0.0f;

    //matrix[4] = 0.0f;
    matrix[5] = -2.0f / tb_delta;
    //matrix[6] = 0.0f;
    //matrix[7] = 0.0f;

    //matrix[8] = 0.0f;
    //matrix[9] = 0.0f;
    matrix[10] = 1.0f / fn_delta;
    //matrix[11] = 0.0f;

    matrix[12] = -(right + left) / rl_delta;
    matrix[13] = (top + bottom) / tb_delta;
    matrix[14] = -znear / fn_delta;
    matrix[15] = 1.0f;
}

void mat4_translate(f32* matrix, f32 dx, f32 dy, f32 dz) noexcept
{
    f32 temp[16];
    memcpy(temp, matrix, sizeof(f32) * 16);

    f32 translation_matrix[16];
    translation_matrix[0] = 1.0f;   translation_matrix[1] = 0.0f;   translation_matrix[2] = 0.0f;   translation_matrix[3] = dx;
    translation_matrix[4] = 0.0f;   translation_matrix[5] = 1.0f;   translation_matrix[6] = 0.0f;   translation_matrix[7] = dy;
    translation_matrix[8] = 0.0f;   translation_matrix[9] = 0.0f;   translation_matrix[10] = 1.0f;  translation_matrix[11] = dz;
    translation_matrix[12] = 0.0f; translation_matrix[13] = 0.0f; translation_matrix[14] = 0.0f; translation_matrix[15] = 1.0f;

    mat4mat4_mult(temp, translation_matrix, matrix);
}

void mat4_translate_simd(f32* matrix, f32 dx, f32 dy, f32 dz) noexcept
{
    alignas(64) f32 temp[16];
    memcpy(temp, matrix, sizeof(f32) * 16);

    alignas(64) f32 translation_matrix[16];
    translation_matrix[0] = 1.0f;   translation_matrix[1] = 0.0f;   translation_matrix[2] = 0.0f;   translation_matrix[3] = dx;
    translation_matrix[4] = 0.0f;   translation_matrix[5] = 1.0f;   translation_matrix[6] = 0.0f;   translation_matrix[7] = dy;
    translation_matrix[8] = 0.0f;   translation_matrix[9] = 0.0f;   translation_matrix[10] = 1.0f;  translation_matrix[11] = dz;
    translation_matrix[12] = 0.0f; translation_matrix[13] = 0.0f; translation_matrix[14] = 0.0f; translation_matrix[15] = 1.0f;

    mat4mat4_mult_simd(temp, translation_matrix, matrix);
}

inline
void mat4_translation(f32* matrix, f32 dx, f32 dy, f32 dz) noexcept
{
    matrix[0] = 1.0f;   matrix[1] = 0.0f;   matrix[2] = 0.0f;   matrix[3] = dx;
    matrix[4] = 0.0f;   matrix[5] = 1.0f;   matrix[6] = 0.0f;   matrix[7] = dy;
    matrix[8] = 0.0f;   matrix[9] = 0.0f;   matrix[10] = 1.0f;  matrix[11] = dz;
    matrix[12] = 0.0f; matrix[13] = 0.0f; matrix[14] = 0.0f; matrix[15] = 1.0f;
}

inline
void mat4_translation_sparse(f32* matrix, f32 dx, f32 dy, f32 dz) noexcept
{
    matrix[3] = dx;
    matrix[7] = dy;
    matrix[11] = dz;
}

inline
void mat4_scale(f32* matrix, f32 dx, f32 dy, f32 dz) noexcept
{
    matrix[0] = dx;   matrix[1] = 0.0f;   matrix[2] = 0.0f;   matrix[3] = 0.0f;
    matrix[4] = 0.0f;   matrix[5] = dy;   matrix[6] = 0.0f;   matrix[7] = 0.0f;
    matrix[8] = 0.0f;   matrix[9] = 0.0f;   matrix[10] = dz;  matrix[11] = 0.0f;
    matrix[12] = 0.0f; matrix[13] = 0.0f; matrix[14] = 0.0f; matrix[15] = 1.0f;
}

inline
void mat4_scale_sparse(f32* matrix, f32 dx, f32 dy, f32 dz) noexcept
{
    matrix[0] = dx;
    matrix[5] = dy;
    matrix[10] = dz;
}

inline
void mat4_transpose(const f32* __restrict matrix, f32* __restrict transposed) noexcept
{
    transposed[1] = matrix[4];
    transposed[2] = matrix[8];
    transposed[3] = matrix[12];
    transposed[4] = matrix[1];
    transposed[6] = matrix[9];
    transposed[7] = matrix[13];
    transposed[8] = matrix[2];
    transposed[9] = matrix[6];
    transposed[11] = matrix[14];
    transposed[12] = matrix[3];
    transposed[13] = matrix[7];
    transposed[14] = matrix[11];
}

inline
void mat4_transpose(f32* matrix) noexcept
{
    f32 temp;

    temp = matrix[1];
    matrix[1] = matrix[4];
    matrix[4] = temp;

    temp = matrix[2];
    matrix[2] = matrix[8];
    matrix[8] = temp;

    temp = matrix[3];
    matrix[3] = matrix[12];
    matrix[12] = temp;

    temp = matrix[6];
    matrix[6] = matrix[9];
    matrix[9] = temp;

    temp = matrix[7];
    matrix[7] = matrix[13];
    matrix[13] = temp;

    temp = matrix[11];
    matrix[11] = matrix[14];
    matrix[14] = temp;
}

inline
void mat3_transpose(const f32* __restrict matrix, f32* __restrict transposed) noexcept
{
    transposed[1] = matrix[3];
    transposed[2] = matrix[6];
    transposed[3] = matrix[1];
    transposed[5] = matrix[7];
    transposed[6] = matrix[2];
    transposed[7] = matrix[5];
}

inline
void mat3_transpose(f32* matrix) noexcept
{
    f32 temp;

    temp = matrix[1];
    matrix[1] = matrix[3];
    matrix[3] = temp;

    temp = matrix[2];
    matrix[2] = matrix[6];
    matrix[6] = temp;

    temp = matrix[5];
    matrix[5] = matrix[7];
    matrix[7] = temp;
}

inline
void mat2_transpose(const f32* __restrict matrix, f32* __restrict transposed) noexcept
{
    transposed[1] = matrix[2];
    transposed[2] = matrix[1];
}

inline
void mat2_transpose(f32* matrix) noexcept
{
    f32 temp = matrix[1];
    matrix[1] = matrix[2];
    matrix[2] = temp;
}

inline
void vec3_normal(
    v3_f32* __restrict normal,
    const v3_f32* __restrict a, const v3_f32* __restrict b, const v3_f32* __restrict c
) noexcept {
    v3_f32 edge1;
    v3_f32 edge2;

    // Calculate two edges of the triangle
    edge1.x = b->x - a->x;
    edge1.y = b->y - a->y;
    edge1.z = b->z - a->z;

    edge2.x = c->x - a->x;
    edge2.y = c->y - a->y;
    edge2.z = c->z - a->z;

    vec3_cross(normal, &edge1, &edge2);
    vec3_normalize(normal);
}

inline
void vec3_barycenter(
    v3_f32* __restrict barycenter,
    const v3_f32* __restrict a, const v3_f32* __restrict b, const v3_f32* __restrict c
) noexcept {
    barycenter->x = (a->x + b->x + c->x) / 3.0f;
    barycenter->y = (a->y + b->y + c->y) / 3.0f;
    barycenter->z = (a->z + b->z + c->z) / 3.0f;
}

#endif
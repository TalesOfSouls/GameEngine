/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef TOS_MATH_MATRIX_FLOAT32_H
#define TOS_MATH_MATRIX_FLOAT32_H

#include "../../stdlib/Intrinsics.h"
#include "../../stdlib/Mathtypes.h"
#include "../../utils/MathUtils.h"
#include "../../utils/TestUtils.h"
#include <math.h>

// @todo Implement intrinsic versions!

void vec2_normalize_f32(float* __restrict x, float* __restrict y)
{
    float d = sqrtf((*x) * (*x) + (*y) * (*y));

    *x /= d;
    *y /= d;
}

inline
void vec2_add(v2_f32* __restrict vec, const v2_f32* a, const v2_f32* b) {
    vec->x = a->x + b->x;
    vec->y = a->y + b->y;
}

inline
void vec2_add(v2_f32* __restrict vec, const v2_f32* b) {
    vec->x += b->x;
    vec->y += b->y;
}

inline
void vec2_sub(v2_f32* __restrict vec, const v2_f32* a, const v2_f32* b) {
    vec->x = a->x - b->x;
    vec->y = a->y - b->y;
}

inline
void vec2_sub(v2_f32* __restrict vec, const v2_f32* b) {
    vec->x -= b->x;
    vec->y -= b->y;
}

inline
void vec2_mul(v2_f32* vec, const v2_f32* a, float s) {
    vec->x = a->x * s;
    vec->y = a->y * s;
}

inline
void vec2_mul(v2_f32* vec, float s) {
    vec->x *= s;
    vec->y *= s;
}

inline
float vec2_mul(const v2_f32* a, const v2_f32* b) {
    return a->x * b->x + a->y * b->y;
}

inline
void vec2_mul(v2_f32* __restrict vec, const v2_f32* a, const v2_f32* b) {
    vec->x = a->x * b->x;
    vec->y = a->y * b->y;
}

inline
void vec2_mul(v2_f32* __restrict vec, const v2_f32* b) {
    vec->x *= b->x;
    vec->y *= b->y;
}

inline
float vec2_cross(const v2_f32* a, const v2_f32* b) {
    return a->x * b->y - a->y * b->x;
}

inline
float vec2_dot(const v2_f32* a, const v2_f32* b) {
    return a->x * b->x + a->y * b->y;
}

void vec3_normalize_f32(float* __restrict x, float* __restrict y, float* __restrict z)
{
    float d = sqrtf((*x) * (*x) + (*y) * (*y) + (*z) * (*z));

    *x /= d;
    *y /= d;
    *z /= d;
}

void vec3_normalize_f32(v3_f32* vec)
{
    float d = sqrtf(vec->x * vec->x + vec->y * vec->y + vec->z * vec->z);

    vec->x /= d;
    vec->y /= d;
    vec->z /= d;
}

inline
void vec3_add(v3_f32* __restrict vec, const v3_f32* a, const v3_f32* b) {
    vec->x = a->x + b->x;
    vec->y = a->y + b->y;
    vec->z = a->z + b->z;
}

inline
void vec3_add(v3_f32* __restrict vec, const v3_f32* b) {
    vec->x += b->x;
    vec->y += b->y;
    vec->z += b->z;
}

inline
void vec3_sub(v3_f32* __restrict vec, const v3_f32* a, const v3_f32* b) {
    vec->x = a->x - b->x;
    vec->y = a->y - b->y;
    vec->z = a->z - b->z;
}

inline
void vec3_sub(v3_f32* __restrict vec, const v3_f32* b) {
    vec->x -= b->x;
    vec->y -= b->y;
    vec->z -= b->z;
}

inline
void vec3_mul(v3_f32* vec, const v3_f32* a, float s) {
    vec->x = a->x * s;
    vec->y = a->y * s;
    vec->z = a->z * s;
}

inline
void vec3_mul(v3_f32* vec, float s) {
    vec->x *= s;
    vec->y *= s;
    vec->z *= s;
}

inline
float vec3_mul(const v3_f32* a, const v3_f32* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

inline
void vec3_mul(v3_f32* __restrict vec, const v3_f32* a, const v3_f32* b) {
    vec->x = a->x * b->x;
    vec->y = a->y * b->y;
    vec->z = a->z * b->z;
}

inline
void vec3_mul(v3_f32* __restrict vec, const v3_f32* b) {
    vec->x *= b->x;
    vec->y *= b->y;
    vec->z *= b->z;
}

void vec3_cross(v3_f32* __restrict vec, const v3_f32* a, const v3_f32* b) {
    vec->x = a->y * b->z - a->z * b->y;
    vec->y = a->z * b->x - a->x * b->z;
    vec->z = a->x * b->y - a->y * b->x;
}

float vec3_dot(const v3_f32* a, const v3_f32* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z;
}

void vec4_normalize_f32(float* __restrict x, float* __restrict y, float* __restrict z, float* __restrict w)
{
    float d = sqrtf((*x) * (*x) + (*y) * (*y) + (*z) * (*z) + (*w) * (*w));

    *x /= d;
    *y /= d;
    *z /= d;
    *w /= d;
}

inline
void vec4_add(v4_f32* __restrict vec, const v4_f32* a, const v4_f32* b) {
    vec->x = a->x + b->x;
    vec->y = a->y + b->y;
    vec->z = a->z + b->z;
    vec->w = a->w + b->w;
}

inline
void vec4_add(v4_f32* __restrict vec, const v4_f32* b) {
    vec->x += b->x;
    vec->y += b->y;
    vec->z += b->z;
    vec->w += b->w;
}

inline
void vec4_sub(v4_f32* __restrict vec, const v4_f32* a, const v4_f32* b) {
    vec->x = a->x - b->x;
    vec->y = a->y - b->y;
    vec->z = a->z - b->z;
    vec->w = a->w - b->w;
}

inline
void vec4_sub(v4_f32* __restrict vec, const v4_f32* b) {
    vec->x -= b->x;
    vec->y -= b->y;
    vec->z -= b->z;
    vec->w -= b->w;
}

inline
void vec4_mul(v4_f32* vec, const v4_f32* a, float s) {
    vec->x = a->x * s;
    vec->y = a->y * s;
    vec->z = a->z * s;
    vec->w = a->w * s;
}

inline
void vec4_mul(v4_f32* vec, float s) {
    vec->x *= s;
    vec->y *= s;
    vec->z *= s;
    vec->w *= s;
}

inline
float vec4_mul(const v4_f32* a, const v4_f32* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

inline
void vec4_mul(v4_f32* __restrict vec, const v4_f32* a, const v4_f32* b) {
    vec->x = a->x * b->x;
    vec->y = a->y * b->y;
    vec->z = a->z * b->z;
    vec->w = a->w * b->w;
}

inline
void vec4_mul(v4_f32* __restrict vec, const v4_f32* b) {
    vec->x *= b->x;
    vec->y *= b->y;
    vec->z *= b->z;
    vec->w *= b->w;
}

inline
float vec4_dot(const v4_f32* a, const v4_f32* b) {
    return a->x * b->x + a->y * b->y + a->z * b->z + a->w * b->w;
}

inline
void vec4_cross(v4_f32* __restrict vec, const v4_f32* a, const v4_f32* b, const v4_f32* c) {
    vec->x = a->y * (b->z * c->w - b->w * c->z) - a->z * (b->y * c->w - b->w * c->y) + a->w * (b->y * c->z - b->z * c->y);
    vec->y = -(a->x * (b->z * c->w - b->w * c->z) - a->z * (b->x * c->w - b->w * c->x) + a->w * (b->x * c->z - b->z * c->x));
    vec->z = a->x * (b->y * c->w - b->w * c->y) - a->y * (b->x * c->w - b->w * c->x) + a->w * (b->x * c->y - b->y * c->x);
    vec->w = -(a->x * (b->y * c->z - b->z * c->y) - a->y * (b->x * c->z - b->z * c->x) + a->z * (b->x * c->y - b->y * c->x));
}

inline
void mat3_identity(float* matrix)
{
    matrix[0] = 1.0f; matrix[1] = 0.0f; matrix[2] = 0.0f;
    matrix[3] = 0.0f; matrix[4] = 1.0f; matrix[5] = 0.0f;
    matrix[6] = 0.0f; matrix[7] = 0.0f; matrix[8] = 1.0f;
}

inline
void mat3_identity_sparse(float* matrix)
{
    matrix[0] = 1.0f; matrix[4] = 1.0f; matrix[8] = 1.0f;
}

inline
void mat3_identity(__m128* matrix)
{
    matrix[0] = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
    matrix[1] = _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f);
    matrix[2] = _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f);
}

inline
void mat4_identity(float* matrix)
{
    matrix[0] = 1.0f;  matrix[1] = 0.0f;  matrix[2] = 0.0f;  matrix[3] = 0.0f;
    matrix[4] = 0.0f;  matrix[5] = 1.0f;  matrix[6] = 0.0f;  matrix[7] = 0.0f;
    matrix[8] = 0.0f;  matrix[9] = 0.0f;  matrix[10] = 1.0f; matrix[11] = 0.0f;
    matrix[12] = 0.0f; matrix[13] = 0.0f; matrix[14] = 0.0f; matrix[15] = 1.0f;
}

inline
void mat4_identity_sparse(float* matrix)
{
    matrix[0] = 1.0f; matrix[5] = 1.0f; matrix[10] = 1.0f; matrix[15] = 1.0f;
}

inline
void mat4_identity(__m128* matrix)
{
    matrix[0] = _mm_set_ps(1.0f, 0.0f, 0.0f, 0.0f);
    matrix[1] = _mm_set_ps(0.0f, 1.0f, 0.0f, 0.0f);
    matrix[2] = _mm_set_ps(0.0f, 0.0f, 1.0f, 0.0f);
    matrix[3] = _mm_set_ps(0.0f, 0.0f, 0.0f, 1.0f);
}

// x, y, z need to be normalized
// https://en.wikipedia.org/wiki/Rodrigues%27_rotation_formula
void mat4_rotation(float* matrix, float x, float y, float z, float angle)
{
    ASSERT_SIMPLE(OMS_ABS(x * x + y * y + z * z - 1.0f) < 0.01);

    // @todo replace with quaternions
    float s = sinf(angle);
    float c = cosf(angle);
    float m = 1 - c;

    float mx = m * x;
    float my = m * y;
    float mz = m * z;

    float xs = x * s;
    float ys = y * s;
    float zs = z * s;

    float mxy = mx * y;
    float mzx = mz * x;
    float myz = my * z;

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

void mat4_rotation(float* matrix, float pitch, float yaw, float roll)
{
    float cos_pitch = cosf(pitch);
    float sin_pitch = sinf(pitch);
    float cos_yaw = cosf(yaw);
    float sin_yaw = sinf(yaw);
    float cos_roll = cosf(roll);
    float sin_roll = sinf(roll);

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
void mat3vec3_mult(const float* __restrict matrix, const float* __restrict vector, float* __restrict result)
{
    result[0] = matrix[0] * vector[0] + matrix[1] * vector[1] + matrix[2] * vector[2];
    result[1] = matrix[3] * vector[0] + matrix[4] * vector[1] + matrix[5] * vector[2];
    result[2] = matrix[6] * vector[0] + matrix[7] * vector[1] + matrix[8] * vector[2];
}

// @question could simple mul add sse be faster?
void mat3vec3_mult_sse(const float* __restrict matrix, const float* __restrict vector, float* __restrict result)
{
    __m128 vec = _mm_loadu_ps(vector);
    vec = _mm_insert_ps(vec, _mm_setzero_ps(), 0x30); // vec[3] = 0

    for (int i = 0; i < 3; ++i) {
        __m128 row = _mm_loadu_ps(&matrix[i * 3]);
        row = _mm_insert_ps(row, _mm_setzero_ps(), 0x30);  // row[3] = 0

        __m128 dot = _mm_dp_ps(row, vec, 0xF1);

        result[i] = _mm_cvtss_f32(dot);
    }
}

// @question could simple mul add sse be faster?
void mat3vec3_mult_sse(const __m128* __restrict matrix, const __m128* __restrict vector, float* __restrict result)
{
    for (int i = 0; i < 3; ++i) {
        __m128 dot = _mm_dp_ps(matrix[i], *vector, 0xF1);

        result[i] = _mm_cvtss_f32(dot);
    }
}

// @question could simple mul add sse be faster?
void mat3vec3_mult_sse(const __m128* __restrict matrix, const __m128* __restrict vector, __m128* __restrict result)
{
    for (int i = 0; i < 4; ++i) {
        result[i] = _mm_dp_ps(matrix[i], *vector, 0xF1);
    }
}

inline
void mat4vec4_mult(const float* __restrict matrix, const float* __restrict vector, float* __restrict result)
{
    result[0] = matrix[0] * vector[0] + matrix[1] * vector[1] + matrix[2] * vector[2] + matrix[3] * vector[3];
    result[1] = matrix[4] * vector[0] + matrix[5] * vector[1] + matrix[6] * vector[2] + matrix[7] * vector[3];
    result[2] = matrix[8] * vector[0] + matrix[9] * vector[1] + matrix[10] * vector[2] + matrix[11] * vector[3];
    result[3] = matrix[12] * vector[0] + matrix[13] * vector[1] + matrix[14] * vector[2] + matrix[15] * vector[3];
}

// @question could simple mul add sse be faster?
void mat4vec4_mult_sse(const float* __restrict matrix, const float* __restrict vector, float* __restrict result)
{
    __m128 vec = _mm_loadu_ps(vector);

    for (int i = 0; i < 4; ++i) {
        __m128 row = _mm_loadu_ps(&matrix[i * 4]);
        __m128 dot = _mm_dp_ps(row, vec, 0xF1);

        result[i] = _mm_cvtss_f32(dot);
    }
}

// @question could simple mul add sse be faster?
void mat4vec4_mult_sse(const __m128* __restrict matrix, const __m128* __restrict vector, float* __restrict result)
{
    for (int i = 0; i < 4; ++i) {
        __m128 dot = _mm_dp_ps(matrix[i], *vector, 0xF1);

        result[i] = _mm_cvtss_f32(dot);
    }
}

// @question could simple mul add sse be faster?
void mat4vec4_mult_sse(const __m128* __restrict matrix, const __m128* __restrict vector, __m128* __restrict result)
{
    for (int i = 0; i < 4; ++i) {
        result[i] = _mm_dp_ps(matrix[i], *vector, 0xF1);
    }
}

inline
void mat4mat4_mult(const float* __restrict a, const float* __restrict b, float* __restrict result)
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

void mat4mat4_mult(const float* __restrict a, const float* __restrict b, float* __restrict result, int steps)
{
    if (steps > 1) {
        // @todo check http://fhtr.blogspot.com/2010/02/4x4-float-matrix-multiplication-using.html
        // @question could simple mul add sse be faster?
        // Load rows of matrix a
        __m128 a_1 = _mm_loadu_ps(a);
        __m128 a_2 = _mm_loadu_ps(&a[4]);
        __m128 a_3 = _mm_loadu_ps(&a[8]);
        __m128 a_4 = _mm_loadu_ps(&a[12]);

        // Load columns of matrix b
        __m128 b_1 = _mm_loadu_ps(b);
        __m128 b_2 = _mm_loadu_ps(&b[4]);
        __m128 b_3 = _mm_loadu_ps(&b[8]);
        __m128 b_4 = _mm_loadu_ps(&b[12]);

        _mm_storeu_ps(&result[0],
            _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(a_1, a_1, _MM_SHUFFLE(0, 0, 0, 0)), b_1),
                    _mm_mul_ps(_mm_shuffle_ps(a_1, a_1, _MM_SHUFFLE(1, 1, 1, 1)), b_2)
                ),
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(a_1, a_1, _MM_SHUFFLE(2, 2, 2, 2)), b_3),
                    _mm_mul_ps(_mm_shuffle_ps(a_1, a_1, _MM_SHUFFLE(3, 3, 3, 3)), b_4)
                )
            )
        );

        _mm_storeu_ps(&result[4],
            _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(a_2, a_2, _MM_SHUFFLE(0, 0, 0, 0)), b_1),
                    _mm_mul_ps(_mm_shuffle_ps(a_2, a_2, _MM_SHUFFLE(1, 1, 1, 1)), b_2)
                ),
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(a_2, a_2, _MM_SHUFFLE(2, 2, 2, 2)), b_3),
                    _mm_mul_ps(_mm_shuffle_ps(a_2, a_2, _MM_SHUFFLE(3, 3, 3, 3)), b_4)
                )
            )
        );

        _mm_storeu_ps(&result[8],
            _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(a_3, a_3, _MM_SHUFFLE(0, 0, 0, 0)), b_1),
                    _mm_mul_ps(_mm_shuffle_ps(a_3, a_3, _MM_SHUFFLE(1, 1, 1, 1)), b_2)
                ),
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(a_3, a_3, _MM_SHUFFLE(2, 2, 2, 2)), b_3),
                    _mm_mul_ps(_mm_shuffle_ps(a_3, a_3, _MM_SHUFFLE(3, 3, 3, 3)), b_4)
                )
            )
        );

        _mm_storeu_ps(&result[12],
            _mm_add_ps(
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(a_4, a_4, _MM_SHUFFLE(0, 0, 0, 0)), b_1),
                    _mm_mul_ps(_mm_shuffle_ps(a_4, a_4, _MM_SHUFFLE(1, 1, 1, 1)), b_2)
                ),
                _mm_add_ps(
                    _mm_mul_ps(_mm_shuffle_ps(a_4, a_4, _MM_SHUFFLE(2, 2, 2, 2)), b_3),
                    _mm_mul_ps(_mm_shuffle_ps(a_4, a_4, _MM_SHUFFLE(3, 3, 3, 3)), b_4)
                )
            )
        );
    } else {
        mat4mat4_mult(a, b, result);
    }
}

void mat4mat4_mult_sse(const __m128* __restrict a, const __m128* __restrict b_transposed, float* __restrict result)
{
    __m128 dot;

    // @question could simple mul add sse be faster?
    // b1
    dot = _mm_dp_ps(a[0], b_transposed[0], 0xF1);
    result[0] = _mm_cvtss_f32(dot);

    dot = _mm_dp_ps(a[1], b_transposed[0], 0xF1);
    result[1] = _mm_cvtss_f32(dot);

    dot = _mm_dp_ps(a[2], b_transposed[0], 0xF1);
    result[2] = _mm_cvtss_f32(dot);

    dot = _mm_dp_ps(a[3], b_transposed[0], 0xF1);
    result[3] = _mm_cvtss_f32(dot);

    // b2
    dot = _mm_dp_ps(a[0], b_transposed[1], 0xF1);
    result[4] = _mm_cvtss_f32(dot);

    dot = _mm_dp_ps(a[1], b_transposed[1], 0xF1);
    result[5] = _mm_cvtss_f32(dot);

    dot = _mm_dp_ps(a[2], b_transposed[1], 0xF1);
    result[6] = _mm_cvtss_f32(dot);

    dot = _mm_dp_ps(a[3], b_transposed[1], 0xF1);
    result[7] = _mm_cvtss_f32(dot);

    // b3
    dot = _mm_dp_ps(a[0], b_transposed[2], 0xF1);
    result[8] = _mm_cvtss_f32(dot);

    dot = _mm_dp_ps(a[1], b_transposed[2], 0xF1);
    result[9] = _mm_cvtss_f32(dot);

    dot = _mm_dp_ps(a[2], b_transposed[2], 0xF1);
    result[10] = _mm_cvtss_f32(dot);

    dot = _mm_dp_ps(a[3], b_transposed[2], 0xF1);
    result[11] = _mm_cvtss_f32(dot);

    // b4
    dot = _mm_dp_ps(a[0], b_transposed[3], 0xF1);
    result[12] = _mm_cvtss_f32(dot);

    dot = _mm_dp_ps(a[1], b_transposed[3], 0xF1);
    result[13] = _mm_cvtss_f32(dot);

    dot = _mm_dp_ps(a[2], b_transposed[3], 0xF1);
    result[14] = _mm_cvtss_f32(dot);

    dot = _mm_dp_ps(a[3], b_transposed[3], 0xF1);
    result[15] = _mm_cvtss_f32(dot);
}

inline
void mat4mat4_mult_sse(const __m128* __restrict a, const __m128* __restrict b_transpose, __m128* __restrict result)
{
    for (int i = 0; i < 4; ++i) {
        result[i] = _mm_mul_ps(a[0], b_transpose[i]);

        for (int j = 1; j < 4; ++j) {
            result[i] = _mm_add_ps(_mm_mul_ps(a[j], b_transpose[4 * j + i]), result[i]);
        }
    }
}

// @performance Consider to replace with 1d array
void mat4_frustum_planes(float planes[6][4], float radius, float *matrix) {
    // @todo make this a setting
    // @bug fix to row-major system
    // @todo don't use 2d arrays
    float znear = 0.125;
    float zfar = radius * 32 + 64;

    float *m = matrix;

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
    float *matrix,
    float left, float right, float bottom, float top,
    float znear, float zfar
 ) {
    float temp, temp2, temp3, temp4;
    temp = 2.0f * znear;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = zfar - znear;

    matrix[0] = temp / temp2;
    //matrix[1] = 0.0f;
    //matrix[2] = 0.0f;
    //matrix[3] = 0.0f;

    //matrix[4] = 0.0f;
    matrix[5] = temp / temp3;
    //matrix[6] = 0.0f;
    //matrix[7] = 0.0f;

    matrix[8] = (right + left) / temp2;
    matrix[9] = (top + bottom) / temp3;
    matrix[10] = -(zfar + znear) / temp4;
    matrix[11] = -1.0f;

    //matrix[12] = 0.0f;
    //matrix[13] = 0.0f;
    matrix[14] = (-temp * zfar) / temp4;
    //matrix[15] = 0.0f;
}

void mat4_frustum_sparse_lh(
    float *matrix,
    float left, float right, float bottom, float top,
    float znear, float zfar
 ) {
    float temp, temp2, temp3, temp4;
    temp = 2.0f * znear;
    temp2 = right - left;
    temp3 = top - bottom;
    temp4 = zfar - znear;

    matrix[0] = temp / temp2;
    //matrix[1] = 0.0f;
    //matrix[2] = 0.0f;
    //matrix[3] = 0.0f;

    //matrix[4] = 0.0f;
    matrix[5] = temp / temp3;
    //matrix[6] = 0.0f;
    //matrix[7] = 0.0f;

    matrix[8] = (right + left) / temp2;
    matrix[9] = (top + bottom) / temp3;
    matrix[10] = (zfar + znear) / temp4;
    matrix[11] = 1.0f;

    //matrix[12] = 0.0f;
    //matrix[13] = 0.0f;
    matrix[14] = (temp * zfar) / temp4;
    //matrix[15] = 0.0f;
}

// fov needs to be in rad
void mat4_perspective_sparse_lh(
    float *matrix, float fov, float aspect,
    float znear, float zfar)
{
    ASSERT_SIMPLE(znear > 0.0f);

    float ymax, xmax;
    ymax = znear * tanf(fov * 0.5f);
    xmax = ymax * aspect;

    mat4_frustum_sparse_lh(matrix, -xmax, xmax, -ymax, ymax, znear, zfar);
}

void mat4_perspective_sparse_rh(
    float *matrix, float fov, float aspect,
    float znear, float zfar)
{
    ASSERT_SIMPLE(znear > 0.0f);

    float ymax, xmax;
    ymax = znear * tanf(fov * 0.5f);
    xmax = ymax * aspect;

    mat4_frustum_sparse_rh(matrix, -xmax, xmax, -ymax, ymax, znear, zfar);
}

void mat4_ortho(
    float *matrix,
    float left, float right, float bottom, float top,
    float near_dist, float far_dist
) {
    float rl_delta = right - left;
    float tb_delta = top - bottom;
    float fn_delta = far_dist - near_dist;

    matrix[0] = 2.0f / rl_delta;
    matrix[1] = 0.0f;
    matrix[2] = 0.0f;
    matrix[3] = 0.0f;

    matrix[4] = 0.0f;
    matrix[5] = 2.0f / tb_delta;
    matrix[6] = 0.0f;
    matrix[7] = 0.0f;

    matrix[8] = 0.0f;
    matrix[9] = 0.0f;
    matrix[10] = -2.0f / fn_delta;
    matrix[11] = 0.0f;

    matrix[12] = -(right + left) / rl_delta;
    matrix[13] = -(top + bottom) / tb_delta;
    matrix[14] = -(far_dist + near_dist) / fn_delta;
    matrix[15] = 1.0f;
}

void mat4_translate(float* matrix, float dx, float dy, float dz)
{
    float temp[16];
    memcpy(temp, matrix, sizeof(float) * 16);

    float translation_matrix[16];
    translation_matrix[0] = 1.0f;   translation_matrix[1] = 0.0f;   translation_matrix[2] = 0.0f;   translation_matrix[3] = dx;
    translation_matrix[4] = 0.0f;   translation_matrix[5] = 1.0f;   translation_matrix[6] = 0.0f;   translation_matrix[7] = dy;
    translation_matrix[8] = 0.0f;   translation_matrix[9] = 0.0f;   translation_matrix[10] = 1.0f;  translation_matrix[11] = dz;
    translation_matrix[12] = 0.0f; translation_matrix[13] = 0.0f; translation_matrix[14] = 0.0f; translation_matrix[15] = 1.0f;

    mat4mat4_mult(temp, translation_matrix, matrix);
}

void mat4_translate(float* matrix, float dx, float dy, float dz, int steps)
{
    alignas(64) float temp[16];
    memcpy(temp, matrix, sizeof(float) * 16);

    alignas(64) float translation_matrix[16];
    translation_matrix[0] = 1.0f;   translation_matrix[1] = 0.0f;   translation_matrix[2] = 0.0f;   translation_matrix[3] = dx;
    translation_matrix[4] = 0.0f;   translation_matrix[5] = 1.0f;   translation_matrix[6] = 0.0f;   translation_matrix[7] = dy;
    translation_matrix[8] = 0.0f;   translation_matrix[9] = 0.0f;   translation_matrix[10] = 1.0f;  translation_matrix[11] = dz;
    translation_matrix[12] = 0.0f; translation_matrix[13] = 0.0f; translation_matrix[14] = 0.0f; translation_matrix[15] = 1.0f;

    mat4mat4_mult(temp, translation_matrix, matrix, steps);
}

inline
void mat4_translation(float* matrix, float dx, float dy, float dz)
{
    matrix[0] = 1.0f;   matrix[1] = 0.0f;   matrix[2] = 0.0f;   matrix[3] = dx;
    matrix[4] = 0.0f;   matrix[5] = 1.0f;   matrix[6] = 0.0f;   matrix[7] = dy;
    matrix[8] = 0.0f;   matrix[9] = 0.0f;   matrix[10] = 1.0f;  matrix[11] = dz;
    matrix[12] = 0.0f; matrix[13] = 0.0f; matrix[14] = 0.0f; matrix[15] = 1.0f;
}

inline
void mat4_translation_sparse(float* matrix, float dx, float dy, float dz)
{
    matrix[3] = dx;
    matrix[7] = dy;
    matrix[11] = dz;
}

inline
void mat4_scale(float* matrix, float dx, float dy, float dz)
{
    matrix[0] = dx;   matrix[1] = 0.0f;   matrix[2] = 0.0f;   matrix[3] = 0.0f;
    matrix[4] = 0.0f;   matrix[5] = dy;   matrix[6] = 0.0f;   matrix[7] = 0.0f;
    matrix[8] = 0.0f;   matrix[9] = 0.0f;   matrix[10] = dz;  matrix[11] = 0.0f;
    matrix[12] = 0.0f; matrix[13] = 0.0f; matrix[14] = 0.0f; matrix[15] = 1.0f;
}

inline
void mat4_scale_sparse(float* matrix, float dx, float dy, float dz)
{
    matrix[0] = dx;
    matrix[5] = dy;
    matrix[10] = dz;
}

inline
void mat4_transpose(const float* __restrict matrix, float* __restrict transposed)
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
void mat4_transpose(float* matrix)
{
    float temp;

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
void mat3_transpose(const float* __restrict matrix, float* __restrict transposed)
{
    transposed[1] = matrix[3];
    transposed[2] = matrix[6];
    transposed[3] = matrix[1];
    transposed[5] = matrix[7];
    transposed[6] = matrix[2];
    transposed[7] = matrix[5];
}

inline
void mat3_transpose(float* matrix)
{
    float temp;

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
void mat2_transpose(const float* __restrict matrix, float* __restrict transposed)
{
    transposed[1] = matrix[2];
    transposed[2] = matrix[1];
}

inline
void mat2_transpose(float* matrix)
{
    float temp = matrix[1];
    matrix[1] = matrix[2];
    matrix[2] = temp;
}

#endif
/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_F32_H
#define COMS_STDLIB_SIMD_F32_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Types.h"
#include "SIMD_SVML_AVX2.h"

struct f32_8 {
    union {
        #if ARM
            svfloat32_t s;
        #else
            __m256 s;
        #endif

        f32 v[8];
    };
};

inline f32_8 load_f32_8(const f32* mem)
{
    f32_8 simd;
    simd.s = _mm256_load_ps(mem);

    return simd;
}

inline f32_8 init_f32_8(const f32* mem)
{
    f32_8 simd;
    simd.s = _mm256_set_ps(mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7]);

    return simd;
}

inline void unload_f32_8(f32_8 a, f32 *array) { _mm256_store_ps(array, a.s); }

inline f32_8 init_zero_f32_8()
{
    f32_8 simd;
    simd.s = _mm256_setzero_ps();

    return simd;
}

inline f32_8 init_value_f32_8(f32 value)
{
    f32_8 simd;
    simd.s = _mm256_set1_ps(value);

    return simd;
}

inline f32_8 init_values_f32_8(
    f32 a, f32 b, f32 c, f32 d,
    f32 e, f32 f, f32 g, f32 h
)
{
    f32_8 simd;
    simd.s = _mm256_set_ps(a, b, c, d, e, f, g, h);

    return simd;
}

inline f32_8 operator+(f32_8 a, f32_8 b)
{
    f32_8 simd;
    simd.s = _mm256_add_ps(a.s, b.s);

    return simd;
}

inline f32_8 operator-(f32_8 a, f32_8 b)
{
    f32_8 simd;
    simd.s = _mm256_sub_ps(a.s, b.s);

    return simd;
}

inline f32_8 operator-(f32_8 a) { return init_zero_f32_8() - a; }

inline f32_8 operator*(f32_8 a, f32_8 b)
{
    f32_8 simd;
    simd.s = _mm256_mul_ps(a.s, b.s);

    return simd;
}

inline f32_8 operator/(f32_8 a, f32_8 b)
{
    f32_8 simd;
    simd.s = _mm256_div_ps(a.s, b.s);

    return simd;
}

inline f32_8 operator^(f32_8 a, f32_8 b)
{
    f32_8 simd;
    simd.s = _mm256_xor_ps(a.s, b.s);

    return simd;
}

inline f32_8 &operator-=(f32_8 &a, f32_8 b)
{
    a = a - b;

    return a;
}

inline f32_8 &operator+=(f32_8 &a, f32_8 b)
{
    a = a + b;

    return a;
}

inline f32_8 &operator*=(f32_8 &a, f32_8 b)
{
    a = a * b;

    return a;
}

inline f32_8 &operator/=(f32_8 &a, f32_8 b)
{
    a = a / b;

    return a;
}

inline f32_8 &operator^=(f32_8 &a, f32_8 b)
{
    a = a ^ b;

    return a;
}

inline f32_8 operator<(f32_8 a, f32_8 b)
{
    f32_8 simd;
    simd.s = _mm256_cmp_ps(a.s, b.s, _CMP_LT_OQ);

    return simd;
}

inline f32_8 operator<=(f32_8 a, f32_8 b)
{
    f32_8 simd;
    simd.s = _mm256_cmp_ps(a.s, b.s, _CMP_LE_OQ);

    return simd;
}

inline f32_8 operator>(f32_8 a, f32_8 b)
{
    f32_8 simd;
    simd.s = _mm256_cmp_ps(a.s, b.s, _CMP_GT_OQ);

    return simd;
}

inline f32_8 operator>=(f32_8 a, f32_8 b)
{
    f32_8 simd;
    simd.s = _mm256_cmp_ps(a.s, b.s, _CMP_GE_OQ);

    return simd;
}

inline f32_8 operator==(f32_8 a, f32_8 b)
{
    f32_8 simd;
    simd.s = _mm256_cmp_ps(a.s, b.s, _CMP_EQ_OQ);

    return simd;
}

inline f32_8 operator!=(f32_8 a, f32_8 b)
{
    f32_8 simd;
    simd.s = _mm256_cmp_ps(a.s, b.s, _CMP_NEQ_OQ);

    return simd;
}

inline f32_8 operator&(f32_8 a, f32_8 b)
{
    f32_8 simd;
    simd.s = _mm256_and_ps(a.s, b.s);

    return simd;
}

inline f32_8 operator|(f32_8 a, f32_8 b)
{
    f32_8 simd;
    simd.s = _mm256_or_ps(a.s, b.s);

    return simd;
}

inline f32_8 &operator&=(f32_8 &a, f32_8 b)
{
    a = a & b;

    return a;
}

inline f32_8 &operator|=(f32_8 &a, f32_8 b)
{
    a = a | b;

    return a;
}

inline f32_8 abs(f32_8 a)
{
    uint32 unsigned_mask = (uint32) (1U << 31);
    __m256 mask                = _mm256_set1_ps(*(f32 *) &unsigned_mask);

    f32_8 simd;
    simd.s = _mm256_and_ps(a.s, mask);

    return simd;
}

inline f32_8 simd_min(f32_8 a, f32_8 b)
{
    f32_8 simd;
    simd.s = _mm256_min_ps(a.s, b.s);

    return simd;
}

inline f32_8 simd_max(f32_8 a, f32_8 b)
{
    f32_8 simd;
    simd.s = _mm256_max_ps(a.s, b.s);

    return simd;
}

inline f32_8 sign(f32_8 a)
{
    uint32 umask = (uint32) (1U << 31);
    __m256 mask        = _mm256_set1_ps(*(f32 *) &umask);

    f32_8 signBit;
    signBit.s = _mm256_and_ps(a.s, mask);

    f32_8 b;
    b.s = _mm256_set1_ps(1.0f);

    f32_8 simd = b | signBit;

    return simd;
}

inline f32_8 floor(f32_8 a)
{
    f32_8 simd;
    simd.s = _mm256_floor_ps(a.s);

    return simd;
}

inline f32_8 ceil(f32_8 a)
{
    f32_8 simd;
    simd.s = _mm256_ceil_ps(a.s);

    return simd;
}

inline f32_8 sqrt(f32_8 a)
{
    f32_8 simd;
    simd.s = _mm256_sqrt_ps(a.s);

    return simd;
}

inline f32_8 sqrt_inv_approx(f32_8 a)
{
    f32_8 simd;
    simd.s = _mm256_rsqrt_ps(a.s);

    return simd;
}

inline f32_8 one_over_approx(f32_8 a)
{
    f32_8 simd;
    simd.s = _mm256_rcp_ps(a.s);

    return simd;
}

inline f32_8 clamp(f32_8 min_value, f32_8 a, f32_8 max_value)
{
    return simd_min(simd_max(a, min_value), max_value);
}

inline int32 which_true(f32_8 a)
{
    int32 which_true = _mm256_movemask_ps(a.s);

    return which_true;
}

inline bool any_true(f32_8 a)
{
    bool is_any_true = _mm256_movemask_ps(a.s) > 0;

    return is_any_true;
}

inline bool all_true(f32_8 a)
{
    bool is_true = _mm256_movemask_ps(a.s) == 255;

    return is_true;
}

inline bool all_false(f32_8 a)
{
    bool is_false = _mm256_movemask_ps(a.s) == 0;

    return is_false;
}

inline
void simd_cmp_le(const __m256* a, f32 b, bool* result, int32 size)
{
    __m256 b_8 = _mm256_set1_ps(b);

    for (int32 i = 0; i < size; ++i) {
        int32 mask = _mm256_movemask_ps(_mm256_cmp_ps(a[i], b_8, _CMP_LE_OQ));

        for (int32 j = 0; j < 8; ++j) {
            result[i * 8 + j] = (mask & (1 << j)) != 0;
        }
    }
}

inline
f32_8 simd_sin(f32_8 a)
{
    f32_8 simd;
    simd.s = _mm256_sin_ps(a.s);

    return simd;
}

inline
f32_8 simd_cos(f32_8 a)
{
    f32_8 simd;
    simd.s = _mm256_cos_ps(a.s);

    return simd;
}

inline
f32_8 simd_asin(f32_8 a)
{
    f32_8 simd;
    simd.s = _mm256_asin_ps(a.s);

    return simd;
}

inline
f32_8 simd_acos(f32_8 a)
{
    f32_8 simd;
    simd.s = _mm256_acos_ps(a.s);

    return simd;
}

inline
void simd_div(const f32* a, f32 b, __m256* result, int32 size)
{
    int32 i = 0;
    int32 j = 0;

    // @todo this his how all the functions should be implemented that take in baseic types and output basic types
    __m256 a_8;
    __m256 b_8 = _mm256_set1_ps(b);
    __m256 result_8;

    for (; i <= size - 8; i += 8) {
        a_8 = _mm256_load_ps(a);
        result_8 = _mm256_div_ps(a_8, b_8);
        result[j] = result_8;

        a += 8;
        ++j;
    }

    int32 diff = size - i;
    alignas(32) f32 temp[8];

    for (int32 k = 0; k < diff; k++) {
        temp[k] = a[i + k] / b;
    }

    result[j] = _mm256_load_ps(temp);
}

#endif

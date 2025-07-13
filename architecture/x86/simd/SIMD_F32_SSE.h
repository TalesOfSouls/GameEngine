/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_F32_SSE_H
#define COMS_STDLIB_SIMD_F32_SSE_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Types.h"
#include "SIMD_SVML_SSE.h"

struct f32_4 {
    union {
        #if ARM
            svfloat32_t s;
        #else
            __m128 s;
        #endif

        f32 v[4];
    };
};

inline f32_4 load_f32_4(const f32* mem)
{
    f32_4 simd;
    simd.s = _mm_load_ps(mem);

    return simd;
}

inline f32_4 init_f32_4(const f32* mem)
{
    f32_4 simd;
    simd.s = _mm_set_ps(mem[0], mem[1], mem[2], mem[3]);

    return simd;
}

inline void unload_f32_4(f32_4 a, f32 *array) { _mm_store_ps(array, a.s); }

inline f32_4 init_zero_f32_4()
{
    f32_4 simd;
    simd.s = _mm_setzero_ps();

    return simd;
}

inline f32_4 init_value_f32_4(f32 value)
{
    f32_4 simd;
    simd.s = _mm_set1_ps(value);

    return simd;
}

inline f32_4 init_values_f32_4(f32 a, f32 b, f32 c, f32 d)
{
    f32_4 simd;
    simd.s = _mm_set_ps(a, b, c, d);

    return simd;
}

inline f32_4 operator+(f32_4 a, f32_4 b)
{
    f32_4 simd;
    simd.s = _mm_add_ps(a.s, b.s);

    return simd;
}

inline f32_4 operator-(f32_4 a, f32_4 b)
{
    f32_4 simd;
    simd.s = _mm_sub_ps(a.s, b.s);

    return simd;
}

inline f32_4 operator-(f32_4 a) { return init_zero_f32_4() - a; }

inline f32_4 operator*(f32_4 a, f32_4 b)
{
    f32_4 simd;
    simd.s = _mm_mul_ps(a.s, b.s);

    return simd;
}

inline f32_4 operator/(f32_4 a, f32_4 b)
{
    f32_4 simd;
    simd.s = _mm_div_ps(a.s, b.s);

    return simd;
}

inline f32_4 operator^(f32_4 a, f32_4 b)
{
    f32_4 simd;
    simd.s = _mm_xor_ps(a.s, b.s);

    return simd;
}

inline f32_4 &operator-=(f32_4 &a, f32_4 b)
{
    a = a - b;

    return a;
}

inline f32_4 &operator+=(f32_4 &a, f32_4 b)
{
    a = a + b;

    return a;
}

inline f32_4 &operator*=(f32_4 &a, f32_4 b)
{
    a = a * b;

    return a;
}

inline f32_4 &operator/=(f32_4 &a, f32_4 b)
{
    a = a / b;

    return a;
}

inline f32_4 &operator^=(f32_4 &a, f32_4 b)
{
    a = a ^ b;

    return a;
}

inline f32_4 operator<(f32_4 a, f32_4 b)
{
    f32_4 simd;
    simd.s = _mm_cmplt_ps(a.s, b.s);

    return simd;
}

inline f32_4 operator<=(f32_4 a, f32_4 b)
{
    f32_4 simd;
    simd.s = _mm_cmple_ps(a.s, b.s);

    return simd;
}

inline f32_4 operator>(f32_4 a, f32_4 b)
{
    f32_4 simd;
    simd.s = _mm_cmpgt_ps(a.s, b.s);

    return simd;
}

inline f32_4 operator>=(f32_4 a, f32_4 b)
{
    f32_4 simd;
    simd.s = _mm_cmpge_ps(a.s, b.s);

    return simd;
}

inline f32_4 operator==(f32_4 a, f32_4 b)
{
    f32_4 simd;
    simd.s = _mm_cmpeq_ps(a.s, b.s);

    return simd;
}

inline f32_4 operator!=(f32_4 a, f32_4 b)
{
    f32_4 simd;
    simd.s = _mm_cmpneq_ps(a.s, b.s);

    return simd;
}

inline f32_4 operator&(f32_4 a, f32_4 b)
{
    f32_4 simd;
    simd.s = _mm_and_ps(a.s, b.s);

    return simd;
}

inline f32_4 operator|(f32_4 a, f32_4 b)
{
    f32_4 simd;
    simd.s = _mm_or_ps(a.s, b.s);

    return simd;
}

inline f32_4 &operator&=(f32_4 &a, f32_4 b)
{
    a = a & b;

    return a;
}

inline f32_4 &operator|=(f32_4 &a, f32_4 b)
{
    a = a | b;

    return a;
}

inline f32_4 abs(f32_4 a)
{
    uint32 unsigned_mask = (uint32) (1U << 31);
    __m128 mask                = _mm_set1_ps(*(f32 *) &unsigned_mask);

    f32_4 simd;
    simd.s = _mm_and_ps(a.s, mask);

    return simd;
}

inline f32_4 simd_min(f32_4 a, f32_4 b)
{
    f32_4 simd;
    simd.s = _mm_min_ps(a.s, b.s);

    return simd;
}

inline f32_4 simd_max(f32_4 a, f32_4 b)
{
    f32_4 simd;
    simd.s = _mm_max_ps(a.s, b.s);

    return simd;
}

inline f32_4 sign(f32_4 a)
{
    uint32 umask = (uint32) (1U << 31);
    __m128 mask        = _mm_set1_ps(*(f32 *) &umask);

    f32_4 signBit;
    signBit.s = _mm_and_ps(a.s, mask);

    f32_4 b;
    b.s = _mm_set1_ps(1.0f);

    f32_4 simd = b | signBit;

    return simd;
}

inline f32_4 floor(f32_4 a)
{
    f32_4 simd;
    simd.s = _mm_floor_ps(a.s);

    return simd;
}

inline f32_4 ceil(f32_4 a)
{
    f32_4 simd;
    simd.s = _mm_ceil_ps(a.s);

    return simd;
}

inline f32_4 sqrt(f32_4 a)
{
    f32_4 simd;
    simd.s = _mm_sqrt_ps(a.s);

    return simd;
}

inline f32_4 sqrt_inv_approx(f32_4 a)
{
    f32_4 simd;
    simd.s = _mm_rsqrt_ps(a.s);

    return simd;
}

inline f32_4 one_over_approx(f32_4 a)
{
    f32_4 simd;
    simd.s = _mm_rcp_ps(a.s);

    return simd;
}

inline f32_4 clamp(f32_4 min_value, f32_4 a, f32_4 max_value)
{
    return simd_min(simd_max(a, min_value), max_value);
}

inline int32 which_true(f32_4 a)
{
    int32 which_true = _mm_movemask_ps(a.s);

    return which_true;
}

inline bool any_true(f32_4 a)
{
    bool is_any_true = _mm_movemask_ps(a.s) > 0;

    return is_any_true;
}

inline bool all_true(f32_4 a)
{
    bool is_true = _mm_movemask_ps(a.s) == 15;

    return is_true;
}

inline bool all_false(f32_4 a)
{
    bool is_false = _mm_movemask_ps(a.s) == 0;

    return is_false;
}

inline
f32_4 simd_sin(f32_4 a)
{
    f32_4 simd;
    simd.s = _mm_sin_ps(a.s);

    return simd;
}

inline
f32_4 simd_cos(f32_4 a)
{
    f32_4 simd;
    simd.s = _mm_cos_ps(a.s);

    return simd;
}

inline
f32_4 simd_asin(f32_4 a)
{
    f32_4 simd;
    simd.s = _mm_asin_ps(a.s);

    return simd;
}

inline
f32_4 simd_acos(f32_4 a)
{
    f32_4 simd;
    simd.s = _mm_acos_ps(a.s);

    return simd;
}

// @todo implement more trigonometry function

#endif

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
#include "SIMD_SVML_AVX512.h"

struct f32_16 {
    union {
        #if ARM
            svfloat32_t s;
        #else
            __m512 s;
        #endif

        f32 v[16];
    };
};

inline f32_16 load_f32_16(const f32* mem)
{
    f32_16 simd;
    simd.s = _mm512_load_ps(mem);

    return simd;
}

inline f32_16 init_f32_16(const f32* mem)
{
    f32_16 simd;
    simd.s = _mm512_set_ps(mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7], mem[8], mem[9], mem[10],
                            mem[11], mem[12], mem[13], mem[14], mem[15]);

    return simd;
}

inline void unload_f32_16(f32_16 a, f32 *array) { _mm512_store_ps(array, a.s); }

inline f32_16 init_zero_f32_16()
{
    f32_16 simd;
    simd.s = _mm512_setzero_ps();

    return simd;
}

inline f32_16 init_value_f32_16(f32 value)
{
    f32_16 simd;
    simd.s = _mm512_set1_ps(value);

    return simd;
}

inline f32_16 init_values_f32_16(
    f32 a, f32 b, f32 c, f32 d,
    f32 e, f32 f, f32 g, f32 h,
    f32 i, f32 j, f32 k, f32 l,
    f32 m, f32 n, f32 o, f32 p
)
{
    f32_16 simd;
    simd.s = _mm512_set_ps(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);

    return simd;
}

inline f32_16 operator+(f32_16 a, f32_16 b)
{
    f32_16 simd;
    simd.s = _mm512_add_ps(a.s, b.s);

    return simd;
}

inline f32_16 operator-(f32_16 a, f32_16 b)
{
    f32_16 simd;
    simd.s = _mm512_sub_ps(a.s, b.s);

    return simd;
}

inline f32_16 operator-(f32_16 a) { return init_zero_f32_16() - a; }

inline f32_16 operator*(f32_16 a, f32_16 b)
{
    f32_16 simd;
    simd.s = _mm512_mul_ps(a.s, b.s);

    return simd;
}

inline f32_16 operator/(f32_16 a, f32_16 b)
{
    f32_16 simd;
    simd.s = _mm512_div_ps(a.s, b.s);

    return simd;
}

inline f32_16 operator^(f32_16 a, f32_16 b)
{
    f32_16 simd;
    simd.s = _mm512_xor_ps(a.s, b.s);

    return simd;
}

inline f32_16 &operator-=(f32_16 &a, f32_16 b)
{
    a = a - b;

    return a;
}

inline f32_16 &operator+=(f32_16 &a, f32_16 b)
{
    a = a + b;

    return a;
}

inline f32_16 &operator*=(f32_16 &a, f32_16 b)
{
    a = a * b;

    return a;
}

inline f32_16 &operator/=(f32_16 &a, f32_16 b)
{
    a = a / b;

    return a;
}

inline f32_16 &operator^=(f32_16 &a, f32_16 b)
{
    a = a ^ b;

    return a;
}

inline f32_16 operator<(f32_16 a, f32_16 b)
{
    f32_16 simd;
    simd.s = _mm512_mask_blend_ps(_mm512_cmplt_ps_mask(a.s, b.s), a.s, b.s);

    return simd;
}

inline f32_16 operator<=(f32_16 a, f32_16 b)
{
    f32_16 simd;
    simd.s = _mm512_mask_blend_ps(_mm512_cmp_ps_mask(a.s, b.s, _CMP_LE_OQ), a.s, b.s);

    return simd;
}

inline f32_16 operator>(f32_16 a, f32_16 b)
{
    f32_16 simd;
    simd.s = _mm512_mask_blend_ps(_mm512_cmp_ps_mask(a.s, b.s, _CMP_GT_OQ), a.s, b.s);

    return simd;
}

inline f32_16 operator>=(f32_16 a, f32_16 b)
{
    f32_16 simd;
    simd.s = _mm512_mask_blend_ps(_mm512_cmp_ps_mask(a.s, b.s, _CMP_GE_OQ), a.s, b.s);

    return simd;
}

inline f32_16 operator==(f32_16 a, f32_16 b)
{
    f32_16 simd;
    simd.s = _mm512_mask_blend_ps(_mm512_cmp_ps_mask(a.s, b.s, _CMP_EQ_OQ), a.s, b.s);

    return simd;
}

inline f32_16 operator!=(f32_16 a, f32_16 b)
{
    f32_16 simd;
    simd.s = _mm512_mask_blend_ps(_mm512_cmp_ps_mask(a.s, b.s, _CMP_NEQ_OQ), a.s, b.s);

    return simd;
}

inline f32_16 operator&(f32_16 a, f32_16 b)
{
    f32_16 simd;
    simd.s = _mm512_and_ps(a.s, b.s);

    return simd;
}

inline f32_16 operator|(f32_16 a, f32_16 b)
{
    f32_16 simd;
    simd.s = _mm512_or_ps(a.s, b.s);

    return simd;
}

inline f32_16 &operator&=(f32_16 &a, f32_16 b)
{
    a = a & b;

    return a;
}

inline f32_16 &operator|=(f32_16 &a, f32_16 b)
{
    a = a | b;

    return a;
}

inline f32_16 abs(f32_16 a)
{
    f32_16 simd;
    simd.s = _mm512_abs_ps(a.s);

    return simd;
}

inline f32_16 simd_min(f32_16 a, f32_16 b)
{
    f32_16 simd;
    simd.s = _mm512_min_ps(a.s, b.s);

    return simd;
}

inline f32_16 simd_max(f32_16 a, f32_16 b)
{
    f32_16 simd;
    simd.s = _mm512_max_ps(a.s, b.s);

    return simd;
}

inline f32_16 sign(f32_16 a)
{
    uint32 umask = (uint32) (1U << 31);
    __m512 mask        = _mm512_set1_ps(*(f32 *) &umask);

    f32_16 signBit;
    signBit.s = _mm512_and_ps(a.s, mask);

    f32_16 b;
    b.s = _mm512_set1_ps(1.0f);

    f32_16 simd = b | signBit;

    return simd;
}

inline f32_16 floor(f32_16 a)
{
    f32_16 simd;
    simd.s = _mm512_floor_ps(a.s);

    return simd;
}

inline f32_16 ceil(f32_16 a)
{
    f32_16 simd;
    simd.s = _mm512_ceil_ps(a.s);

    return simd;
}

inline f32_16 sqrt(f32_16 a)
{
    f32_16 simd;
    simd.s = _mm512_sqrt_ps(a.s);

    return simd;
}

inline f32_16 sqrt_inv_approx(f32_16 a)
{
    f32_16 simd;
    simd.s = _mm512_rsqrt14_ps(a.s);

    return simd;
}

inline f32_16 one_over_approx(f32_16 a)
{
    f32_16 simd;
    simd.s = _mm512_rcp14_ps(a.s);

    return simd;
}

inline f32_16 clamp(f32_16 min_value, f32_16 a, f32_16 max_value)
{
    return simd_min(simd_max(a, min_value), max_value);
}

inline int32 which_true(f32_16 a)
{
    int32 which_true = _mm512_movepi32_mask(_mm512_castps_si512(a.s));

    return which_true;
}

inline bool any_true(f32_16 a)
{
    bool is_any_true = _mm512_movepi32_mask(_mm512_castps_si512(a.s)) > 0;

    return is_any_true;
}

inline bool all_true(f32_16 a)
{
    bool is_true = _mm512_movepi32_mask(_mm512_castps_si512(a.s)) == 65535;

    return is_true;
}

inline bool all_false(f32_16 a)
{
    // @todo This can be optimized (requires also changes in the comparison functions return)
    bool is_false = _mm512_movepi32_mask(_mm512_castps_si512(a.s)) == 0;

    return is_false;
}

inline
f32_16 simd_sin(f32_16 a)
{
    f32_16 simd;
    simd.s = _mm512_sin_ps(a.s);

    return simd;
}

inline
f32_16 simd_cos(f32_16 a)
{
    f32_16 simd;
    simd.s = _mm512_cos_ps(a.s);

    return simd;
}

inline
f32_16 simd_asin(f32_16 a)
{
    f32_16 simd;
    simd.s = _mm512_asin_ps(a.s);

    return simd;
}

inline
f32_16 simd_acos(f32_16 a)
{
    f32_16 simd;
    simd.s = _mm512_acos_ps(a.s);

    return simd;
}

// @todo implement more trigonometry function

#endif

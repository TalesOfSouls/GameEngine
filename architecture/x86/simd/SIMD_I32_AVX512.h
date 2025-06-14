/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_I32_H
#define COMS_STDLIB_SIMD_I32_H

#include <immintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>

#include "../../../stdlib/Types.h"
#include "SIMD_SVML_AVX512.h"

// @todo a lot of sse functions require high level (e.g. sse4.1) this needs to be changed to be more general
//      or better create alternative functions for the available sse version.

// @question why are we passing structs by value?
struct int32_16 {
    union {
        #if ARM
            svint32_t s;
        #else
            __m512i s;
        #endif

        int32 v[16];
    };
};

inline int32_16 load_int32_16(const int32* mem)
{
    int32_16 simd;
    simd.s = _mm512_load_epi32(mem);

    return simd;
}

inline int32_16 init_int32_16(const int32* mem)
{
    int32_16 simd;
    simd.s = _mm512_set_epi32(
        mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7],
        mem[8], mem[9], mem[10], mem[11], mem[12], mem[13], mem[14], mem[15]);

    return simd;
}

inline void unload_int32_16(int32_16 a, int32 *array) { _mm512_store_epi32(array, a.s); }

inline int32_16 init_zero_int32_16()
{
    int32_16 simd;
    simd.s = _mm512_setzero_epi32();

    return simd;
}

inline int32_16 init_value_int32_16(int32 value)
{
    int32_16 simd;
    simd.s = _mm512_set1_epi32(value);

    return simd;
}

inline int32_16 init_values_int32_16(
    int32 a, int32 b, int32 c, int32 d,
    int32 e, int32 f, int32 g, int32 h,
    int32 i, int32 j, int32 k, int32 l,
    int32 m, int32 n, int32 o, int32 p
)
{
    int32_16 simd;
    simd.s = _mm512_set_epi32(a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p);

    return simd;
}

inline int32_16 operator+(int32_16 a, int32_16 b)
{
    int32_16 simd;
    simd.s = _mm512_add_epi32(a.s, b.s);

    return simd;
}

inline int32_16 operator-(int32_16 a, int32_16 b)
{
    int32_16 simd;
    simd.s = _mm512_sub_epi32(a.s, b.s);

    return simd;
}

inline int32_16 operator-(int32_16 a) { return init_zero_int32_16() - a; }

inline int32_16 operator*(int32_16 a, int32_16 b)
{
    int32_16 simd;
    simd.s = _mm512_mul_epi32(a.s, b.s);

    return simd;
}

inline int32_16 operator/(int32_16 a, int32_16 b)
{
    int32_16 simd;
    simd.s = _mm512_div_epi32(a.s, b.s);

    return simd;
}

inline int32_16 operator^(int32_16 a, int32_16 b)
{
    int32_16 simd;
    simd.s = _mm512_xor_epi32(a.s, b.s);

    return simd;
}

inline int32_16 &operator-=(int32_16 &a, int32_16 b)
{
    a = a - b;

    return a;
}

inline int32_16 &operator+=(int32_16 &a, int32_16 b)
{
    a = a + b;

    return a;
}

inline int32_16 &operator*=(int32_16 &a, int32_16 b)
{
    a = a * b;

    return a;
}

inline int32_16 &operator/=(int32_16 &a, int32_16 b)
{
    a.s = (a / b).s;

    return a;
}

inline int32_16 &operator^=(int32_16 &a, int32_16 b)
{
    a = a ^ b;

    return a;
}

inline int32_16 operator<(int32_16 a, int32_16 b)
{
    int32_16 simd;
    simd.s = _mm512_mask_blend_epi32(_mm512_cmplt_epi32_mask(a.s, b.s), a.s, b.s);

    return simd;
}

inline int32_16 operator<=(int32_16 a, int32_16 b)
{
    int32_16 simd;
    simd.s = _mm512_mask_blend_epi32(_mm512_knot(_mm512_cmpgt_epi32_mask(b.s, a.s)), b.s, a.s);

    return simd;
}

inline int32_16 operator>(int32_16 a, int32_16 b)
{
    int32_16 simd;
    simd.s = _mm512_mask_blend_epi32(_mm512_cmpgt_epi32_mask(a.s, b.s), a.s, b.s);

    return simd;
}

inline int32_16 operator>=(int32_16 a, int32_16 b)
{
    int32_16 simd;
    simd.s = _mm512_mask_blend_epi32(_mm512_cmpge_epi32_mask(a.s, b.s), a.s, b.s);

    return simd;
}

inline int32_16 operator==(int32_16 a, int32_16 b)
{
    int32_16 simd;
    simd.s = _mm512_mask_blend_epi32(_mm512_cmpeq_epi32_mask(a.s, b.s), a.s, b.s);

    return simd;
}

inline int32_16 operator!=(int32_16 a, int32_16 b)
{
    int32_16 simd;
    simd.s = _mm512_mask_blend_epi32(_mm512_cmp_epi32_mask(a.s, b.s, _MM_CMPINT_NE), a.s, b.s);

    return simd;
}

inline int32_16 operator&(int32_16 a, int32_16 b)
{
    int32_16 simd;
    simd.s = _mm512_and_si512(a.s, b.s);

    return simd;
}

inline int32_16 operator|(int32_16 a, int32_16 b)
{
    int32_16 simd;
    simd.s = _mm512_or_epi32(a.s, b.s);

    return simd;
}

inline int32_16 &operator&=(int32_16 &a, int32_16 b)
{
    a = a & b;

    return a;
}

inline int32_16 &operator|=(int32_16 &a, int32_16 b)
{
    a = a | b;

    return a;
}

inline int32_16 abs(int32_16 a)
{
    int32_16 simd;
    simd.s = _mm512_abs_epi64(a.s);

    return simd;
}

inline int32_16 simd_min(int32_16 a, int32_16 b)
{
    int32_16 simd;
    simd.s = _mm512_min_epi32(a.s, b.s);

    return simd;
}

inline int32_16 simd_max(int32_16 a, int32_16 b)
{
    int32_16 simd;
    simd.s = _mm512_max_epi32(a.s, b.s);

    return simd;
}

inline int32_16 sign(int32_16 a)
{
    __m512i mask = _mm512_set1_epi32(0x80000000);
    __m512i signBit = _mm512_and_si512(a.s, mask);
    __m512i b = _mm512_set1_epi32(1);
    int32_16 simd;

    simd.s = _mm512_or_si512(b, signBit);

    return simd;
}

inline int32_16 clamp(int32_16 min_value, int32_16 a, int32_16 max_value)
{
    return simd_min(simd_max(a, min_value), max_value);
}

inline int32 which_true(int32_16 a)
{
    int32 which_true = _mm512_movepi32_mask(a.s);

    return which_true;
}

inline bool any_true(int32_16 a)
{
    bool is_any_true = _mm512_movepi32_mask(a.s) > 0;

    return is_any_true;
}

inline bool all_true(int32_16 a)
{
    bool is_true = _mm512_movepi32_mask(a.s) == 65535;

    return is_true;
}

inline bool all_false(int32_16 a)
{
    // @todo This can be optimized (requires also changes in the comparison functions return)
    bool is_false = _mm512_movepi32_mask(a.s) == 0;

    return is_false;
}

#endif

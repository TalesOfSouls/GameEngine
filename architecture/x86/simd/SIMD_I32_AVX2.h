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

// @todo a lot of sse functions require high level (e.g. sse4.1) this needs to be changed to be more general
//      or better create alternative functions for the available sse version.

// @question why are we passing structs by value?
struct int32_8 {
    union {
        #if ARM
            svint32_t s;
        #else
            __m256i s;
        #endif

        int32 v[8];
    };
};

inline int32_8 load_int32_8(const int32* mem)
{
    int32_8 simd;
    simd.s = _mm256_load_si256((__m256i *) mem);

    return simd;
}

inline int32_8 init_int32_8(const int32* mem)
{
    int32_8 simd;
    simd.s = _mm256_set_epi32(mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7]);

    return simd;
}

inline void unload_int32_8(int32_8 a, int32 *array) { _mm256_store_si256((__m256i *) array, a.s); }

inline int32_8 init_zero_int32_8()
{
    int32_8 simd;
    simd.s = _mm256_setzero_si256();

    return simd;
}

inline int32_8 init_value_int32_8(int32 value)
{
    int32_8 simd;
    simd.s = _mm256_set1_epi32(value);

    return simd;
}

inline int32_8 init_values_int32_8(
    int32 a, int32 b, int32 c, int32 d,
    int32 e, int32 f, int32 g, int32 h
)
{
    int32_8 simd;
    simd.s = _mm256_set_epi32(a, b, c, d, e, f, g, h);

    return simd;
}

inline int32_8 operator+(int32_8 a, int32_8 b)
{
    int32_8 simd;
    simd.s = _mm256_add_epi32(a.s, b.s);

    return simd;
}

inline int32_8 operator-(int32_8 a, int32_8 b)
{
    int32_8 simd;
    simd.s = _mm256_sub_epi32(a.s, b.s);

    return simd;
}

inline int32_8 operator-(int32_8 a) { return init_zero_int32_8() - a; }

inline int32_8 operator*(int32_8 a, int32_8 b)
{
    int32_8 simd;
    simd.s = _mm256_mul_epi32(a.s, b.s);

    return simd;
}

inline int32_8 operator^(int32_8 a, int32_8 b)
{
    int32_8 simd;
    simd.s = _mm256_xor_epi32(a.s, b.s);

    return simd;
}

inline int32_8 &operator-=(int32_8 &a, int32_8 b)
{
    a = a - b;

    return a;
}

inline int32_8 &operator+=(int32_8 &a, int32_8 b)
{
    a = a + b;

    return a;
}

inline int32_8 &operator*=(int32_8 &a, int32_8 b)
{
    a = a * b;

    return a;
}

inline int32_8 &operator^=(int32_8 &a, int32_8 b)
{
    a = a ^ b;

    return a;
}

inline int32_8 operator<(int32_8 a, int32_8 b)
{
    int32_8 simd;
    simd.s = _mm256_xor_si256(_mm256_cmpgt_epi32(a.s, b.s), _mm256_set1_epi32(-1));

    return simd;
}

inline int32_8 operator<=(int32_8 a, int32_8 b)
{
    int32_8 simd;
    simd.s = _mm256_andnot_si256(_mm256_cmpgt_epi32(a.s, b.s), _mm256_set1_epi32(-1));

    return simd;
}

inline int32_8 operator>(int32_8 a, int32_8 b)
{
    int32_8 simd;
    simd.s = _mm256_cmpgt_epi32(a.s, b.s);

    return simd;
}

inline int32_8 operator>=(int32_8 a, int32_8 b)
{
    int32_8 simd;
    simd.s = _mm256_andnot_si256(_mm256_cmpgt_epi32(b.s, a.s), _mm256_set1_epi32(-1));

    return simd;
}

inline int32_8 operator==(int32_8 a, int32_8 b)
{
    int32_8 simd;
    simd.s = _mm256_cmpeq_epi32(a.s, b.s);

    return simd;
}

inline int32_8 operator!=(int32_8 a, int32_8 b)
{
    int32_8 simd;
    simd.s = _mm256_mask_blend_epi32(_mm256_cmp_epi32_mask(a.s, b.s, _MM_CMPINT_NE), a.s, b.s);

    return simd;
}

inline int32_8 operator&(int32_8 a, int32_8 b)
{
    int32_8 simd;
    simd.s = _mm256_and_si256(a.s, b.s);

    return simd;
}

inline int32_8 operator|(int32_8 a, int32_8 b)
{
    int32_8 simd;
    simd.s = _mm256_or_epi32(a.s, b.s);

    return simd;
}

inline int32_8 &operator&=(int32_8 &a, int32_8 b)
{
    a = a & b;

    return a;
}

inline int32_8 &operator|=(int32_8 &a, int32_8 b)
{
    a = a | b;

    return a;
}

inline int32_8 abs(int32_8 a)
{
    int32_8 simd;
    simd.s = _mm256_abs_epi32(a.s);

    return simd;
}

inline int32_8 simd_min(int32_8 a, int32_8 b)
{
    int32_8 simd;
    simd.s = _mm256_min_epi32(a.s, b.s);

    return simd;
}

inline int32_8 simd_max(int32_8 a, int32_8 b)
{
    int32_8 simd;
    simd.s = _mm256_max_epi32(a.s, b.s);

    return simd;
}

inline int32_8 sign(int32_8 a)
{
    __m256i mask = _mm256_set1_epi32(0x80000000);
    __m256i signBit = _mm256_and_si256(a.s, mask);
    __m256i b = _mm256_set1_epi32(1);

    int32_8 simd;
    simd.s = _mm256_or_si256(b, signBit);

    return simd;
}

inline int32_8 clamp(int32_8 min_value, int32_8 a, int32_8 max_value)
{
    return simd_min(simd_max(a, min_value), max_value);
}

inline int32 which_true(int32_8 a)
{
    int32 which_true = _mm256_movemask_epi8(a.s);

    return which_true;
}

inline bool any_true(int32_8 a)
{
    bool is_any_true = _mm256_movemask_epi8(a.s) > 0;

    return is_any_true;
}

inline bool all_true(int32_8 a)
{
    bool is_true = _mm256_movemask_epi8(a.s) == 255;

    return is_true;
}

inline bool all_false(int32_8 a)
{
    bool is_false = _mm256_movemask_epi8(a.s) == 0;

    return is_false;
}

#endif

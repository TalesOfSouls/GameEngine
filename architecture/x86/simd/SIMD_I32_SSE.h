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

struct int32_4 {
    union {
        #if ARM
            svint32_t s;
        #else
            __m128i s;
        #endif

        int32 v[4];
    };
};

inline int32_4 load_int32_4(const int32* mem)
{
    int32_4 simd;
    simd.s = _mm_load_si128((__m128i *) mem);

    return simd;
}

inline int32_4 init_int32_4(const int32* mem)
{
    int32_4 simd;
    simd.s = _mm_set_epi32(mem[0], mem[1], mem[2], mem[3]);

    return simd;
}

inline void unload_int32_4(int32_4 a, int32 *array) { _mm_store_si128((__m128i *) array, a.s); }

inline int32_4 init_zero_int32_4()
{
    int32_4 simd;
    simd.s = _mm_setzero_si128();

    return simd;
}

inline int32_4 init_value_int32_4(int32 value)
{
    int32_4 simd;
    simd.s = _mm_set1_epi32(value);

    return simd;
}

inline int32_4 init_values_int32_4(int32 a, int32 b, int32 c, int32 d)
{
    int32_4 simd;
    simd.s = _mm_set_epi32(a, b, c, d);

    return simd;
}

inline int32_4 operator+(int32_4 a, int32_4 b)
{
    int32_4 simd;
    simd.s = _mm_add_epi32(a.s, b.s);

    return simd;
}

inline int32_4 operator-(int32_4 a, int32_4 b)
{
    int32_4 simd;
    simd.s = _mm_sub_epi32(a.s, b.s);

    return simd;
}

inline int32_4 operator-(int32_4 a) { return init_zero_int32_4() - a; }

inline int32_4 operator*(int32_4 a, int32_4 b)
{
    int32_4 simd;
    simd.s = _mm_mul_epi32(a.s, b.s);

    return simd;
}

inline int32_4 operator^(int32_4 a, int32_4 b)
{
    int32_4 simd;
    simd.s = _mm_xor_epi32(a.s, b.s);

    return simd;
}

inline int32_4 &operator-=(int32_4 &a, int32_4 b)
{
    a = a - b;

    return a;
}

inline int32_4 &operator+=(int32_4 &a, int32_4 b)
{
    a = a + b;

    return a;
}

inline int32_4 &operator*=(int32_4 &a, int32_4 b)
{
    a = a * b;

    return a;
}

inline int32_4 &operator^=(int32_4 &a, int32_4 b)
{
    a = a ^ b;

    return a;
}

inline int32_4 operator<(int32_4 a, int32_4 b)
{
    int32_4 simd;
    simd.s = _mm_cmplt_epi32(a.s, b.s);

    return simd;
}

inline int32_4 operator<=(int32_4 a, int32_4 b)
{
    int32_4 simd;
    simd.s = _mm_andnot_si128(_mm_cmplt_epi32(b.s, a.s), _mm_set1_epi32(-1));

    return simd;
}

inline int32_4 operator>(int32_4 a, int32_4 b)
{
    int32_4 simd;
    simd.s = _mm_cmpgt_epi32(a.s, b.s);

    return simd;
}

inline int32_4 operator>=(int32_4 a, int32_4 b)
{
    int32_4 simd;
    simd.s = _mm_andnot_si128(_mm_cmplt_epi32(a.s, b.s), _mm_set1_epi32(-1));

    return simd;
}

inline int32_4 operator==(int32_4 a, int32_4 b)
{
    int32_4 simd;
    simd.s = _mm_cmpeq_epi32(a.s, b.s);

    return simd;
}

inline int32_4 operator!=(int32_4 a, int32_4 b)
{
    int32_4 simd;
    simd.s = _mm_andnot_si128(_mm_cmpeq_epi32(a.s, b.s), _mm_set1_epi32(-1));

    return simd;
}

inline int32_4 operator&(int32_4 a, int32_4 b)
{
    int32_4 simd;
    simd.s = _mm_and_si128(a.s, b.s);

    return simd;
}

inline int32_4 operator|(int32_4 a, int32_4 b)
{
    int32_4 simd;
    simd.s = _mm_or_epi32(a.s, b.s);

    return simd;
}

inline int32_4 &operator&=(int32_4 &a, int32_4 b)
{
    a = a & b;

    return a;
}

inline int32_4 &operator|=(int32_4 &a, int32_4 b)
{
    a = a | b;

    return a;
}

inline int32_4 abs(int32_4 a)
{
    int32_4 simd;
    simd.s = _mm_abs_epi32(a.s);

    return simd;
}

inline int32_4 simd_min(int32_4 a, int32_4 b)
{
    int32_4 simd;
    simd.s = _mm_min_epi32(a.s, b.s);

    return simd;
}

inline int32_4 simd_max(int32_4 a, int32_4 b)
{
    int32_4 simd;
    simd.s = _mm_max_epi32(a.s, b.s);

    return simd;
}

inline int32_4 sign(int32_4 a)
{
    __m128i mask = _mm_set1_epi32(0x80000000);
    __m128i signBit = _mm_and_si128(a.s, mask);
    __m128i b = _mm_set1_epi32(1);

    int32_4 simd;
    simd.s = _mm_or_si128(b, signBit);

    return simd;
}

inline int32_4 clamp(int32_4 min_value, int32_4 a, int32_4 max_value)
{
    return simd_min(simd_max(a, min_value), max_value);
}

inline int32 which_true(int32_4 a)
{
    int32 which_true = _mm_movemask_epi8(a.s);

    return which_true;
}

inline bool any_true(int32_4 a)
{
    bool is_any_true = _mm_movemask_epi8(a.s) > 0;

    return is_any_true;
}

inline bool all_true(int32_4 a)
{
    bool is_true = _mm_movemask_epi8(a.s) == 15;

    return is_true;
}

inline bool all_false(int32_4 a)
{
    bool is_false = _mm_movemask_epi8(a.s) == 0;

    return is_false;
}

#endif

/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_I16_SSE_H
#define COMS_STDLIB_SIMD_I16_SSE_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Types.h"

struct int16_8 {
    union {
        #if ARM
            svint16_t s;
        #else
            __m128i s;
        #endif

        int16 v[8];
    };
};

inline int16_8 load_int16_8(const int16* mem)
{
    int16_8 simd;
    simd.s = _mm_load_si128((__m128i *) mem);

    return simd;
}

inline int16_8 init_int16_8(const int16* mem)
{
    int16_8 simd;
    simd.s = _mm_set_epi16(
        mem[0], mem[1], mem[2], mem[3],
        mem[4], mem[5], mem[6], mem[7]
    );

    return simd;
}

inline void unload_int16_8(int16_8 a, int16 *array) { _mm_store_si128((__m128i *) array, a.s); }

inline int16_8 init_zero_int16_8()
{
    int16_8 simd;
    simd.s = _mm_setzero_si128();

    return simd;
}

inline int16_8 init_value_int16_8(int16 value)
{
    int16_8 simd;
    simd.s = _mm_set1_epi16(value);

    return simd;
}

inline int16_8 operator+(int16_8 a, int16_8 b)
{
    int16_8 simd;
    simd.s = _mm_add_epi16(a.s, b.s);

    return simd;
}

inline int16_8 operator-(int16_8 a, int16_8 b)
{
    int16_8 simd;
    simd.s = _mm_sub_epi16(a.s, b.s);

    return simd;
}

inline int16_8 operator-(int16_8 a) { return init_zero_int16_8() - a; }

inline int16_8 operator*(int16_8 a, int16_8 b)
{
    int16_8 simd;
    simd.s = _mm_mul_epi32(a.s, b.s);

    return simd;
}

inline int16_8 operator^(int16_8 a, int16_8 b)
{
    int16_8 simd;
    simd.s = _mm_xor_si128(a.s, b.s);

    return simd;
}

inline int16_8 &operator-=(int16_8 &a, int16_8 b)
{
    a = a - b;

    return a;
}

inline int16_8 &operator+=(int16_8 &a, int16_8 b)
{
    a = a + b;

    return a;
}

inline int16_8 &operator*=(int16_8 &a, int16_8 b)
{
    a = a * b;

    return a;
}

inline int16_8 &operator^=(int16_8 &a, int16_8 b)
{
    a = a ^ b;

    return a;
}

inline int16_8 operator<(int16_8 a, int16_8 b)
{
    int16_8 simd;
    simd.s = _mm_cmplt_epi16(a.s, b.s);

    return simd;
}

inline int16_8 operator<=(int16_8 a, int16_8 b)
{
    int16_8 simd;
    simd.s = _mm_andnot_si128(_mm_cmplt_epi16(b.s, a.s), _mm_set1_epi16(-1));

    return simd;
}

inline int16_8 operator>(int16_8 a, int16_8 b)
{
    int16_8 simd;
    simd.s = _mm_cmpgt_epi16(a.s, b.s);

    return simd;
}

inline int16_8 operator>=(int16_8 a, int16_8 b)
{
    int16_8 simd;
    simd.s = _mm_andnot_si128(_mm_cmplt_epi16(a.s, b.s), _mm_set1_epi16(-1));

    return simd;
}

inline int16_8 operator==(int16_8 a, int16_8 b)
{
    int16_8 simd;
    simd.s = _mm_cmpeq_epi16(a.s, b.s);

    return simd;
}

inline int16_8 operator!=(int16_8 a, int16_8 b)
{
    int16_8 simd;
    simd.s = _mm_andnot_si128(_mm_cmpeq_epi16(a.s, b.s), _mm_set1_epi16(-1));

    return simd;
}

inline int16_8 operator&(int16_8 a, int16_8 b)
{
    int16_8 simd;
    simd.s = _mm_and_si128(a.s, b.s);

    return simd;
}

inline int16_8 operator|(int16_8 a, int16_8 b)
{
    int16_8 simd;
    simd.s = _mm_or_si128(a.s, b.s);

    return simd;
}

inline int16_8 &operator&=(int16_8 &a, int16_8 b)
{
    a = a & b;

    return a;
}

inline int16_8 &operator|=(int16_8 &a, int16_8 b)
{
    a = a | b;

    return a;
}

inline int16_8 abs(int16_8 a)
{
    int16_8 simd;
    simd.s = _mm_abs_epi16(a.s);

    return simd;
}

inline int16_8 simd_min(int16_8 a, int16_8 b)
{
    int16_8 simd;
    simd.s = _mm_min_epi16(a.s, b.s);

    return simd;
}

inline int16_8 simd_max(int16_8 a, int16_8 b)
{
    int16_8 simd;
    simd.s = _mm_max_epi16(a.s, b.s);

    return simd;
}

inline int16_8 clamp(int16_8 min_value, int16_8 a, int16_8 max_value)
{
    return simd_min(simd_max(a, min_value), max_value);
}

inline int32 which_true(int16_8 a)
{
    return _mm_movemask_epi8(a.s);
}

inline bool any_true(int16_8 a)
{
    bool is_any_true = _mm_movemask_epi8(a.s) > 0;

    return is_any_true;
}

inline bool all_true(int16_8 a)
{
    bool is_true = _mm_movemask_epi8(a.s) == 15;

    return is_true;
}

inline bool all_false(int16_8 a)
{
    bool is_false = _mm_movemask_epi8(a.s) == 0;

    return is_false;
}

#endif
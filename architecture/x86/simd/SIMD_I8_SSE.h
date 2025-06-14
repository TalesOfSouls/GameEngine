/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_I8_H
#define COMS_STDLIB_SIMD_I8_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Types.h"

struct int8_16 {
    union {
        #if ARM
            svint8_t s;
        #else
            __m128i s;
        #endif

        int8 v[16];
    };
};

inline int8_16 load_int8_16(const int8* mem)
{
    int8_16 simd;
    simd.s = _mm_load_si128((__m128i *) mem);

    return simd;
}

inline int8_16 init_int8_16(const int8* mem)
{
    int8_16 simd;
    simd.s = _mm_set_epi8(
        mem[0], mem[1], mem[2], mem[3],
        mem[4], mem[5], mem[6], mem[7],
        mem[8], mem[9], mem[10], mem[11],
        mem[12], mem[13], mem[14], mem[15]
    );

    return simd;
}

inline void unload_int8_16(int8_16 a, int8 *array) { _mm_store_si128((__m128i *) array, a.s); }

inline int8_16 init_zero_int8_16()
{
    int8_16 simd;
    simd.s = _mm_setzero_si128();

    return simd;
}

inline int8_16 init_value_int8_16(int8 value)
{
    int8_16 simd;
    simd.s = _mm_set1_epi8(value);

    return simd;
}

inline int8_16 operator+(int8_16 a, int8_16 b)
{
    int8_16 simd;
    simd.s = _mm_add_epi8(a.s, b.s);

    return simd;
}

inline int8_16 operator-(int8_16 a, int8_16 b)
{
    int8_16 simd;
    simd.s = _mm_sub_epi8(a.s, b.s);

    return simd;
}

inline int8_16 operator-(int8_16 a) { return init_zero_int8_16() - a; }

inline int8_16 operator*(int8_16 a, int8_16 b)
{
    int8_16 simd;
    simd.s = _mm_mul_epi32(a.s, b.s);

    return simd;
}

inline int8_16 operator^(int8_16 a, int8_16 b)
{
    int8_16 simd;
    simd.s = _mm_xor_si128(a.s, b.s);

    return simd;
}

inline int8_16 &operator-=(int8_16 &a, int8_16 b)
{
    a = a - b;

    return a;
}

inline int8_16 &operator+=(int8_16 &a, int8_16 b)
{
    a = a + b;

    return a;
}

inline int8_16 &operator*=(int8_16 &a, int8_16 b)
{
    a = a * b;

    return a;
}

inline int8_16 &operator^=(int8_16 &a, int8_16 b)
{
    a = a ^ b;

    return a;
}

inline int8_16 operator<(int8_16 a, int8_16 b)
{
    int8_16 simd;
    simd.s = _mm_cmplt_epi8(a.s, b.s);

    return simd;
}

inline int8_16 operator<=(int8_16 a, int8_16 b)
{
    int8_16 simd;
    simd.s = _mm_andnot_si128(_mm_cmplt_epi8(b.s, a.s), _mm_set1_epi8(-1));

    return simd;
}

inline int8_16 operator>(int8_16 a, int8_16 b)
{
    int8_16 simd;
    simd.s = _mm_cmpgt_epi8(a.s, b.s);

    return simd;
}

inline int8_16 operator>=(int8_16 a, int8_16 b)
{
    int8_16 simd;
    simd.s = _mm_andnot_si128(_mm_cmplt_epi8(a.s, b.s), _mm_set1_epi8(-1));

    return simd;
}

inline int8_16 operator==(int8_16 a, int8_16 b)
{
    int8_16 simd;
    simd.s = _mm_cmpeq_epi8(a.s, b.s);

    return simd;
}

inline int8_16 operator!=(int8_16 a, int8_16 b)
{
    int8_16 simd;
    simd.s = _mm_andnot_si128(_mm_cmpeq_epi8(a.s, b.s), _mm_set1_epi8(-1));

    return simd;
}

inline int8_16 operator&(int8_16 a, int8_16 b)
{
    int8_16 simd;
    simd.s = _mm_and_si128(a.s, b.s);

    return simd;
}

inline int8_16 operator|(int8_16 a, int8_16 b)
{
    int8_16 simd;
    simd.s = _mm_or_si128(a.s, b.s);

    return simd;
}

inline int8_16 &operator&=(int8_16 &a, int8_16 b)
{
    a = a & b;

    return a;
}

inline int8_16 &operator|=(int8_16 &a, int8_16 b)
{
    a = a | b;

    return a;
}

inline int8_16 abs(int8_16 a)
{
    int8_16 simd;
    simd.s = _mm_abs_epi8(a.s);

    return simd;
}

inline int8_16 simd_min(int8_16 a, int8_16 b)
{
    int8_16 simd;
    simd.s = _mm_min_epi8(a.s, b.s);

    return simd;
}

inline int8_16 simd_max(int8_16 a, int8_16 b)
{
    int8_16 simd;
    simd.s = _mm_max_epi8(a.s, b.s);

    return simd;
}

inline int8_16 clamp(int8_16 min_value, int8_16 a, int8_16 max_value)
{
    return simd_min(simd_max(a, min_value), max_value);
}

inline int32 which_true(int8_16 a)
{
    int32 which_true = _mm_movemask_epi8(a.s);

    return which_true;
}

inline bool any_true(int8_16 a)
{
    bool is_any_true = _mm_movemask_epi8(a.s) > 0;

    return is_any_true;
}

inline bool all_true(int8_16 a)
{
    bool is_true = _mm_movemask_epi8(a.s) == 15;

    return is_true;
}

inline bool all_false(int8_16 a)
{
    bool is_false = _mm_movemask_epi8(a.s) == 0;

    return is_false;
}

#endif
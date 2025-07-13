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

struct int8_32 {
    union {
        #if ARM
            svint8_t s;
        #else
            __m256i s;
        #endif

        int8 v[32];
    };
};

inline int8_32 load_int8_32(const int8* mem)
{
    int8_32 simd;
    simd.s = _mm256_load_si256((__m256i *) mem);

    return simd;
}

inline int8_32 init_int8_32(const int8* mem)
{
    int8_32 simd;
    simd.s = _mm256_set_epi8(
        mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7],
        mem[8], mem[9], mem[10], mem[11], mem[12], mem[13], mem[14], mem[15],
        mem[16], mem[17], mem[18], mem[19], mem[20], mem[21], mem[22], mem[23],
        mem[24], mem[25], mem[26], mem[27], mem[28], mem[29], mem[30], mem[31]
    );

    return simd;
}

inline void unload_int8_32(int8_32 a, int8 *array) { _mm256_store_si256((__m256i *) array, a.s); }

inline int8_32 init_zero_int8_32()
{
    int8_32 simd;
    simd.s = _mm256_setzero_si256();

    return simd;
}

inline int8_32 init_value_int8_32(int8 value)
{
    int8_32 simd;
    simd.s = _mm256_set1_epi8(value);

    return simd;
}

inline int8_32 operator+(int8_32 a, int8_32 b)
{
    int8_32 simd;
    simd.s = _mm256_add_epi8(a.s, b.s);

    return simd;
}

inline int8_32 operator-(int8_32 a, int8_32 b)
{
    int8_32 simd;
    simd.s = _mm256_sub_epi8(a.s, b.s);

    return simd;
}

inline int8_32 operator-(int8_32 a) { return init_zero_int8_32() - a; }

inline int8_32 operator*(int8_32 a, int8_32 b)
{
    int8_32 simd;
    simd.s = _mm256_mul_epi32(a.s, b.s);

    return simd;
}

inline int8_32 operator^(int8_32 a, int8_32 b)
{
    int8_32 simd;
    simd.s = _mm256_xor_si256(a.s, b.s);

    return simd;
}

inline int8_32 &operator-=(int8_32 &a, int8_32 b)
{
    a = a - b;

    return a;
}

inline int8_32 &operator+=(int8_32 &a, int8_32 b)
{
    a = a + b;

    return a;
}

inline int8_32 &operator*=(int8_32 &a, int8_32 b)
{
    a = a * b;

    return a;
}

inline int8_32 &operator^=(int8_32 &a, int8_32 b)
{
    a = a ^ b;

    return a;
}

inline int8_32 operator<(int8_32 a, int8_32 b)
{
    int8_32 simd;
    simd.s = _mm256_xor_si256(_mm256_cmpgt_epi8(a.s, b.s), _mm256_set1_epi8(-1));

    return simd;
}

inline int8_32 operator<=(int8_32 a, int8_32 b)
{
    int8_32 simd;
    simd.s = _mm256_andnot_si256(_mm256_cmpgt_epi8(a.s, b.s), _mm256_set1_epi8(-1));

    return simd;
}

inline int8_32 operator>(int8_32 a, int8_32 b)
{
    int8_32 simd;
    simd.s = _mm256_cmpgt_epi8(a.s, b.s);

    return simd;
}

inline int8_32 operator>=(int8_32 a, int8_32 b)
{
    int8_32 simd;
    simd.s = _mm256_andnot_si256(_mm256_cmpgt_epi8(b.s, a.s), _mm256_set1_epi8(-1));

    return simd;
}

inline int8_32 operator==(int8_32 a, int8_32 b)
{
    int8_32 simd;
    simd.s = _mm256_cmpeq_epi8(a.s, b.s);

    return simd;
}

inline int8_32 operator!=(int8_32 a, int8_32 b)
{
    int8_32 simd;
    simd.s = _mm256_mask_blend_epi8(_mm256_cmp_epi8_mask(a.s, b.s, _MM_CMPINT_NE), a.s, b.s);

    return simd;
}

inline int8_32 operator&(int8_32 a, int8_32 b)
{
    int8_32 simd;
    simd.s = _mm256_and_si256(a.s, b.s);

    return simd;
}

inline int8_32 operator|(int8_32 a, int8_32 b)
{
    int8_32 simd;
    simd.s = _mm256_or_si256(a.s, b.s);

    return simd;
}

inline int8_32 &operator&=(int8_32 &a, int8_32 b)
{
    a = a & b;

    return a;
}

inline int8_32 &operator|=(int8_32 &a, int8_32 b)
{
    a = a | b;

    return a;
}

inline int8_32 abs(int8_32 a)
{
    int8_32 simd;
    simd.s = _mm256_abs_epi16(a.s);

    return simd;
}

inline int8_32 simd_min(int8_32 a, int8_32 b)
{
    int8_32 simd;
    simd.s = _mm256_min_epi8(a.s, b.s);

    return simd;
}

inline int8_32 simd_max(int8_32 a, int8_32 b)
{
    int8_32 simd;
    simd.s = _mm256_max_epi8(a.s, b.s);

    return simd;
}

inline int8_32 clamp(int8_32 min_value, int8_32 a, int8_32 max_value)
{
    return simd_min(simd_max(a, min_value), max_value);
}

inline int32 which_true(int8_32 a)
{
    int32 which_true = _mm256_movemask_epi8(a.s);

    return which_true;
}

inline bool any_true(int8_32 a)
{
    bool is_any_true = _mm256_movemask_epi8(a.s) > 0;

    return is_any_true;
}

inline bool all_true(int8_32 a)
{
    bool is_true = _mm256_movemask_epi8(a.s) == 255;

    return is_true;
}

inline bool all_false(int8_32 a)
{
    bool is_false = _mm256_movemask_epi8(a.s) == 0;

    return is_false;
}

#endif
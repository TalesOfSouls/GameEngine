/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_I16_AVX2_H
#define COMS_STDLIB_SIMD_I16_AVX2_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Types.h"

struct int16_16 {
    union {
        #if ARM
            svint16_t s;
        #else
            __m256i s;
        #endif

        int16 v[16];
    };
};

inline int16_16 load_int16_16(const int16* mem)
{
    int16_16 simd;
    simd.s = _mm256_load_si256((__m256i *) mem);

    return simd;
}

inline int16_16 init_int16_16(const int16* mem)
{
    int16_16 simd;
    simd.s = _mm256_set_epi16(
        mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7],
        mem[8], mem[9], mem[10], mem[11], mem[12], mem[13], mem[14], mem[15]
    );

    return simd;
}

inline void unload_int16_16(int16_16 a, int16 *array) { _mm256_store_si256((__m256i *) array, a.s); }

inline int16_16 init_zero_int16_16()
{
    int16_16 simd;
    simd.s = _mm256_setzero_si256();

    return simd;
}

inline int16_16 init_value_int16_16(int16 value)
{
    int16_16 simd;
    simd.s = _mm256_set1_epi16(value);

    return simd;
}

inline int16_16 operator+(int16_16 a, int16_16 b)
{
    int16_16 simd;
    simd.s = _mm256_add_epi16(a.s, b.s);

    return simd;
}

inline int16_16 operator-(int16_16 a, int16_16 b)
{
    int16_16 simd;
    simd.s = _mm256_sub_epi16(a.s, b.s);

    return simd;
}

inline int16_16 operator-(int16_16 a) { return init_zero_int16_16() - a; }

inline int16_16 operator*(int16_16 a, int16_16 b)
{
    int16_16 simd;
    simd.s = _mm256_mul_epi32(a.s, b.s);

    return simd;
}

inline int16_16 operator^(int16_16 a, int16_16 b)
{
    int16_16 simd;
    simd.s = _mm256_xor_si256(a.s, b.s);

    return simd;
}

inline int16_16 &operator-=(int16_16 &a, int16_16 b)
{
    a = a - b;

    return a;
}

inline int16_16 &operator+=(int16_16 &a, int16_16 b)
{
    a = a + b;

    return a;
}

inline int16_16 &operator*=(int16_16 &a, int16_16 b)
{
    a = a * b;

    return a;
}

inline int16_16 &operator^=(int16_16 &a, int16_16 b)
{
    a = a ^ b;

    return a;
}

inline int16_16 operator<(int16_16 a, int16_16 b)
{
    int16_16 simd;
    simd.s = _mm256_xor_si256(_mm256_cmpgt_epi16(a.s, b.s), _mm256_set1_epi16(-1));

    return simd;
}

inline int16_16 operator<=(int16_16 a, int16_16 b)
{
    int16_16 simd;
    simd.s = _mm256_andnot_si256(_mm256_cmpgt_epi16(a.s, b.s), _mm256_set1_epi16(-1));

    return simd;
}

inline int16_16 operator>(int16_16 a, int16_16 b)
{
    int16_16 simd;
    simd.s = _mm256_cmpgt_epi16(a.s, b.s);

    return simd;
}

inline int16_16 operator>=(int16_16 a, int16_16 b)
{
    int16_16 simd;
    simd.s = _mm256_andnot_si256(_mm256_cmpgt_epi16(b.s, a.s), _mm256_set1_epi16(-1));

    return simd;
}

inline int16_16 operator==(int16_16 a, int16_16 b)
{
    int16_16 simd;
    simd.s = _mm256_cmpeq_epi16(a.s, b.s);

    return simd;
}

inline int16_16 operator!=(int16_16 a, int16_16 b)
{
    int16_16 simd;
    simd.s = _mm256_mask_blend_epi16(_mm256_cmp_epi16_mask(a.s, b.s, _MM_CMPINT_NE), a.s, b.s);

    return simd;
}


inline int16_16 operator&(int16_16 a, int16_16 b)
{
    int16_16 simd;
    simd.s = _mm256_and_si256(a.s, b.s);

    return simd;
}

inline int16_16 operator|(int16_16 a, int16_16 b)
{
    int16_16 simd;
    simd.s = _mm256_or_si256(a.s, b.s);

    return simd;
}

inline int16_16 &operator&=(int16_16 &a, int16_16 b)
{
    a = a & b;

    return a;
}

inline int16_16 &operator|=(int16_16 &a, int16_16 b)
{
    a = a | b;

    return a;
}

inline int16_16 abs(int16_16 a)
{
    int16_16 simd;
    simd.s = _mm256_abs_epi16(a.s);

    return simd;
}

inline int16_16 simd_min(int16_16 a, int16_16 b)
{
    int16_16 simd;
    simd.s = _mm256_min_epi16(a.s, b.s);

    return simd;
}

inline int16_16 simd_max(int16_16 a, int16_16 b)
{
    int16_16 simd;
    simd.s = _mm256_max_epi16(a.s, b.s);

    return simd;
}

inline int16_16 clamp(int16_16 min_value, int16_16 a, int16_16 max_value)
{
    return simd_min(simd_max(a, min_value), max_value);
}

inline int32 which_true(int16_16 a)
{
    return _mm256_movemask_epi8(a.s);
}

inline bool any_true(int16_16 a)
{
    bool is_any_true = _mm256_movemask_epi8(a.s) > 0;

    return is_any_true;
}

inline bool all_true(int16_16 a)
{
    bool is_true = _mm256_movemask_epi8(a.s) == 255;

    return is_true;
}

inline bool all_false(int16_16 a)
{
    bool is_false = _mm256_movemask_epi8(a.s) == 0;

    return is_false;
}

#endif
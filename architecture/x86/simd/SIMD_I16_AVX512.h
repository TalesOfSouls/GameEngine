/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_I16_AVX512_H
#define COMS_STDLIB_SIMD_I16_AVX512_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Types.h"

struct int16_32 {
    union {
        #if ARM
            svint16_t s;
        #else
            __m512i s;
        #endif

        int16 v[32];
    };
};

inline int16_32 load_int16_32(const int16* mem)
{
    int16_32 simd;
    simd.s = _mm512_load_si512((__m512i *) mem);

    return simd;
}

inline int16_32 init_int16_32(const int16* mem)
{
    int16_32 simd;
    simd.s = _mm512_set_epi16(
        mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7],
        mem[8], mem[9], mem[10], mem[11], mem[12], mem[13], mem[14], mem[15],
        mem[16], mem[17], mem[18], mem[19], mem[20], mem[21], mem[22], mem[23],
        mem[24], mem[25], mem[26], mem[27], mem[28], mem[29], mem[30], mem[31]
    );

    return simd;
}

inline void unload_int16_32(int16_32 a, int16 *array) { _mm512_storeu_epi16(array, a.s); }

inline int16_32 init_zero_int16_32()
{
    int16_32 simd;
    simd.s = _mm512_setzero_si512();

    return simd;
}

inline int16_32 init_value_int16_32(int16 value)
{
    int16_32 simd;
    simd.s = _mm512_set1_epi16(value);

    return simd;
}

inline int16_32 operator+(int16_32 a, int16_32 b)
{
    int16_32 simd;
    simd.s = _mm512_add_epi16(a.s, b.s);

    return simd;
}

inline int16_32 operator-(int16_32 a, int16_32 b)
{
    int16_32 simd;
    simd.s = _mm512_sub_epi16(a.s, b.s);

    return simd;
}

inline int16_32 operator-(int16_32 a) { return init_zero_int16_32() - a; }

inline int16_32 operator*(int16_32 a, int16_32 b)
{
    int16_32 simd;
    simd.s = _mm512_mul_epi32(a.s, b.s);

    return simd;
}

inline int16_32 operator^(int16_32 a, int16_32 b)
{
    int16_32 simd;
    simd.s = _mm512_xor_si512(a.s, b.s);

    return simd;
}

inline int16_32 &operator-=(int16_32 &a, int16_32 b)
{
    a = a - b;

    return a;
}

inline int16_32 &operator+=(int16_32 &a, int16_32 b)
{
    a = a + b;

    return a;
}

inline int16_32 &operator*=(int16_32 &a, int16_32 b)
{
    a = a * b;

    return a;
}

inline int16_32 &operator^=(int16_32 &a, int16_32 b)
{
    a = a ^ b;

    return a;
}

inline int16_32 operator<(int16_32 a, int16_32 b)
{
    int16_32 simd;
    simd.s = _mm512_mask_blend_epi16(_mm512_cmplt_epi16_mask(a.s, b.s), a.s, b.s);

    return simd;
}

inline int16_32 operator<=(int16_32 a, int16_32 b)
{
    int16_32 simd;
    __mmask32 mask = _mm512_cmp_epi16_mask(a.s, b.s, _MM_CMPINT_LE);
    simd.s = _mm512_mask_blend_epi16(mask, b.s, a.s);

    return simd;
}

inline int16_32 operator>(int16_32 a, int16_32 b)
{
    int16_32 simd;
    simd.s = _mm512_mask_blend_epi16(_mm512_cmpgt_epi16_mask(a.s, b.s), a.s, b.s);

    return simd;
}

inline int16_32 operator>=(int16_32 a, int16_32 b)
{
    int16_32 simd;
    simd.s = _mm512_mask_blend_epi16(_mm512_cmpge_epi16_mask(a.s, b.s), a.s, b.s);

    return simd;
}

inline int16_32 operator==(int16_32 a, int16_32 b)
{
    int16_32 simd;
    simd.s = _mm512_mask_blend_epi16(_mm512_cmpeq_epi16_mask(a.s, b.s), a.s, b.s);

    return simd;
}

inline int16_32 operator!=(int16_32 a, int16_32 b)
{
    int16_32 simd;
    simd.s = _mm512_mask_blend_epi16(_mm512_cmp_epi16_mask(a.s, b.s, _MM_CMPINT_NE), a.s, b.s);

    return simd;
}

inline int16_32 operator&(int16_32 a, int16_32 b)
{
    int16_32 simd;
    simd.s = _mm512_and_si512(a.s, b.s);

    return simd;
}

inline int16_32 operator|(int16_32 a, int16_32 b)
{
    int16_32 simd;
    simd.s = _mm512_or_si512(a.s, b.s);

    return simd;
}

inline int16_32 &operator&=(int16_32 &a, int16_32 b)
{
    a = a & b;

    return a;
}

inline int16_32 &operator|=(int16_32 &a, int16_32 b)
{
    a = a | b;

    return a;
}

inline int16_32 abs(int16_32 a)
{
    int16_32 simd;
    simd.s = _mm512_abs_epi16(a.s);

    return simd;
}

inline int16_32 simd_min(int16_32 a, int16_32 b)
{
    int16_32 simd;
    simd.s = _mm512_min_epi16(a.s, b.s);

    return simd;
}

inline int16_32 simd_max(int16_32 a, int16_32 b)
{
    int16_32 simd;
    simd.s = _mm512_max_epi16(a.s, b.s);

    return simd;
}

inline int16_32 clamp(int16_32 min_value, int16_32 a, int16_32 max_value)
{
    return simd_min(simd_max(a, min_value), max_value);
}

inline int32 which_true(int16_32 a)
{
    return _mm512_movepi16_mask(a.s);
}

inline bool any_true(int16_32 a)
{
    bool is_any_true = _mm512_movepi16_mask(a.s) > 0;

    return is_any_true;
}

inline bool all_true(int16_32 a)
{
    bool is_true = _mm512_movepi16_mask(a.s) == 65535;

    return is_true;
}

inline bool all_false(int16_32 a)
{
    // @todo This can be optimized (requires also changes in the comparison functions return)
    bool is_false = _mm512_movepi16_mask(a.s) == 0;

    return is_false;
}

#endif
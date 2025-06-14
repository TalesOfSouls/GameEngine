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

struct int8_64 {
    union {
        #if ARM
            svint8_t s;
        #else
            __m512i s;
        #endif

        int8 v[64];
    };
};

inline int8_64 load_int8_64(const int8* mem)
{
    int8_64 simd;
    simd.s = _mm512_load_si512((__m512i *) mem);

    return simd;
}

inline int8_64 init_int8_64(const int8* mem)
{
    int8_64 simd;
    simd.s = _mm512_set_epi8(
        mem[0], mem[1], mem[2], mem[3], mem[4], mem[5], mem[6], mem[7],
        mem[8], mem[9], mem[10], mem[11], mem[12], mem[13], mem[14], mem[15],
        mem[16], mem[17], mem[18], mem[19], mem[20], mem[21], mem[22], mem[23],
        mem[24], mem[25], mem[26], mem[27], mem[28], mem[29], mem[30], mem[31],
        mem[32], mem[33], mem[34], mem[35], mem[36], mem[37], mem[38], mem[39],
        mem[40], mem[41], mem[42], mem[43], mem[44], mem[45], mem[46], mem[47],
        mem[48], mem[49], mem[50], mem[51], mem[52], mem[53], mem[54], mem[55],
        mem[56], mem[57], mem[58], mem[59], mem[60], mem[61], mem[62], mem[63]
    );

    return simd;
}

inline void unload_int8_64(int8_64 a, int8 *array) { _mm512_storeu_epi8(array, a.s); }

inline int8_64 init_zero_int8_64()
{
    int8_64 simd;
    simd.s = _mm512_setzero_si512();

    return simd;
}

inline int8_64 init_value_int8_64(int8 value)
{
    int8_64 simd;
    simd.s = _mm512_set1_epi8(value);

    return simd;
}

inline int8_64 operator+(int8_64 a, int8_64 b)
{
    int8_64 simd;
    simd.s = _mm512_add_epi8(a.s, b.s);

    return simd;
}

inline int8_64 operator-(int8_64 a, int8_64 b)
{
    int8_64 simd;
    simd.s = _mm512_sub_epi8(a.s, b.s);

    return simd;
}

inline int8_64 operator-(int8_64 a) { return init_zero_int8_64() - a; }

inline int8_64 operator*(int8_64 a, int8_64 b)
{
    int8_64 simd;
    simd.s = _mm512_mul_epi32(a.s, b.s);

    return simd;
}

inline int8_64 operator^(int8_64 a, int8_64 b)
{
    int8_64 simd;
    simd.s = _mm512_xor_si512(a.s, b.s);

    return simd;
}

inline int8_64 &operator-=(int8_64 &a, int8_64 b)
{
    a = a - b;

    return a;
}

inline int8_64 &operator+=(int8_64 &a, int8_64 b)
{
    a = a + b;

    return a;
}

inline int8_64 &operator*=(int8_64 &a, int8_64 b)
{
    a = a * b;

    return a;
}

inline int8_64 &operator^=(int8_64 &a, int8_64 b)
{
    a = a ^ b;

    return a;
}

inline int8_64 operator<(int8_64 a, int8_64 b)
{
    int8_64 simd;
    simd.s = _mm512_mask_blend_epi8(_mm512_cmplt_epi8_mask(a.s, b.s), a.s, b.s);

    return simd;
}

inline int8_64 operator<=(int8_64 a, int8_64 b)
{
    int8_64 simd;
    simd.s = _mm512_mask_blend_epi8(_mm512_cmple_epi8_mask(a.s, b.s), b.s, a.s);

    return simd;
}

inline int8_64 operator>(int8_64 a, int8_64 b)
{
    int8_64 simd;
    simd.s = _mm512_mask_blend_epi8(_mm512_cmpgt_epi8_mask(a.s, b.s), a.s, b.s);

    return simd;
}

inline int8_64 operator>=(int8_64 a, int8_64 b)
{
    int8_64 simd;
    simd.s = _mm512_mask_blend_epi8(_mm512_cmpge_epi8_mask(a.s, b.s), a.s, b.s);

    return simd;
}

inline int8_64 operator==(int8_64 a, int8_64 b)
{
    int8_64 simd;
    simd.s = _mm512_mask_blend_epi8(_mm512_cmpeq_epi8_mask(a.s, b.s), a.s, b.s);

    return simd;
}

inline int8_64 operator!=(int8_64 a, int8_64 b)
{
    int8_64 simd;
    simd.s = _mm512_mask_blend_epi8(_mm512_cmp_epi8_mask(a.s, b.s, _MM_CMPINT_NE), a.s, b.s);

    return simd;
}

inline int8_64 operator&(int8_64 a, int8_64 b)
{
    int8_64 simd;
    simd.s = _mm512_and_si512(a.s, b.s);

    return simd;
}

inline int8_64 operator|(int8_64 a, int8_64 b)
{
    int8_64 simd;
    simd.s = _mm512_or_si512(a.s, b.s);

    return simd;
}

inline int8_64 &operator&=(int8_64 &a, int8_64 b)
{
    a = a & b;

    return a;
}

inline int8_64 &operator|=(int8_64 &a, int8_64 b)
{
    a = a | b;

    return a;
}

inline int8_64 abs(int8_64 a)
{
    int8_64 simd;
    simd.s = _mm512_abs_epi16(a.s);

    return simd;
}

inline int8_64 simd_min(int8_64 a, int8_64 b)
{
    int8_64 simd;
    simd.s = _mm512_min_epi8(a.s, b.s);

    return simd;
}

inline int8_64 simd_max(int8_64 a, int8_64 b)
{
    int8_64 simd;
    simd.s = _mm512_max_epi8(a.s, b.s);

    return simd;
}

inline int8_64 clamp(int8_64 min_value, int8_64 a, int8_64 max_value)
{
    return simd_min(simd_max(a, min_value), max_value);
}

inline int64 which_true(int8_64 a)
{
    int64 which_true = _mm512_movepi8_mask(a.s);

    return which_true;
}

inline bool any_true(int8_64 a)
{
    bool is_any_true = _mm512_movepi8_mask(a.s) > 0;

    return is_any_true;
}

inline bool all_true(int8_64 a)
{
    bool is_true = _mm512_movepi8_mask(a.s) == 65535;

    return is_true;
}

inline bool all_false(int8_64 a)
{
    // @todo This can be optimized (requires also changes in the comparison functions return)
    bool is_false = _mm512_movepi8_mask(a.s) == 0;

    return is_false;
}

#endif
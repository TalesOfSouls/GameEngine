/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_SVML_AVX512_H
#define COMS_STDLIB_SIMD_SVML_AVX512_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Types.h"

#if __linux__
    #include <math.h>

    inline __m512i _mm512_div_epi32(__m512i a, __m512i b) {
        alignas(64) int32 a_array[16], b_array[16], result[16];

        _mm512_store_si512((__m512i*) a_array, a);
        _mm512_store_si512((__m512i*) b_array, b);

        for (int32 i = 0; i < 16; ++i) {
            result[i] = a_array[i] / b_array[i];
        }

        return _mm512_load_si512((__m512i*) result);
    }

    inline __m512 _mm512_sin_ps(__m512 a) {
        alignas(64) f32 a_array[8], result[8];
        _mm512_store_ps(a_array, a);
        for (int32 i = 0; i < 16; ++i) {
            result[i] = sinf(a_array[i]);
        }
        return _mm512_load_ps(result);
    }

    inline __m512 _mm512_cos_ps(__m512 a) {
        alignas(64) f32 a_array[8], result[8];
        _mm512_store_ps(a_array, a);
        for (int32 i = 0; i < 16; ++i) {
            result[i] = cosf(a_array[i]);
        }
        return _mm512_load_ps(result);
    }

    inline __m512 _mm512_asin_ps(__m512 a) {
        alignas(64) f32 a_array[8], result[8];
        _mm512_store_ps(a_array, a);
        for (int32 i = 0; i < 16; ++i) {
            result[i] = asinf(a_array[i]);
        }
        return _mm512_load_ps(result);
    }

    inline __m512 _mm512_acos_ps(__m512 a) {
        alignas(64) f32 a_array[16], result[16];
        _mm512_store_ps(a_array, a);
        for (int32 i = 0; i < 16; ++i) {
            result[i] = acosf(a_array[i]);
        }
        return _mm512_load_ps(result);
    }
#endif

#endif
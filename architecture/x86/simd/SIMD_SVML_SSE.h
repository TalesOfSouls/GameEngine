/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_SVML_SSE_H
#define COMS_STDLIB_SIMD_SVML_SSE_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Types.h"

#if __linux__
    #include <math.h>

    inline __m128i _mm_div_epi32(__m128i a, __m128i b) {
        alignas(16) int32 a_array[4], b_array[4], result[4];

        _mm_store_si128((__m128i*) a_array, a);
        _mm_store_si128((__m128i*) b_array, b);

        for (int32 i = 0; i < 4; ++i) {
            result[i] = a_array[i] / b_array[i];
        }

        return _mm_load_si128((__m128i*) result);
    }

    inline __m128 _mm_sin_ps(__m128 a) {
        alignas(16) f32 a_array[4], result[4];
        _mm_store_ps(a_array, a);
        for (int32 i = 0; i < 4; ++i) {
            result[i] = sinf(a_array[i]);
        }
        return _mm_load_ps(result);
    }

    inline __m128 _mm_cos_ps(__m128 a) {
        alignas(16) f32 a_array[4], result[4];
        _mm_store_ps(a_array, a);
        for (int32 i = 0; i < 4; ++i) {
            result[i] = cosf(a_array[i]);
        }
        return _mm_load_ps(result);
    }

    inline __m128 _mm_asin_ps(__m128 a) {
        alignas(16) f32 a_array[4], result[4];
        _mm_store_ps(a_array, a);
        for (int32 i = 0; i < 4; ++i) {
            result[i] = asinf(a_array[i]);
        }
        return _mm_load_ps(result);
    }

    inline __m128 _mm_acos_ps(__m128 a) {
        alignas(16) f32 a_array[4], result[4];
        _mm_store_ps(a_array, a);
        for (int32 i = 0; i < 4; ++i) {
            result[i] = acosf(a_array[i]);
        }
        return _mm_load_ps(result);
    }
#endif

#endif
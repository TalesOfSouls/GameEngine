/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_SVML_AVX2_H
#define COMS_STDLIB_SIMD_SVML_AVX2_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Types.h"

#if __linux__
    #include <math.h>
    inline __m256i _mm256_div_epi32(__m256i a, __m256i b) {
        alignas(32) int32 a_array[8], b_array[8], result[8];

        _mm256_store_si256((__m256i*) a_array, a);
        _mm256_store_si256((__m256i*) b_array, b);

        for (int32 i = 0; i < 8; ++i) {
            result[i] = a_array[i] / b_array[i];
        }

        return _mm256_load_si256((__m256i*) result);
    }

    inline __m256 _mm256_sin_ps(__m256 a) {
        alignas(32) f32 a_array[8], result[8];
        _mm256_store_ps(a_array, a);
        for (int32 i = 0; i < 8; ++i) {
            result[i] = sinf(a_array[i]);
        }
        return _mm256_load_ps(result);
    }

    inline __m256 _mm256_cos_ps(__m256 a) {
        alignas(32) f32 a_array[8], result[8];
        _mm256_store_ps(a_array, a);
        for (int32 i = 0; i < 8; ++i) {
            result[i] = cosf(a_array[i]);
        }
        return _mm256_load_ps(result);
    }

    inline __m256 _mm256_asin_ps(__m256 a) {
        alignas(32) f32 a_array[8], result[8];
        _mm256_store_ps(a_array, a);
        for (int32 i = 0; i < 8; ++i) {
            result[i] = asinf(a_array[i]);
        }
        return _mm256_load_ps(result);
    }

    inline __m256 _mm256_acos_ps(__m256 a) {
        alignas(32) f32 a_array[8], result[8];
        _mm256_store_ps(a_array, a);
        for (int32 i = 0; i < 16; ++i) {
            result[i] = acosf(a_array[i]);
        }
        return _mm256_load_ps(result);
    }
#endif

#endif
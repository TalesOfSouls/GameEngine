/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_I16_H
#define COMS_STDLIB_SIMD_I16_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Types.h"

#ifdef __SSE4_2__
    #include "SIMD_I16_SSE.h"
#endif

#ifdef __AVX2__
    #include "SIMD_I16_AVX2.h"
#endif

#ifdef __AVX512F__
    #include "SIMD_I16_AVX512.h"
#endif

// @todo from down here we can optimize some of the code by NOT using the wrappers
//      the code is self contained and we could use te intrinsic functions directly

inline
void simd_mult(const int16* a, f32 b, int16* result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) a, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX512F__
        if (steps >= 16) {
            steps = 16;
            __m512i a_16;
            __m512 af_lo, af_hi;
            __m512 b_16 = _mm512_set1_ps(b);
            __m512 result_lo, result_hi;
            __m512i result_16;

            for (; i <= size - steps; i += steps) {
                a_16 = _mm512_load_si512((__m512i*) a);

                af_lo = _mm512_cvtepi32_ps(_mm512_cvtepi16_epi32(_mm512_extracti64x4_epi64(a_16, 0)));
                af_hi = _mm512_cvtepi32_ps(_mm512_cvtepi16_epi32(_mm512_extracti64x4_epi64(a_16, 1)));

                result_lo = _mm512_mul_ps(af_lo, b_16);
                result_hi = _mm512_mul_ps(af_hi, b_16);

                result_16 = _mm512_packs_epi32(_mm512_cvtps_epi32(result_lo), _mm512_cvtps_epi32(result_hi));
                _mm512_store_si512((__m512i*) result, result_16);

                a += steps;
                result += steps;
            }

            steps = 1;
        }
    #endif

    #ifdef __AVX2__
        if (steps >= 8) {
            steps = 8;
            __m256i a_8;
            __m256 af_lo, af_hi;
            __m256 b_8 = _mm256_set1_ps(b);
            __m256 result_lo, result_hi;
            __m256i result_8;

            for (; i <= size - steps; i += steps) {
                a_8 = _mm256_load_si256((__m256i*) a);

                af_lo = _mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(_mm256_extracti128_si256(a_8, 0)));
                af_hi = _mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(_mm256_extracti128_si256(a_8, 1)));

                result_lo = _mm256_mul_ps(af_lo, b_8);
                result_hi = _mm256_mul_ps(af_hi, b_8);

                result_8 = _mm256_packs_epi32(_mm256_cvtps_epi32(result_lo), _mm256_cvtps_epi32(result_hi));
                _mm256_store_si256((__m256i*) result, result_8);

                a += steps;
                result += steps;
            }

            steps = 1;
        }
    #endif

    #ifdef __SSE4_2__
        if (steps >= 4) {
            steps = 4;
            __m128i a_4;
            __m128 af_lo, af_hi;
            __m128 b_4 = _mm_set1_ps(b);
            __m128 result_lo, result_hi;
            __m128i result_4;

            for (; i <= size - steps; i += steps) {
                a_4 = _mm_load_si128((__m128i*) a);

                af_lo = _mm_cvtepi32_ps(_mm_cvtepi16_epi32(a_4));
                af_hi = _mm_cvtepi32_ps(_mm_cvtepi16_epi32(_mm_srli_si128(a_4, 8)));

                result_lo = _mm_mul_ps(af_lo, b_4);
                result_hi = _mm_mul_ps(af_hi, b_4);

                result_4 = _mm_packs_epi32(_mm_cvtps_epi32(result_lo), _mm_cvtps_epi32(result_hi));
                _mm_store_si128((__m128i*) result, result_4);

                a += steps;
                result += steps;
            }
        }
    #endif

    // Handle any remaining elements
    for (; i < size; ++i) {
        *result = (int16) ((f32) (*a) * b);
        ++a;
        ++result;
    }
}

#endif
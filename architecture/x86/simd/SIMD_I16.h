/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_STDLIB_SIMD_I16_H
#define COMS_STDLIB_SIMD_I16_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Stdlib.h"

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

/**
 * result[i] = (int16) (a[i] * b), truncated like the scalar cast
 * The SIMD paths saturate values outside of the int16 range (the scalar cast of such a value is undefined behavior)
 *
 * steps = 32 bit lanes of the widest allowed register (16 = AVX-512, 8 = AVX2, 4 = SSE), lowered for unaligned memory
 * Every iteration processes a whole register of int16 values (32 / 16 / 8) -> never writes behind result + size
 */
inline
void simd_mult(const int16* __restrict a, f32 b, int16* __restrict result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) a, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX512F__
        if (steps >= 16) {
            const __m512 b_16 = _mm512_set1_ps(b);

            // 32 int16 per iteration
            for (; i <= size - 32; i += 32) {
                const __m512i a_16 = _mm512_load_si512((const __m512i*) a);

                const __m512 lo = _mm512_mul_ps(_mm512_cvtepi32_ps(_mm512_cvtepi16_epi32(_mm512_extracti64x4_epi64(a_16, 0))), b_16);
                const __m512 hi = _mm512_mul_ps(_mm512_cvtepi32_ps(_mm512_cvtepi16_epi32(_mm512_extracti64x4_epi64(a_16, 1))), b_16);

                // cvtsepi32_epi16 keeps the element order (packs works per 128 bit lane)
                const __m256i lo_16 = _mm512_cvtsepi32_epi16(_mm512_cvttps_epi32(lo));
                const __m256i hi_16 = _mm512_cvtsepi32_epi16(_mm512_cvttps_epi32(hi));
                _mm512_store_si512((__m512i*) result, _mm512_inserti64x4(_mm512_castsi256_si512(lo_16), hi_16, 1));

                a += 32;
                result += 32;
            }

            steps = 1;
        }
    #endif

    #ifdef __AVX2__
        if (steps >= 8) {
            const __m256 b_8 = _mm256_set1_ps(b);

            // 16 int16 per iteration
            for (; i <= size - 16; i += 16) {
                const __m256i a_8 = _mm256_load_si256((const __m256i*) a);

                const __m256 lo = _mm256_mul_ps(_mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(_mm256_extracti128_si256(a_8, 0))), b_8);
                const __m256 hi = _mm256_mul_ps(_mm256_cvtepi32_ps(_mm256_cvtepi16_epi32(_mm256_extracti128_si256(a_8, 1))), b_8);

                // packs works per 128 bit lane (elements 0-3, 8-11, 4-7, 12-15) -> restore the order of the 64 bit blocks
                const __m256i packed = _mm256_packs_epi32(_mm256_cvttps_epi32(lo), _mm256_cvttps_epi32(hi));
                _mm256_store_si256((__m256i*) result, _mm256_permute4x64_epi64(packed, 0xD8));

                a += 16;
                result += 16;
            }

            steps = 1;
        }
    #endif

    #ifdef __SSE4_2__
        if (steps >= 4) {
            const __m128 b_4 = _mm_set1_ps(b);

            // 8 int16 per iteration
            for (; i <= size - 8; i += 8) {
                const __m128i a_4 = _mm_load_si128((const __m128i*) a);

                const __m128 lo = _mm_mul_ps(_mm_cvtepi32_ps(_mm_cvtepi16_epi32(a_4)), b_4);
                const __m128 hi = _mm_mul_ps(_mm_cvtepi32_ps(_mm_cvtepi16_epi32(_mm_srli_si128(a_4, 8))), b_4);

                _mm_store_si128((__m128i*) result, _mm_packs_epi32(_mm_cvttps_epi32(lo), _mm_cvttps_epi32(hi)));

                a += 8;
                result += 8;
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
/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_F32_H
#define COMS_STDLIB_SIMD_F32_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Types.h"

#ifdef __SSE4_2__
    #include "SIMD_F32_SSE.h"
#endif

#ifdef __AVX2__
    #include "SIMD_F32_AVX2.h"
#endif

#ifdef __AVX512F__
    #include "SIMD_F32_AVX512.h"
#endif

// @todo from down here we can optimize some of the code by NOT using the wrappers
//      the code is self contained and we could use te intrinsic functions directly

void simd_mult(const f32* a, const f32* b, f32* result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) a, steps);
    steps = intrin_validate_steps((const byte*) b, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX512F__
        if (steps >= 16) {
            steps = 16;
            __m512 a_16;
            __m512 b_16;
            __m512 result_16;

            for (; i <= size - steps; i += steps) {
                a_16 = _mm512_load_ps(a);
                b_16 = _mm512_load_ps(b);
                result_16 = _mm512_mul_ps(a_16, b_16);
                _mm512_store_ps(result, result_16);

                a += steps;
                b += steps;
                result += steps;
            }

            steps = 1;
        }
    #endif

    #ifdef __AVX2__
        if (steps >= 8) {
            steps = 8;
            __m256 a_8;
            __m256 b_8;
            __m256 result_8;

            for (; i <= size - steps; i += steps) {
                a_8 = _mm256_load_ps(a);
                b_8 = _mm256_load_ps(b);
                result_8 = _mm256_mul_ps(a_8, b_8);
                _mm256_store_ps(result, result_8);

                a += steps;
                b += steps;
                result += steps;
            }

            steps = 1;
        }
    #endif

    #ifdef __SSE4_2__
        if (steps >= 4) {
            steps = 4;
            __m128 a_4;
            __m128 b_4;
            __m128 result_4;

            for (; i <= size - steps; i += steps) {
                a_4 = _mm_load_ps(a);
                b_4 = _mm_load_ps(b);
                result_4 = _mm_mul_ps(a_4, b_4);
                _mm_store_ps(result, result_4);

                a += steps;
                b += steps;
                result += steps;
            }
        }
    #endif

    for (; i < size; ++i) {
        *result = *a * *b;

        ++a;
        ++b;
        ++result;
    }
}

inline
void simd_mult(const f32* a, f32 b, f32* result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) a, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX512F__
        if (steps >= 16) {
            steps = 16;
            __m512 a_16;
            __m512 b_16 = _mm512_set1_ps(b);
            __m512 result_16;

            for (; i <= size - steps; i += steps) {
                a_16 = _mm512_load_ps(a);
                result_16 = _mm512_mul_ps(a_16, b_16);
                _mm512_store_ps(result, result_16);

                a += steps;
                result += steps;
            }
        }
    #endif

    #ifdef __AVX2__
    if (steps >= 8) {
        steps = 8;
        __m256 a_8;
        __m256 b_8 = _mm256_set1_ps(b);
        __m256 result_8;

        for (; i <= size - steps; i += steps) {
            a_8 = _mm256_load_ps(a);
            result_8 = _mm256_mul_ps(a_8, b_8);
            _mm256_store_ps(result, result_8);

            a += steps;
            result += steps;
        }
    }
    #endif

    #ifdef __SSE4_2__
    if (steps >= 4) {
        steps = 4;
        __m128 a_4;
        __m128 b_4 = _mm_set1_ps(b);
        __m128 result_4;

        for (; i <= size - steps; i += steps) {
            a_4 = _mm_load_ps(a);
            result_4 = _mm_mul_ps(a_4, b_4);
            _mm_store_ps(result, result_4);

            a += steps;
            result += steps;
        }
    }
    #endif

    for (; i < size; ++i) {
        *result = *a * b;

        ++a;
        ++result;
    }
}

inline
void simd_div(const f32* a, f32 b, f32* result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) a, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX512F__
        if (steps >= 16) {
            steps = 16;
            __m512 a_16;
            __m512 b_16 = _mm512_set1_ps(b);
            __m512 result_16;

            for (; i <= size - steps; i += steps) {
                a_16 = _mm512_load_ps(a);
                result_16 = _mm512_div_ps(a_16, b_16);
                _mm512_store_ps(result, result_16);

                a += steps;
                result += steps;
            }
        }
    #endif

    #ifdef __AVX2__
        if (steps >= 8) {
            steps = 8;
            __m256 a_8;
            __m256 b_8 = _mm256_set1_ps(b);
            __m256 result_8;

            for (; i <= size - steps; i += steps) {
                a_8 = _mm256_load_ps(a);
                result_8 = _mm256_div_ps(a_8, b_8);
                _mm256_store_ps(result, result_8);

                a += steps;
                result += steps;
            }
        }
    #endif

    #ifdef __SSE4_2__
        if (steps >= 4) {
            steps = 4;
            __m128 a_4;
            __m128 b_4 = _mm_set1_ps(b);
            __m128 result_4;

            for (; i <= size - steps; i += steps) {
                a_4 = _mm_load_ps(a);
                result_4 = _mm_div_ps(a_4, b_4);
                _mm_store_ps(result, result_4);

                a += steps;
                result += steps;
            }
        }
    #endif

    // Scalar fallback
    for (; i < size; ++i) {
        *result = *a / b;

        ++a;
        ++result;
    }
}

#endif

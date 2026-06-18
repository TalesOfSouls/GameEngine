/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_STDLIB_SIMD_F32_H
#define COMS_STDLIB_SIMD_F32_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Stdlib.h"

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

inline
f32 simd_sum(const f32* data, int32 count, int32 steps)
{
    int32 i = 0;
    f32 sum = 0.0f;

    #if defined(__AVX512F__)
        if (steps >= 16 && count >= 16) {
            __m512 acc = _mm512_setzero_ps();

            for (; i + 15 < count; i += 16) {
                acc = _mm512_add_ps(
                    acc,
                    _mm512_load_ps(data + i)
                );
            }

            sum = _mm512_reduce_add_ps(acc);
            steps = 1;
        }
    #elif defined(__AVX2__)
        if (steps >= 8 && count >= 8) {
            __m256 acc = _mm256_setzero_ps();

            for (; i + 7 < count; i += 8) {
                acc = _mm256_add_ps(
                    acc,
                    _mm256_load_ps(data + i)
                );
            }

            __m128 lo = _mm256_castps256_ps128(acc);
            __m128 hi = _mm256_extractf128_ps(acc, 1);

            __m128 sum128 = _mm_add_ps(lo, hi);

            sum128 = _mm_hadd_ps(sum128, sum128);
            sum128 = _mm_hadd_ps(sum128, sum128);

            sum = _mm_cvtss_f32(sum128);
            steps = 1;
        }
    #elif defined(__SSE4_2__)
        if (steps >= 4 && count >= 4) {
            __m128 acc = _mm_setzero_ps();

            for (; i + 3 < count; i += 4) {
                acc = _mm_add_ps(
                    acc,
                    _mm_load_ps(data + i)
                );
            }

            acc = _mm_hadd_ps(acc, acc);
            acc = _mm_hadd_ps(acc, acc);

            sum = _mm_cvtss_f32(acc);
            steps = 1;
        }
    #endif

    for (; i < count; ++i) {
        sum += data[i];
    }

    return sum;
}

inline
int32 simd_sum(const int32* data, int32 count, int32 steps)
{
    int32 i = 0;
    int32 sum = 0;

    #if defined(__AVX512F__)
        if (steps >= 16 && count >= 16) {
            __m512i acc = _mm512_setzero_si512();

            for (; i + 15 < count; i += 16) {
                acc = _mm512_add_epi32(
                    acc,
                    _mm512_load_si512((const void*)(data + i))
                );
            }

            sum = _mm512_reduce_add_epi32(acc);
            steps = 1;
        }
    #elif defined(__AVX2__)
        if (steps >= 8 && count >= 8) {
            __m256i acc = _mm256_setzero_si256();

            for (; i + 7 < count; i += 8) {
                acc = _mm256_add_epi32(
                    acc,
                    _mm256_load_si256((const __m256i*)(data + i))
                );
            }

            __m128i lo = _mm256_castsi256_si128(acc);
            __m128i hi = _mm256_extracti128_si256(acc, 1);

            __m128i sum128 = _mm_add_epi32(lo, hi);

            sum128 = _mm_hadd_epi32(sum128, sum128);
            sum128 = _mm_hadd_epi32(sum128, sum128);

            sum = _mm_cvtsi128_si32(sum128);
            steps = 1;
        }
    #elif defined(__SSE4_1__)
        if (steps >= 4 && count >= 4) {
            __m128i acc = _mm_setzero_si128();

            for (; i + 3 < count; i += 4) {
                acc = _mm_add_epi32(
                    acc,
                    _mm_load_si128((const __m128i*)(data + i))
                );
            }

            acc = _mm_hadd_epi32(acc, acc);
            acc = _mm_hadd_epi32(acc, acc);

            sum = _mm_cvtsi128_si32(acc);
            steps = 1;
        }
    #endif

    for (; i < count; ++i) {
        sum += data[i];
    }

    return sum;
}

inline
f32 simd_sum_unaligned(const f32* data, int32 count, int32 steps)
{
    int32 i = 0;
    f32 sum = 0.0f;

    #if defined(__AVX512F__)
        if (steps >= 16 && count >= 16) {
            __m512 acc = _mm512_setzero_ps();

            for (; i + 15 < count; i += 16) {
                acc = _mm512_add_ps(
                    acc,
                    _mm512_loadu_ps(data + i)
                );
            }

            sum = _mm512_reduce_add_ps(acc);
            steps = 1;
        }
    #elif defined(__AVX2__)
        if (steps >= 8 && count >= 8) {
            __m256 acc = _mm256_setzero_ps();

            for (; i + 7 < count; i += 8) {
                acc = _mm256_add_ps(
                    acc,
                    _mm256_loadu_ps(data + i)
                );
            }

            __m128 lo = _mm256_castps256_ps128(acc);
            __m128 hi = _mm256_extractf128_ps(acc, 1);

            __m128 sum128 = _mm_add_ps(lo, hi);

            sum128 = _mm_hadd_ps(sum128, sum128);
            sum128 = _mm_hadd_ps(sum128, sum128);

            sum = _mm_cvtss_f32(sum128);
            steps = 1;
        }
    #elif defined(__SSE4_2__)
        if (steps >= 4 && count >= 4) {
            __m128 acc = _mm_setzero_ps();

            for (; i + 3 < count; i += 4) {
                acc = _mm_add_ps(
                    acc,
                    _mm_loadu_ps(data + i)
                );
            }

            acc = _mm_hadd_ps(acc, acc);
            acc = _mm_hadd_ps(acc, acc);

            sum = _mm_cvtss_f32(acc);
            steps = 1;
        }
    #endif

    for (; i < count; ++i) {
        sum += data[i];
    }

    return sum;
}

inline
int32 simd_sum_unaligned(const int32* data, int32 count, int32 steps)
{
    int32 i = 0;
    int32 sum = 0;

    #if defined(__AVX512F__)
        if (steps >= 16 && count >= 16) {
            __m512i acc = _mm512_setzero_si512();

            for (; i + 15 < count; i += 16) {
                acc = _mm512_add_epi32(
                    acc,
                    _mm512_loadu_si512((const void*)(data + i))
                );
            }

            sum = _mm512_reduce_add_epi32(acc);
            steps = 1;
        }
    #elif defined(__AVX2__)
        if (steps >= 8 && count >= 8) {
            __m256i acc = _mm256_setzero_si256();

            for (; i + 7 < count; i += 8) {
                acc = _mm256_add_epi32(
                    acc,
                    _mm256_loadu_si256((const __m256i*)(data + i))
                );
            }

            __m128i lo = _mm256_castsi256_si128(acc);
            __m128i hi = _mm256_extracti128_si256(acc, 1);

            __m128i sum128 = _mm_add_epi32(lo, hi);

            sum128 = _mm_hadd_epi32(sum128, sum128);
            sum128 = _mm_hadd_epi32(sum128, sum128);

            sum = _mm_cvtsi128_si32(sum128);
            steps = 1;
        }
    #elif defined(__SSE4_1__)
        if (steps >= 4 && count >= 4) {
            __m128i acc = _mm_setzero_si128();

            for (; i + 3 < count; i += 4) {
                acc = _mm_add_epi32(
                    acc,
                    _mm_loadu_si128((const __m128i*)(data + i))
                );
            }

            acc = _mm_hadd_epi32(acc, acc);
            acc = _mm_hadd_epi32(acc, acc);

            sum = _mm_cvtsi128_si32(acc);
            steps = 1;
        }
    #endif

    for (; i < count; ++i) {
        sum += data[i];
    }

    return sum;
}

void simd_mult(const f32* __restrict a, const f32* __restrict b, f32* __restrict result, int32 size, int32 steps = 16)
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
void simd_mult(const f32* __restrict a, f32 b, f32* __restrict result, int32 size, int32 steps = 16)
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
void simd_div(const f32* __restrict a, f32 b, f32* __restrict result, int32 size, int32 steps = 16)
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

inline
__m256 dot3_avx(__m256 ax, __m256 ay, __m256 az, __m256 bx, __m256 by, __m256 bz) {
    return _mm256_add_ps(
        _mm256_add_ps(_mm256_mul_ps(ax, bx), _mm256_mul_ps(ay, by)),
        _mm256_mul_ps(az, bz)
    );
}

#endif

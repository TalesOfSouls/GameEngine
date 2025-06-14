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

#ifdef __SSE4_2__
    #include "SIMD_I8_SSE.h"
#endif

#ifdef __AVX2__
    #include "SIMD_I8_AVX2.h"
#endif

#ifdef __AVX512F__
    #include "SIMD_I8_AVX512.h"
#endif

int simd_equal(const byte* a, const byte* b, uint32 size, uint32 steps = 8) {
    uint32 i = 0;
    steps = intrin_validate_steps((const byte*) a, steps);
    steps = intrin_validate_steps((const byte*) b, steps);

    #ifdef __AVX512F__
        if (steps >= 16) {
            steps = 16;
            if (size >= 128) {
                __m512i a_16;
                __m512i b_16;
                __mmask64 result_mask;

                for (; i <= size - 64; i += 64) {  // 64 bytes per iteration
                    a_16 = _mm512_load_si512((__m512i*) a);
                    b_16 = _mm512_load_si512((__m512i*) b);

                    result_mask = _mm512_cmpeq_epi8_mask(a_16, b_16);

                    if (result_mask != 0xFFFFFFFFFFFFFFFF) {
                        return false;
                    }

                    a += 64;
                    b += 64;
                }
            }

            if (size - i >= 64) {
                return simd_equal(a, b, size - i, 8);
            } else if (size - i >= 32) {
                return simd_equal(a, b, size - i, 4);
            }
        }
    #endif

    #ifdef __AVX2__
        if (steps >= 8) {
            steps = 8;
            if (size >= 64) {
                __m256i a_8;
                __m256i b_8;
                __m256i result_8;

                for (; i <= size - steps; i += steps) {
                    a_8 = _mm256_load_si256((__m256i*) a);
                    b_8 = _mm256_load_si256((__m256i*) b);

                    result_8 = _mm256_cmpeq_epi8(a_8, b_8);

                    if (_mm256_testc_si256(result_8, _mm256_set1_epi8(-1)) != 1) {
                        return false;
                    }

                    a += steps;
                    b += steps;
                }
            }

            if (size - i >= 32) {
                return simd_equal(a, b, size - i, 4);
            }
        }
    #endif

    #ifdef __SSE4_2__
        if (steps >= 4) {
            steps = 4;
            if (size >= 16) {
                __m128i a_4;
                __m128i b_4;
                __m128i result_4;

                for (; i <= size - steps; i += steps) {
                    a_4 = _mm_load_si128((__m128i*) a);
                    b_4 = _mm_load_si128((__m128i*) b);

                    result_4 = _mm_cmpeq_epi8(a_4, b_4);

                    if (_mm_movemask_epi8(result_4) != 0xFFFF) {
                        return false;
                    }

                    a += steps;
                    b += steps;
                }
            }
        }
    #endif

    for (; i < size; ++i) {
        if (*a++ != *b++) {
            return false;
        }
    }

    return true;
}

#endif
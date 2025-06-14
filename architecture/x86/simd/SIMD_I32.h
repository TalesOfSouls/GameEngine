/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_I32_H
#define COMS_STDLIB_SIMD_I32_H

#include <immintrin.h>
#include <xmmintrin.h>
#include <emmintrin.h>

#include "../../../stdlib/Types.h"
#include "../../../utils/BitUtils.h"

#ifdef __SSE4_2__
    #include "SIMD_I32_SSE.h"
#endif

#ifdef __AVX2__
    #include "SIMD_I32_AVX2.h"
#endif

#ifdef __AVX512F__
    #include "SIMD_I32_AVX512.h"
#endif

inline
void simd_mult(const int32* a, const int32* b, int32* result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) a, steps);
    steps = intrin_validate_steps((const byte*) b, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX512F__
        if (steps >= 16) {
            steps = 16;
            __m512i a_16;
            __m512i b_16;
            __m512i result_16;

            for (; i <= size - steps; i += steps) {
                a_16 = _mm512_load_epi32(a);
                b_16 = _mm512_load_epi32(b);
                result_16 = _mm512_mul_epi32(a_16, b_16);
                _mm512_store_epi32(result, result_16);

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
            __m256i a_8;
            __m256i b_8;
            __m256i result_8;

            for (; i <= size - steps; i += steps) {
                a_8 = _mm256_load_si256((__m256i *) a);
                b_8 = _mm256_load_si256((__m256i *) b);
                result_8 = _mm256_mul_epi32(a_8, b_8);
                _mm256_store_si256((__m256i *) result, result_8);

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
            __m128i a_4;
            __m128i b_4;
            __m128i result_4;

            for (; i <= size - steps; i += steps) {
                a_4 = _mm_load_si128((__m128i *) a);
                b_4 = _mm_load_si128((__m128i *) b);
                result_4 = _mm_mul_epi32(a_4, b_4);
                _mm_store_si128((__m128i *) result, result_4);

                a += steps;
                b += steps;
                result += steps;
            }
        }
    #endif

    for (; i < size; ++i) {
        ++a;
        ++b;
        ++result;

        *result = *a * *b;
    }
}

inline
void simd_mult(const int32* a, const f32* b, f32* result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) a, steps);
    steps = intrin_validate_steps((const byte*) b, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX512F__
        if (steps >= 16) {
            steps = 16;
            __m512i a_16;
            __m512 af_16;
            __m512 b_16;
            __m512 result_16;

            for (; i <= size - steps; i += steps) {
                a_16 = _mm512_load_epi32(a);
                af_16 = _mm512_cvtepi32_ps(a_16);
                b_16 = _mm512_load_ps(b);
                result_16 = _mm512_mul_ps(af_16, b_16);
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
            __m256i a_8;
            __m256 af_8;
            __m256 b_8;
            __m256 result_8;

            for (; i <= size - steps; i += steps) {
                a_8 = _mm256_load_si256((__m256i *) a);
                af_8 = _mm256_cvtepi32_ps(a_8);
                b_8 = _mm256_load_ps(b);
                result_8 = _mm256_mul_ps(af_8, b_8);
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
            __m128i a_4;
            __m128 af_4;
            __m128 b_4;
            __m128 result_4;

            for (; i <= size - steps; i += steps) {
                a_4 = _mm_load_si128((__m128i *) a);
                af_4 = _mm_cvtepi32_ps(a_4);
                b_4 = _mm_load_ps(b);
                result_4 = _mm_mul_ps(af_4, b_4);
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
void simd_mult(const int32* a, const f32* b, int32* result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) a, steps);
    steps = intrin_validate_steps((const byte*) b, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX512F__
        if (steps >= 16) {
            steps = 16;
            __m512i a_16;
            __m512 af_16;
            __m512 b_16;
            __m512 result_16;
            __m512i resulti_16;

            for (; i <= size - steps; i += steps) {
                a_16 = _mm512_load_epi32(a);
                af_16 = _mm512_cvtepi32_ps(a_16);
                b_16 = _mm512_load_ps(b);
                result_16 = _mm512_mul_ps(af_16, b_16);
                resulti_16 = _mm512_cvtps_epi32(result_16);
                _mm512_store_epi32(result, resulti_16);

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
            __m256i a_8;
            __m256 af_8;
            __m256 b_8;
            __m256 result_8;
            __m256i resulti_8;

            for (; i <= size - steps; i += steps) {
                a_8 = _mm256_load_si256((__m256i *) a);
                af_8 = _mm256_cvtepi32_ps(a_8);
                b_8 = _mm256_load_ps(b);
                result_8 = _mm256_mul_ps(af_8, b_8);
                resulti_8 = _mm256_cvtps_epi32(result_8);
                _mm256_store_si256((__m256i *) result, resulti_8);

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
            __m128i a_4;
            __m128 af_4;
            __m128 b_4;
            __m128 result_4;
            __m128i resulti_4;

            for (; i <= size - steps; i += steps) {
                a_4 = _mm_load_si128((__m128i *) a);
                af_4 = _mm_cvtepi32_ps(a_4);
                b_4 = _mm_load_ps(b);
                result_4 = _mm_mul_ps(af_4, b_4);
                resulti_4 = _mm_cvtps_epi32(result_4);
                _mm_store_si128((__m128i *) result, resulti_4);

                a += steps;
                b += steps;
                result += steps;
            }
        }
    #endif

    for (; i < size; ++i) {
        *result = (int32) (*a * *b);

        ++a;
        ++b;
        ++result;
    }
}

inline
void simd_mult(const int32* a, f32 b, int32* result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) a, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX512F__
        if (steps >= 16) {
            steps = 16;
            __m512i a_16;
            __m512 af_16;
            __m512 b_16 = _mm512_set1_ps(b);
            __m512 result_16;
            __m512i resulti_16;

            for (; i <= size - steps; i += steps) {
                a_16 = _mm512_load_epi32(a);
                af_16 = _mm512_cvtepi32_ps(a_16);
                result_16 = _mm512_mul_ps(af_16, b_16);
                resulti_16 = _mm512_cvtps_epi32(result_16);
                _mm512_store_epi32(result, resulti_16);

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
            __m256 af_8;
            __m256 b_8 = _mm256_set1_ps(b);
            __m256 result_8;
            __m256i resulti_8;

            for (; i <= size - steps; i += steps) {
                a_8 = _mm256_load_si256((__m256i *) a);
                af_8 = _mm256_cvtepi32_ps(a_8);
                result_8 = _mm256_mul_ps(af_8, b_8);
                resulti_8 = _mm256_cvtps_epi32(result_8);
                _mm256_store_si256((__m256i *) result, resulti_8);

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
            __m128 af_4;
            __m128 b_4 = _mm_set1_ps(b);
            __m128 result_4;
            __m128i resulti_4;

            for (; i <= size - steps; i += steps) {
                a_4 = _mm_load_si128((__m128i *) a);
                af_4 = _mm_cvtepi32_ps(a_4);
                result_4 = _mm_mul_ps(af_4, b_4);
                resulti_4 = _mm_cvtps_epi32(result_4);
                _mm_store_si128((__m128i *) result, resulti_4);

                a += steps;
                result += steps;
            }
        }
    #endif

    for (; i < size; ++i) {
        *result = (int32) (*a * b);

        ++a;
        ++result;
    }
}

inline
void simd_div(const int32* a, f32 b, f32* result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) a, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX512F__
        if (steps >= 16) {
            steps = 16;
            __m512i a_16;
            __m512 af_16;
            __m512 b_16 = _mm512_set1_ps(b);
            __m512 result_16;

            for (; i <= size - steps; i += steps) {
                a_16 = _mm512_load_epi32(a);
                af_16 = _mm512_cvtepi32_ps(a_16);
                result_16 = _mm512_div_ps(af_16, b_16);
                _mm512_store_ps(result, result_16);

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
            __m256 af_8;
            __m256 b_8 = _mm256_set1_ps(b);
            __m256 result_8;

            for (; i <= size - steps; i += steps) {
                a_8 = _mm256_load_si256((__m256i *) a);
                af_8 = _mm256_cvtepi32_ps(a_8);
                result_8 = _mm256_div_ps(af_8, b_8);
                _mm256_store_ps(result, result_8);

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
            __m128 af_4;
            __m128 b_4 = _mm_set1_ps(b);
            __m128 result_4;

            for (; i <= size - steps; i += steps) {
                a_4 = _mm_load_si128((__m128i *) a);
                af_4 = _mm_cvtepi32_ps(a_4);
                result_4 = _mm_div_ps(af_4, b_4);
                _mm_store_ps(result, result_4);

                a += steps;
                result += steps;
            }
        }
    #endif

    for (; i < size; ++i) {
        *result = *a / b;

        ++a;
        ++result;
    }
}

inline
void simd_add(const int32* a, const int32* b, int32* result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) a, steps);
    steps = intrin_validate_steps((const byte*) b, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX512F__
        if (steps >= 16) {
            steps = 16;
            __m512i a_16;
            __m512i b_16;
            __m512i result_16;

            for (; i <= size - steps; i += steps) {
                a_16 = _mm512_load_epi32(a);
                b_16 = _mm512_load_epi32(b);
                result_16 = _mm512_add_epi32(a_16, b_16);
                _mm512_store_epi32(result, result_16);

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
            __m256i a_8;
            __m256i b_8;
            __m256i result_8;

            for (; i <= size - steps; i += steps) {
                a_8 = _mm256_load_si256((__m256i *) a);
                b_8 = _mm256_load_si256((__m256i *) b);
                result_8 = _mm256_add_epi32(a_8, b_8);
                _mm256_store_si256((__m256i *) result, result_8);

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
            __m128i a_4;
            __m128i b_4;
            __m128i result_4;

            for (; i <= size - steps; i += steps) {
                a_4 = _mm_load_si128((__m128i *) a);
                b_4 = _mm_load_si128((__m128i *) b);
                result_4 = _mm_add_epi32(a_4, b_4);
                _mm_store_si128((__m128i *) result, result_4);

                a += steps;
                b += steps;
                result += steps;
            }
        }
    #endif

    for (; i < size; ++i) {
        *result = *a + *b;

        ++a;
        ++b;
        ++result;
    }
}

inline
void simd_add(const int32* a, const f32* b, f32* result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) a, steps);
    steps = intrin_validate_steps((const byte*) b, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX512F__
        if (steps >= 16) {
            steps = 16;
            __m512i a_16;
            __m512 af_16;
            __m512 b_16;
            __m512 result_16;

            for (; i <= size - steps; i += steps) {
                a_16 = _mm512_load_epi32(a);
                af_16 = _mm512_cvtepi32_ps(a_16);
                b_16 = _mm512_load_ps(b);
                result_16 = _mm512_add_ps(af_16, b_16);
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
            __m256i a_8;
            __m256 af_8;
            __m256 b_8;
            __m256 result_8;

            for (; i <= size - steps; i += steps) {
                a_8 = _mm256_load_si256((__m256i *) a);
                af_8 = _mm256_cvtepi32_ps(a_8);
                b_8 = _mm256_load_ps(b);
                result_8 = _mm256_add_ps(af_8, b_8);
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
            __m128i a_4;
            __m128 af_4;
            __m128 b_4;
            __m128 result_4;

            for (; i <= size - steps; i += steps) {
                a_4 = _mm_load_si128((__m128i *) a);
                af_4 = _mm_cvtepi32_ps(a_4);
                b_4 = _mm_load_ps(b);
                result_4 = _mm_add_ps(af_4, b_4);
                _mm_store_ps(result, result_4);

                a += steps;
                b += steps;
                result += steps;
            }
        }
    #endif

    for (; i < size; ++i) {
        *result = *a + *b;

        ++a;
        ++b;
        ++result;
    }
}

inline
void simd_add(const int32* a, const f32* b, int32* result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) a, steps);
    steps = intrin_validate_steps((const byte*) b, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX512F__
        if (steps >= 16) {
            steps = 16;
            __m512i a_16;
            __m512 af_16;
            __m512 b_16;
            __m512 result_16;
            __m512i resulti_16;

            for (; i <= size - steps; i += steps) {
                a_16 = _mm512_load_epi32(a);
                af_16 = _mm512_cvtepi32_ps(a_16);
                b_16 = _mm512_load_ps(b);
                result_16 = _mm512_add_ps(af_16, b_16);
                resulti_16 = _mm512_cvtps_epi32(result_16);
                _mm512_store_epi32(result, resulti_16);

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
            __m256i a_8;
            __m256 af_8;
            __m256 b_8;
            __m256 result_8;
            __m256i resulti_8;

            for (; i <= size - steps; i += steps) {
                a_8 = _mm256_load_si256((__m256i *) a);
                af_8 = _mm256_cvtepi32_ps(a_8);
                b_8 = _mm256_load_ps(b);
                result_8 = _mm256_add_ps(af_8, b_8);
                resulti_8 = _mm256_cvtps_epi32(result_8);
                _mm256_store_si256((__m256i *) result, resulti_8);

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
            __m128i a_4;
            __m128 af_4;
            __m128 b_4;
            __m128 result_4;
            __m128i resulti_4;

            for (; i <= size - steps; i += steps) {
                a_4 = _mm_load_si128((__m128i *) a);
                af_4 = _mm_cvtepi32_ps(a_4);
                b_4 = _mm_load_ps(b);
                result_4 = _mm_add_ps(af_4, b_4);
                resulti_4 = _mm_cvtps_epi32(result_4);
                _mm_store_si128((__m128i *) result, resulti_4);

                a += steps;
                b += steps;
                result += steps;
            }
        }
    #endif

    for (; i < size; ++i) {
        *result = (int32) (*a + *b);

        ++a;
        ++b;
        ++result;
    }
}

void
endian_swap(const int32* val, int32* result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) val, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX2__
        if (steps >= 8) {
            steps = 8;
            const __m256i mask_256 = _mm256_setr_epi8(
                3, 2, 1, 0,  7, 6, 5, 4,
                11, 10, 9, 8,  15, 14, 13, 12,
                19, 18, 17, 16,  23, 22, 21, 20,
                27, 26, 25, 24,  31, 30, 29, 28
            );

            for (i = 0; i <= size - steps; i += steps) {
                __m256i vec = _mm256_load_si256((const __m256i *) (val + i));
                vec = _mm256_shuffle_epi8(vec, mask_256);

                _mm256_store_si256((__m256i *) (result + i), vec);
            }

            steps = 1;
        }
    #endif

    #ifdef __SSE4_2__
        if (steps >= 4) {
            steps = 4;
            const __m128i mask_128 = _mm_setr_epi8(
                3, 2, 1, 0,
                7, 6, 5, 4,
                11, 10, 9, 8,
                15, 14, 13, 12
            );

            for (i = 0; i <= size - steps; i += steps) {
                __m128i vec = _mm_load_si128((__m128i *) (const __m128i *) (val + i));
                vec = _mm_shuffle_epi8(vec, mask_128);

                _mm_store_si128((__m128i *) (result + i), vec);
            }
        }
    #endif

    for (; i < size; ++i) {
        uint32 v = ((uint32 *) val)[i];
        ((int32 *) result)[i] = ((v << 24)
            | ((v & 0xFF00) << 8)
            | ((v >> 8) & 0xFF00)
            | (v >> 24));
    }
}

void
endian_swap(const uint32* val, uint32* result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) val, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX2__
        if (steps >= 8) {
            steps = 8;
            const __m256i mask_256 = _mm256_setr_epi8(
                3, 2, 1, 0,  7, 6, 5, 4,
                11, 10, 9, 8,  15, 14, 13, 12,
                19, 18, 17, 16,  23, 22, 21, 20,
                27, 26, 25, 24,  31, 30, 29, 28
            );

            for (i = 0; i <= size - steps; i += steps) {
                __m256i vec = _mm256_load_si256((const __m256i *) (val + i));
                vec = _mm256_shuffle_epi8(vec, mask_256);

                _mm256_store_si256((__m256i *) (result + i), vec);
            }

            steps = 1;
        }
    #endif

    #ifdef __SSE4_2__
        if (steps >= 4) {
            steps = 4;
            const __m128i mask_128 = _mm_setr_epi8(
                3, 2, 1, 0,
                7, 6, 5, 4,
                11, 10, 9, 8,
                15, 14, 13, 12
            );

            for (i = 0; i <= size - steps; i += steps) {
                __m128i vec = _mm_load_si128((__m128i *) (const __m128i *) (val + i));
                vec = _mm_shuffle_epi8(vec, mask_128);

                _mm_store_si128((__m128i *) (result + i), vec);
            }
        }
    #endif

    for (; i < size; ++i) {
        uint32 v = ((uint32 *) val)[i];
        ((uint32 *) result)[i] = ((v << 24)
            | ((v & 0xFF00) << 8)
            | ((v >> 8) & 0xFF00)
            | (v >> 24));
    }
}

void endian_swap(const int16* val, int16* result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) val, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX2__
    if (steps >= 8) {
        steps = 8;
        const __m256i mask_256 = _mm256_setr_epi8(
            1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14,
            17, 16, 19, 18, 21, 20, 23, 22, 25, 24, 27, 26, 29, 28, 31, 30
        );

        for (i = 0; i <= size - steps; i += steps) {
            __m256i vec = _mm256_load_si256((const __m256i *) (val + i));
            vec = _mm256_shuffle_epi8(vec, mask_256);

            _mm256_store_si256((__m256i *) (result + i), vec);
        }

        steps = 1;
    }
    #endif

    #ifdef __SSE4_2__
        if (steps >= 4) {
            steps = 4;
            const __m128i mask_128 = _mm_setr_epi8(
                1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14
            );

            for (i = 0; i <= size - steps; i += steps) {
                __m128i vec = _mm_load_si128((const __m128i *) (val + i));
                vec = _mm_shuffle_epi8(vec, mask_128);

                _mm_store_si128((__m128i *) (result + i), vec);
            }
        }
    #endif

    for (; i < size; ++i) {
        uint16 v = ((uint16 *) val)[i];
        ((int16 *) result)[i] = ((v << 8) | (v >> 8));
    }
}

void endian_swap(const uint16* val, uint16* result, int32 size, int32 steps = 16)
{
    int32 i = 0;
    steps = intrin_validate_steps((const byte*) val, steps);
    steps = intrin_validate_steps((const byte*) result, steps);

    #ifdef __AVX2__
        if (steps >= 8) {
            steps = 8;
            const __m256i mask_256 = _mm256_setr_epi8(
                1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14,
                17, 16, 19, 18, 21, 20, 23, 22, 25, 24, 27, 26, 29, 28, 31, 30
            );

            for (i = 0; i <= size - steps; i += steps) {
                __m256i vec = _mm256_load_si256((const __m256i *) (val + i));
                vec = _mm256_shuffle_epi8(vec, mask_256);

                _mm256_store_si256((__m256i *) (result + i), vec);
            }

            steps = 1;
        }
    #endif

    #ifdef __SSE4_2__
        if (steps >= 4) {
            steps = 4;
            const __m128i mask_128 = _mm_setr_epi8(
                1, 0, 3, 2, 5, 4, 7, 6, 9, 8, 11, 10, 13, 12, 15, 14
            );

            for (i = 0; i <= size - steps; i += steps) {
                __m128i vec = _mm_load_si128((const __m128i *) (val + i));
                vec = _mm_shuffle_epi8(vec, mask_128);

                _mm_store_si128((__m128i *) (result + i), vec);
            }
        }
    #endif

    for (; i < size; ++i) {
        uint16 v = ((uint16 *) val)[i];
        ((uint16 *) result)[i] = ((v << 8) | (v >> 8));
    }
}

#if _WIN32 || __LITTLE_ENDIAN__
    #define SWAP_ENDIAN_LITTLE_SIMD(val, result, size, steps) ((void)0)
    #define SWAP_ENDIAN_BIG_SIMD(val, result, size, steps) endian_swap((val), (result), (size), (steps))
#else
    #define SWAP_ENDIAN_LITTLE_SIMD(val, result, size, steps) endian_swap((val), (result), (size), (steps))
    #define SWAP_ENDIAN_BIG_SIMD(val, result, size, steps) ((void)0)
#endif

#endif

/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ARCHITECTURE_X86_UTILS_H
#define COMS_ARCHITECTURE_X86_UTILS_H

#include <stdlib.h>
#include "../../../../stdlib/Stdlib.h"
#include "../../../../compiler/CompilerUtils.h"
#include "../../../../stdlib/Simd.h"

// Only allowed for data >= 64 bits
bool is_empty(const byte* region, uint64 size, int32 steps = 8)
{
    // Quick check of first byte
    if (*region != 0) {
        return false;
    }

    const byte* const end = region + size;
    steps = intrin_validate_steps(region, steps);

    switch (steps) {
        #ifdef __AVX512F__
            case 16: {
                    while (region + 64 <= end) {
                        __m512i chunk = _mm512_load_si512((const __m512i *) region);
                        __mmask64 mask = _mm512_cmpeq_epi8_mask(chunk, _mm512_setzero_si512());
                        if (mask != 0xFFFFFFFFFFFFFFFF) {
                            return false;
                        }

                        region += 64;
                    }
                };
                FALLTHROUGH;
        #else
            case 16: FALLTHROUGH;
        #endif
        #ifdef __AVX2__
            case 8: {
                while (region + 32 <= end) {
                    __m256i chunk = _mm256_load_si256((const __m256i *) region);
                    if (!_mm256_testz_si256(chunk, chunk)) {
                        return false;
                    }

                    region += 32;
                }
            };
            FALLTHROUGH;
        #else
            case 8: FALLTHROUGH;
        #endif
        #ifdef __SSE4_2__
            case 4: {
                while (region + 16 <= end) {
                    __m128i chunk = _mm_load_si128((const __m128i *) region);
                    if (!_mm_testz_si128(chunk, chunk)) {
                        return false;
                    }

                    region += 16;
                }
            }
            FALLTHROUGH;
        #else
            case 4: FALLTHROUGH;
        #endif
        case 1: {
            while (region + 4 <= end) {
                if (*((const uint32_t *) region) != 0) {
                    return false;
                }

                region += 4;
            }

            while (region < end) {
                if (*region++ != 0) {
                    return false;
                }
            }
        } break;
        default:
            UNREACHABLE();
    }

    return true;
}

#endif
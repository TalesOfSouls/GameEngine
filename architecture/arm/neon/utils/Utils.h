/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ARCHITECTURE_ARM_NEON_UTILS_H
#define COMS_ARCHITECTURE_ARM_NEON_UTILS_H

#include <stdlib.h>
#include "../../../../stdlib/Types.h"
#include "../../../../compiler/CompilerUtils.h"
#include "../../../../stdlib/Simd.h"
#include <arm_neon.h>

// @question When do we want to use neon and when do we want to use sve?
// Only allowed for data >= 64 bits
bool is_empty(const uint8* region, uint64 size, int32 steps = 8) {
    if (*((uint64 *) region) != 0) {
        return false;
    }

    const uint8* end = region + size;
    steps = intrin_validate_steps(region, steps);

    switch (steps) {
        case 16: {
            while (region + 64 <= end) {
                uint8x16_t chunk1 = vld1q_u8(region);
                uint8x16_t chunk2 = vld1q_u8(region + 16);
                uint8x16_t chunk3 = vld1q_u8(region + 32);
                uint8x16_t chunk4 = vld1q_u8(region + 48);

                uint8x16_t zero = vdupq_n_u8(0);
                uint8x16_t result1 = vceqq_u8(chunk1, zero);
                uint8x16_t result2 = vceqq_u8(chunk2, zero);
                uint8x16_t result3 = vceqq_u8(chunk3, zero);
                uint8x16_t result4 = vceqq_u8(chunk4, zero);

                uint8x16_t combined = vandq_u8(vandq_u8(result1, result2), vandq_u8(result3, result4));

                if (vminvq_u8(combined) != 0xFF) {
                    return false;
                }

                region += 64;
            }
            break;
        }
        case 8: {
            while (region + 32 <= end) {
                uint8x16_t chunk1 = vld1q_u8(region);
                uint8x16_t chunk2 = vld1q_u8(region + 16);

                uint8x16_t zero = vdupq_n_u8(0);
                uint8x16_t result1 = vceqq_u8(chunk1, zero);
                uint8x16_t result2 = vceqq_u8(chunk2, zero);

                uint8x16_t combined = vandq_u8(result1, result2);

                if (vminvq_u8(combined) != 0xFF) {
                    return false;
                }

                region += 32;
            }
            break;
        }
        case 4: {
            while (region + 16 <= end) {
                uint8x16_t chunk = vld1q_u8(region);

                uint8x16_t zero = vdupq_n_u8(0);
                uint8x16_t result = vceqq_u8(chunk, zero);

                if (vminvq_u8(result) != 0xFF) {
                    return false;
                }

                region += 16;
            }
            break;
        }
        case 1: {
            while (region + 4 <= end) {
                if (*((const uint32 *) region) != 0) {
                    return false;
                }

                region += 4;
            }

            while (region < end) {
                if (*region++ != 0) {
                    return false;
                }
            }
            break;
        }
        default:
            UNREACHABLE();
    }

    return true;
}

#endif
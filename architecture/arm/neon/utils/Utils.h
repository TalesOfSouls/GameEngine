/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_ARCHITECTURE_ARM_NEON_UTILS_H
#define COMS_ARCHITECTURE_ARM_NEON_UTILS_H

#include "../../../../stdlib/Stdlib.h"
#include <arm_neon.h>

// @question When do we want to use neon and when do we want to use sve?
// Same structure as the x86 version (architecture/x86/simd/utils/Utils.h):
// the larger blocks fall through to the smaller ones -> every size is checked completely
// (vld1q has no alignment requirement, the steps only select the block size)
bool is_empty(const uint8* region, uint64 size, int32 steps = 8) NO_EXCEPT
{
    if (size == 0) {
        return true;
    }

    // Quick check of first byte (never reads behind the region, size can be < 8)
    if (*region != 0) {
        return false;
    }

    const uint8* const end = region + size;
    steps = intrin_validate_steps(region, steps);

    switch (steps) {
        case 16: {
            while (region + 64 <= end) {
                const uint8x16_t combined = vorrq_u8(
                    vorrq_u8(vld1q_u8(region), vld1q_u8(region + 16)),
                    vorrq_u8(vld1q_u8(region + 32), vld1q_u8(region + 48))
                );

                if (vmaxvq_u8(combined) != 0) {
                    return false;
                }

                region += 64;
            }
        }
        FALLTHROUGH;
        case 8: {
            while (region + 32 <= end) {
                const uint8x16_t combined = vorrq_u8(vld1q_u8(region), vld1q_u8(region + 16));

                if (vmaxvq_u8(combined) != 0) {
                    return false;
                }

                region += 32;
            }
        }
        FALLTHROUGH;
        case 4: {
            while (region + 16 <= end) {
                if (vmaxvq_u8(vld1q_u8(region)) != 0) {
                    return false;
                }

                region += 16;
            }
        }
        FALLTHROUGH;
        case 1: {
            while (region + 4 <= end) {
                // region may be unaligned, a load through a misaligned pointer is undefined behavior
                if (compiler_load_unaligned<uint32>(region) != 0) {
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

/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ARCHITECTURE_ARM_SVE_UTILS_H
#define COMS_ARCHITECTURE_ARM_SVE_UTILS_H

#include <stdlib.h>
#include "../../../../stdlib/Types.h"
#include "../../../../compiler/CompilerUtils.h"
#include "../../../../stdlib/Simd.h"
#include <arm_sve.h>

// Only allowed for data >= 64 bits
bool is_empty(const uint8* region, uint64 size, int32 steps = 8) {
    if (*((uint64 *) region) != 0) {
        return false;
    }

    const uint8* end = region + size;
    steps = intrin_validate_steps(region, steps);

    uint64 sve_vector_bytes = svcntb();

    switch (steps) {
        case 16: {
            while (region + sve_vector_bytes * 4 <= end) {
                svuint8_t chunk1 = svld1_u8(svptrue_b8(), region);
                svuint8_t chunk2 = svld1_u8(svptrue_b8(), region + sve_vector_bytes);
                svuint8_t chunk3 = svld1_u8(svptrue_b8(), region + sve_vector_bytes * 2);
                svuint8_t chunk4 = svld1_u8(svptrue_b8(), region + sve_vector_bytes * 3);

                svuint8_t zero = svdup_n_u8(0);
                svbool_t cmp1 = svcmpeq_u8(svptrue_b8(), chunk1, zero);
                svbool_t cmp2 = svcmpeq_u8(svptrue_b8(), chunk2, zero);
                svbool_t cmp3 = svcmpeq_u8(svptrue_b8(), chunk3, zero);
                svbool_t cmp4 = svcmpeq_u8(svptrue_b8(), chunk4, zero);

                svbool_t combined = svand_b_z(svptrue_b8(), svand_b_z(svptrue_b8(), cmp1, cmp2), svand_b_z(svptrue_b8(), cmp3, cmp4));

                if (!svptest_any(svptrue_b8(), combined)) {
                    return false;
                }

                region += sve_vector_bytes * 4;
            }
            break;
        }
        case 8: {
            while (region + sve_vector_bytes * 2 <= end) {
                svuint8_t chunk1 = svld1_u8(svptrue_b8(), region);
                svuint8_t chunk2 = svld1_u8(svptrue_b8(), region + sve_vector_bytes);

                svuint8_t zero = svdup_n_u8(0);
                svbool_t cmp1 = svcmpeq_u8(svptrue_b8(), chunk1, zero);
                svbool_t cmp2 = svcmpeq_u8(svptrue_b8(), chunk2, zero);

                svbool_t combined = svand_b_z(svptrue_b8(), cmp1, cmp2);

                if (!svptest_any(svptrue_b8(), combined)) {
                    return false;
                }

                region += sve_vector_bytes * 2;
            }
            break;
        }
        case 4: {
            while (region + sve_vector_bytes <= end) {
                svuint8_t chunk = svld1_u8(svptrue_b8(), region);

                svuint8_t zero = svdup_n_u8(0);
                svbool_t cmp = svcmpeq_u8(svptrue_b8(), chunk, zero);

                if (!svptest_any(svptrue_b8(), cmp)) {
                    return false;
                }

                region += sve_vector_bytes;
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
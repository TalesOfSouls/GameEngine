/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_H
#define COMS_STDLIB_SIMD_H

#include "../utils/TestUtils.h"

// Adjusts the step size based on the memory alignment
inline
int32 intrin_validate_steps(const byte* mem, int32 steps) {
    if (steps >= 16 && ((uintptr_t) mem & 63) == 0) {
        return 16;
    } else if (steps >= 8 && ((uintptr_t) mem & 31) == 0) {
        return 8;
    } else if (steps >= 4 && ((uintptr_t) mem & 15) == 0) {
        return 4;
    } else {
        return 1;
    }
}

#if __aarch64__
    #include <arm_neon.h>
#else
    #include "../architecture/x86/simd/SIMD_F32.h"
    #include "../architecture/x86/simd/SIMD_F64.h"
    #include "../architecture/x86/simd/SIMD_I8.h"
    #include "../architecture/x86/simd/SIMD_I16.h"
    #include "../architecture/x86/simd/SIMD_I32.h"
    #include "../architecture/x86/simd/SIMD_I64.h"
    #include "../architecture/x86/simd/SIMD_SVML.h"
#endif

#endif
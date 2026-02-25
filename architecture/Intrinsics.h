/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ARCHITECTURE_INTRINSICS_H
#define COMS_ARCHITECTURE_INTRINSICS_H

#include "../stdlib/Types.h"

// This defines the maximum potential simd step size / vector width which could be even lower based on the runtime feature check
#if defined(__AVX512F__) || defined(__ARM_FEATURE_SVE)
    #define SIMD_POTENTIAL_STEP 16
#elif defined(__AVX2__)
    #define SIMD_POTENTIAL_STEP 8
#elif defined(__SSE4_2__) || defined(__ARM_NEON)
    #define SIMD_POTENTIAL_STEP 4
#else
    #define SIMD_POTENTIAL_STEP 1
#endif

/**
 * Returns the maximum SIMD vector width based on the compile time information and runtime check you have to provide (see CpuInfo)
 */
#define SIMD_MAX_STEP_SIZE(x) OMS_MIN((x), SIMD_POTENTIAL_STEP)

// Adjusts the step size based on the memory alignment
inline
int32 intrin_validate_steps(const byte* mem, int32 steps) NO_EXCEPT
{
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

#ifdef __aarch64__
    #include "arm/Intrinsics.h"
#else
    #include "x86/Intrinsics.h"
#endif

#endif
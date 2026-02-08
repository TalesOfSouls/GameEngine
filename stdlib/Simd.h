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

#include "../utils/Assert.h"

/**
 * WARNING: On Windows you can compile with all SIMD features even if the target machine doesn't support them.
 *              Therefore, if you are on Windows you should always compile with "all" SIMD features
 *          On Linux this would cause problems.
 *              Therefore, only enable the SIMD features that are available
 */

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

#ifdef __aarch64__
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
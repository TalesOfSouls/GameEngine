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

#if defined(__AVX512F__) || defined(__ARM_FEATURE_SVE)
    #define SIMD_POTENTIAL_STEP 16
#elif defined(__AVX2__)
    #define SIMD_POTENTIAL_STEP 8
#elif defined(__SSE4_2__) || defined(__ARM_NEON)
    #define SIMD_POTENTIAL_STEP 4
#else
    #define SIMD_POTENTIAL_STEP 1
#endif

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
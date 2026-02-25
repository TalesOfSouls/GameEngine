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
#include "../architecture/Intrinsics.h"

/**
 * WARNING: On Windows you can compile with all SIMD features even if the target machine doesn't support them.
 *              Therefore, if you are on Windows you should always compile with "all" SIMD features
 *          On Linux this would cause problems.
 *              Therefore, only enable the SIMD features that are available
 */

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
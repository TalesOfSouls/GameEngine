/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_F64_H
#define COMS_STDLIB_SIMD_F64_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Types.h"

#ifdef __SSE4_2__
    #include "SIMD_F64_SSE.h"
#endif

#ifdef __AVX2__
    #include "SIMD_F64_AVX2.h"
#endif

#ifdef __AVX512F__
    #include "SIMD_F64_AVX512.h"
#endif

#endif
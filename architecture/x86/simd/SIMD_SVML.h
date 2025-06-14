/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_SVML_H
#define COMS_STDLIB_SIMD_SVML_H

#ifdef __SSE4_2__
    #include "SIMD_SVML_SSE.h"
#endif

#ifdef __AVX2__
    #include "SIMD_SVML_AVX2.h"
#endif

#ifdef __AVX512F__
    #include "SIMD_SVML_AVX512.h"
#endif

#endif
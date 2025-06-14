/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_F64_AVX2_H
#define COMS_STDLIB_SIMD_F64_AVX2_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Types.h"

struct f64_4 {
    union {
        #if ARM
            svfloat64_t s;
        #else
            __m256 s;
        #endif

        f64 v[4];
    };
};


#endif
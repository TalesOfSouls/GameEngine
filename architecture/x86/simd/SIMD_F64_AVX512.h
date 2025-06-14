/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_F64_AVX512_H
#define COMS_STDLIB_SIMD_F64_AVX512_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Types.h"

struct f64_8 {
    union {
        #if ARM
            svfloat64_t s;
        #else
            __m512 s;
        #endif

        f64 v[8];
    };
};

#endif
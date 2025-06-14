/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_I64_AVX512_H
#define COMS_STDLIB_SIMD_I64_AVX512_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Types.h"

struct int64_8 {
    union {
        #if ARM
            svint64_t s;
        #else
            __m512i s;
        #endif

        int64 v[8];
    };
};

#endif
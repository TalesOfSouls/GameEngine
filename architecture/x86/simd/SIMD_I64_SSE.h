/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_SIMD_I64_SSE_H
#define COMS_STDLIB_SIMD_I64_SSE_H

#include <immintrin.h>
#include <xmmintrin.h>

#include "../../../stdlib/Types.h"

struct int64_2 {
    union {
        #if ARM
            svint64_t s;
        #else
            __m128i s;
        #endif

        int64 v[2];
    };
};

#endif
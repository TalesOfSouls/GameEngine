/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_UTILS_MATH_UTILS_H
#define COMS_UTILS_MATH_UTILS_H

#include "../stdlib/Stdlib.h"
#include "../utils/Assert.h"

// WARNING: Don't use any of these functions yet. They are too imprecise and too slow
//          Compilers use internal intrinsics based on input value at compile time to use the fastest possible solutions

inline
f64 factorial(int32 n) NO_EXCEPT
{
    f64 result = 1.0;
    for (int32 i = 1; i <= n; ++i) {
        result *= i;
    }

    return result;
}

inline
f32 rsqrt_approx(f32 a) NO_EXCEPT
{
    ASSERT_TRUE(a >= 0);

    // Initial guess using magic number (Quake III hack)
    f32 x = a;
    uint32 i = *((uint32 *) &x);
    i = 0x5F3759DF - (i >> 1); // Magic number for initial guess
    x = *(f32 *) &i;

    // Newton-Raphson iterations
    x = x * (1.5f - 0.5f * a * x * x);
    x = x * (1.5f - 0.5f * a * x * x);
    x = x * (1.5f - 0.5f * a * x * x);

    return x;
}

inline
f64 rsqrt_approx(f64 a) NO_EXCEPT
{
    ASSERT_TRUE(a >= 0);

    // Initial guess using magic number (Quake III hack)
    f64 x = a;
    uint64 i = *((uint64 *) &x);
    i = 0x5fe6eb50c7b537a9 - (i >> 1); // Magic number for initial guess
    x = *(f64 *) &i;

    // Newton-Raphson iterations
    x = x * (1.5 - 0.5 * a * x * x);
    x = x * (1.5 - 0.5 * a * x * x);
    x = x * (1.5 - 0.5 * a * x * x);

    return x;
}

#endif

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

#include "../stdlib/Types.h"
#include "../utils/TestUtils.h"

// WARNING: Don't use any of these functions yet. They are too imprecise and too slow

inline
f64 factorial(int32 n) {
    f64 result = 1.0;
    for (int32 i = 1; i <= n; ++i) {
        result *= i;
    }

    return result;
}

inline
f32 sin_approx(f32 x) {
    // Normalize x to the range [-π, π] for better accuracy
    while (x > OMS_PI) {
        x -= OMS_TWO_PI;
    }

    while (x < -OMS_PI) {
        x += OMS_TWO_PI;
    }

    f32 x2 = x * x;
    return x * (1.0f + x2 * (-1.0f / 6.0f + x2 * (1.0f / 120.0f + x2 * (-1.0f / 5040.0f + x2 * (1.0f / 362880.0f)))));
}

inline
f32 cos_approx(f32 x) {
    return sin_approx(OMS_PI_OVER_TWO - x);
}

inline
f32 tan_approx(f32 x) {
    return sin_approx(x) / cos_approx(x);
}

inline
f32 asin_approx(f32 x) {
    // Undefined for |x| > 1
    ASSERT_SIMPLE(x >= -1.0f && x <= 1.0f);

    f32 result = x;
    f32 term = x;
    for (int32 i = 1; i <= 6; ++i) {
        term *= x * x * (2 * i - 1) * (2 * i - 1) / ((2 * i) * (2 * i + 1));
        result += term;
    }

    return result;
}

inline
f32 acos_approx(f32 x) {
    // π/2 - asin_approx(x)
    return OMS_PI_OVER_TWO - asin_approx(x);
}

inline
f32 atan_approx(f32 x) {
    if (x > 1.0f) {
        // π/2 - atan_approx(1/x)
        return OMS_PI_OVER_TWO - atan_approx(1.0f / x);
    } else if (x < -1.0f) {
        // -π/2 - atan_approx(1/x)
        return -OMS_PI_OVER_TWO - atan_approx(1.0f / x);
    }

    f32 result = x;
    f32 term = x;
    for (int32 i = 1; i <= 6; ++i) {
        term *= -x * x;
        result += term / (2.0f * i + 1);
    }

    return result;
}

inline
f32 sqrt_approx(f32 a) {
    ASSERT_SIMPLE(a >= 0);

    int32_t i = *(int32_t*)&a;
    // Magic number for initial guess
    i = 0x1FBD1DF5 + (i >> 1);
    float x = *(float*)&i;

    // Newton-Raphson iterations
    x = 0.5f * (x + a / x);
    x = 0.5f * (x + a / x);
    x = 0.5f * (x + a / x);

    return x;
}

inline
f32 rsqrt_approx(f32 a) {
    ASSERT_SIMPLE(a >= 0);

    // Initial guess using magic number (Quake III hack)
    f32 x = a;
    uint32 i = *(uint32 *)&x;
    i = 0x5F3759DF - (i >> 1); // Magic number for initial guess
    x = *(f32 *) &i;

    // Newton-Raphson iterations
    x = x * (1.5f - 0.5f * a * x * x);
    x = x * (1.5f - 0.5f * a * x * x);
    x = x * (1.5f - 0.5f * a * x * x);

    return x;
}

inline
f32 exp_approx(f32 x) {
    // Range reduction: e^x = e^(x / n)^n
    const int32 n = 8;
    x /= n;

    // Taylor series approximation for e^x
    f32 result = 1.0f;
    f32 term = 1.0f;
    for (int32 i = 1; i <= 10; ++i) {
        term *= x / i;
        result += term;
    }

    // Raise to the nth power
    f32 final_result = result;
    for (int32 i = 1; i < n; ++i) {
        final_result *= result;
    }

    return final_result;
}

inline
f32 log_approx(f32 x) {
    ASSERT_SIMPLE(x > 0);

    // Polynomial approximation
    f32 y = (x - 1) / (x + 1);
    f32 y2 = y * y;
    f32 result = y * (1.0f + y2 * (1.0f / 3.0f + y2 * (1.0f / 5.0f + y2 * (1.0f / 7.0f))));

    return 2.0f * result;
}

inline
f32 pow_approx(f32 a, f32 b) {
    if (a == 0.0f) {
        return 0.0f;
    }

    return exp_approx(b * log_approx(a));
}

////////////////////////////////////////////////////////////////

inline
f64 sin_approx(f64 x) {
    // Normalize x to the range [-π, π] for better accuracy
    while (x > OMS_PI) {
        x -= OMS_TWO_PI;
    }

    while (x < -OMS_PI) {
        x += OMS_TWO_PI;
    }

    f64 x2 = x * x;
    return x * (1.0 + x2 * (-1.0 / 6.0 + x2 * (1.0 / 120.0 + x2 * (-1.0 / 5040.0 + x2 * (1.0 / 362880.0)))));
}

inline
f64 cos_approx(f64 x) {
    return sin_approx(OMS_PI_OVER_TWO - x);
}

inline
f64 tan_approx(f64 x) {
    return sin_approx(x) / cos_approx(x);
}

inline
f64 asin_approx(f64 x) {
    // Undefined for |x| > 1
    ASSERT_SIMPLE(x >= -1.0 && x <= 1.0);

    f64 result = x;
    f64 term = x;
    for (int32 i = 1; i <= 6; ++i) {
        term *= x * x * (2 * i - 1) * (2 * i - 1) / ((2 * i) * (2 * i + 1));
        result += term;
    }

    return result;
}

inline
f64 acos_approx(f64 x) {
    // π/2 - asin_approx(x)
    return OMS_PI_OVER_TWO - asin_approx(x);
}

inline
f64 atan_approx(f64 x) {
    if (x > 1.0) {
        // π/2 - atan_approx(1/x)
        return OMS_PI_OVER_TWO - atan_approx(1.0 / x);
    } else if (x < -1.0) {
        // -π/2 - atan_approx(1/x)
        return -OMS_PI_OVER_TWO - atan_approx(1.0 / x);
    }

    f64 result = x;
    f64 term = x;
    for (int32 i = 1; i <= 6; ++i) {
        term *= -x * x;
        result += term / (2 * i + 1);
    }

    return result;
}

inline
f64 sqrt_approx(f64 a) {
    ASSERT_SIMPLE(a >= 0);

    int64_t i = *(int64_t*)&a;
    // Magic number for initial guess
    i = 0x1FF7A3BEA91D9B1B + (i >> 1);
    f64 x = *(f64*)&i;

    // Newton-Raphson iterations
    x = 0.5 * (x + a / x);
    x = 0.5 * (x + a / x);
    x = 0.5 * (x + a / x);

    return x;
}

inline
f64 rsqrt_approx(f64 a) {
    ASSERT_SIMPLE(a >= 0);

    // Initial guess using magic number (Quake III hack)
    f64 x = a;
    uint64 i = *(uint64 *)&x;
    i = 0x5fe6eb50c7b537a9 - (i >> 1); // Magic number for initial guess
    x = *(f64 *) &i;

    // Newton-Raphson iterations
    x = x * (1.5 - 0.5 * a * x * x);
    x = x * (1.5 - 0.5 * a * x * x);
    x = x * (1.5 - 0.5 * a * x * x);

    return x;
}

inline
f64 exp_approx(f64 x) {
    // Range reduction: e^x = e^(x / n)^n
    const int32 n = 8;
    x /= n;

    // Taylor series approximation for e^x
    f64 result = 1.0;
    f64 term = 1.0;
    for (int32 i = 1; i <= 10; ++i) {
        term *= x / i;
        result += term;
    }

    // Raise to the nth power
    f64 final_result = 1.0;
    for (int32 i = 0; i < n; ++i) {
        final_result *= result;
    }

    return final_result;
}

inline
f64 log_approx(f64 x) {
    ASSERT_SIMPLE(x > 0);

    // Polynomial approximation
    f64 y = (x - 1) / (x + 1);
    f64 y2 = y * y;
    f64 result = y * (1.0 + y2 * (1.0 / 3.0 + y2 * (1.0 / 5.0 + y2 * (1.0 / 7.0))));

    return 2.0 * result;
}

inline
f64 pow_approx(f64 a, f64 b) {
    if (a == 0.0) {
        return 0.0;
    }

    return exp_approx(b * log_approx(a));
}

#endif

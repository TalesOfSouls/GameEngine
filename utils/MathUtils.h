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

// Only works on [-pi; pi] and then comes close to sinf in terms of performance
inline
f32 sin_approx(f32 x) NO_EXCEPT
{
    // Parabolic approximation
    const f32 B = 4.0f / OMS_PI_F32;
    const f32 C = -4.0f / (OMS_PI_F32 * OMS_PI_F32);

    f32 y = B * x + C * x * oms_abs(x);

    // correction factor
    const f32 P = 0.225f;
    y = P * (y * oms_abs(y) - y) + y;

    return y;
}

FORCE_INLINE
f32 cos_approx(f32 x) NO_EXCEPT
{
    return sin_approx(OMS_PI_OVER_TWO_F32 - x);
}

inline
void sincos_approx(f32 x, f32* s, f32* c) NO_EXCEPT
{
    const f32 B = 4.0f / OMS_PI_F32;
    const f32 C = -4.0f / (OMS_PI_F32 * OMS_PI_F32);

    // correction factor
    const f32 P = 0.225f;

    // sine
    f32 y_sin = B * x + C * x * oms_abs(x);
    y_sin = P * (y_sin * oms_abs(y_sin) - y_sin) + y_sin;

    *s = y_sin;

    // cosine using sin(x + pi/2) trick
    // cos(x) ≈ sin(x + pi/2)
    f32 xp = x + OMS_PI_OVER_TWO_F32;

    // Wrap xp back into [-pi, pi]
    if (xp > OMS_PI_F32) {
        xp -= OMS_TWO_PI_F32;
    }

    f32 y_cos = B * xp + C * xp * oms_abs(xp);
    y_cos = P * (y_cos * oms_abs(y_cos) - y_cos) + y_cos;

    *c = y_cos;
}

FORCE_INLINE
f32 tan_approx(f32 x) NO_EXCEPT
{
    return sin_approx(x) / cos_approx(x);
}

inline
f32 asin_approx(f32 x) NO_EXCEPT
{
    // Undefined for |x| > 1
    ASSERT_TRUE(x >= -1.0f && x <= 1.0f);

    f32 result = x;
    f32 term = x;
    for (int32 i = 1; i <= 6; ++i) {
        term *= x * x * (2 * i - 1) * (2 * i - 1) / ((2 * i) * (2 * i + 1));
        result += term;
    }

    return result;
}

FORCE_INLINE
f32 acos_approx(f32 x) NO_EXCEPT
{
    // π/2 - asin_approx(x)
    return OMS_PI_OVER_TWO_F32 - asin_approx(x);
}

inline
f32 atan_approx(f32 x) NO_EXCEPT
{
    if (x > 1.0f) {
        // π/2 - atan_approx(1/x)
        return OMS_PI_OVER_TWO_F32 - atan_approx(1.0f / x);
    } else if (x < -1.0f) {
        // -π/2 - atan_approx(1/x)
        return -OMS_PI_OVER_TWO_F32 - atan_approx(1.0f / x);
    }

    f32 result = x;
    f32 term = x;
    for (int32 i = 1; i <= 501; ++i) {
        term *= -x * x;
        result += term / (2.0f * i + 1);
    }

    return result;
}

inline
f32 sqrt_approx(f32 a) NO_EXCEPT
{
    ASSERT_TRUE(a >= 0);

    int32 i = *((int32*) &a);
    // Magic number for initial guess
    i = 0x1FBD1DF5 + (i >> 1);
    f32 x = *(f32*)&i;

    // Newton-Raphson iterations
    x = 0.5f * (x + a / x);
    x = 0.5f * (x + a / x);
    x = 0.5f * (x + a / x);

    return x;
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
f32 exp_approx(f32 x) NO_EXCEPT
{
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
f32 log_approx(f32 x) NO_EXCEPT
{
    ASSERT_TRUE(x > 0);

    f32_bits v = { x };

    // Extract exponent
    int e = ((v.u >> 23) & 0xFF) - 127;

    // Force mantissa into [1,2)
    v.u = (v.u & 0x007FFFFF) | 0x3F800000;
    f32 m = v.f;

    // y = (m - 1) / (m + 1), |y| <= ~0.1716
    const f32 y  = (m - 1.0f) / (m + 1.0f);
    const f32 y2 = y * y;

    f32 poly = 1.0f +
        y2 * (1.0f / 3.0f +
        y2 * (1.0f / 5.0f +
        y2 * (1.0f / 7.0f +
        y2 * (1.0f / 9.0f +
        y2 * (1.0f / 11.0f)))));

    const f32 ln_m = 2.0f * y * poly;

    // ln(2)
    return ln_m + (f32)e * 0.6931471805599453f;
}

inline
f32 pow_approx(f32 a, f32 b) NO_EXCEPT
{
    if (a == 0.0f) {
        return 0.0f;
    }

    return exp_approx(b * log_approx(a));
}

////////////////////////////////////////////////////////////////

inline
f64 sin_approx(f64 x) NO_EXCEPT
{
    // Parabolic approximation
    const f64 B = 4.0 / OMS_PI_F64;
    const f64 C = -4.0 / (OMS_PI_F64 * OMS_PI_F64);

    f64 y = B * x + C * x * oms_abs(x);

    // Correction
    const f64 P = 0.225f;
    y = P * (y * oms_abs(y) - y) + y;

    return y;
}

FORCE_INLINE
f64 cos_approx(f64 x) NO_EXCEPT
{
    return sin_approx(OMS_PI_OVER_TWO_F64 - x);
}

FORCE_INLINE
f64 tan_approx(f64 x) NO_EXCEPT
{
    return sin_approx(x) / cos_approx(x);
}

inline
f64 asin_approx(f64 x) NO_EXCEPT
{
    // Undefined for |x| > 1
    ASSERT_TRUE(x >= -1.0 && x <= 1.0);

    f64 result = x;
    f64 term = x;
    for (int32 i = 1; i <= 6; ++i) {
        term *= x * x * (2 * i - 1) * (2 * i - 1) / ((2 * i) * (2 * i + 1));
        result += term;
    }

    return result;
}

FORCE_INLINE
f64 acos_approx(f64 x) NO_EXCEPT
{
    // π/2 - asin_approx(x)
    return OMS_PI_OVER_TWO_F64 - asin_approx(x);
}

inline
f64 atan_approx(f64 x) NO_EXCEPT
{
    if (x > 1.0) {
        // π/2 - atan_approx(1/x)
        return OMS_PI_OVER_TWO_F64 - atan_approx(1.0 / x);
    } else if (x < -1.0) {
        // -π/2 - atan_approx(1/x)
        return -OMS_PI_OVER_TWO_F64 - atan_approx(1.0 / x);
    }

    f64 result = x;
    f64 term = x;
    for (int32 i = 1; i <= 501; ++i) {
        term *= -x * x;
        result += term / (2 * i + 1);
    }

    return result;
}

inline
f64 sqrt_approx(f64 a) NO_EXCEPT
{
    ASSERT_TRUE(a >= 0);

    int64 i = *((int64 *) &a);
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

inline
f64 exp_approx(f64 x) NO_EXCEPT
{
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
f64 log_approx(f64 x) NO_EXCEPT
{
    ASSERT_TRUE(x > 0);

    f64_bits v = { x };

    // Extract exponent (11 bits, bias 1023)
    int64 e = ((v.u >> 52) & 0x7FF) - 1023;

    // Force mantissa into [1,2)
    v.u = (v.u & 0x000FFFFFFFFFFFFFULL) | 0x3FF0000000000000ULL;
    f64 m = v.f;

    // y = (m - 1) / (m + 1), |y| <= ~0.1716
    const f64 y  = (m - 1.0) / (m + 1.0);
    const f64 y2 = y * y;

    f64 poly = 1.0 +
        y2 * (1.0 / 3.0 +
        y2 * (1.0 / 5.0 +
        y2 * (1.0 / 7.0 +
        y2 * (1.0 / 9.0 +
        y2 * (1.0 / 11.0)))));

    const f64 ln_m = 2.0 * y * poly;

    // ln(2)
    return ln_m + (f64)e * 0.6931471805599453;
}

inline
f64 pow_approx(f64 a, f64 b) NO_EXCEPT
{
    if (a == 0.0) {
        return 0.0;
    }

    return exp_approx(b * log_approx(a));
}

#endif

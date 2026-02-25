/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_MATH_INTERNAL_H
#define COMS_STDLIB_MATH_INTERNAL_H

#include "Types.h"
#include "Defines.h"
#include "../compiler/CompilerUtils.h"

// Limits
// You should only use these limit functions when you absolutely know which version to use.
// It is much better that you use the general purpose functions defined in Platform.h,
// which in return use these functions based on best match for the used compiler and platform
template <typename T> FORCE_INLINE
CONSTEXPR T max_branched(T a, T b) {
    return (a > b) ? a : b;
}

template <typename T> FORCE_INLINE
CONSTEXPR T min_branched(T a, T b) {
    return (a > b) ? b : a;
}

template <typename T> FORCE_INLINE
CONSTEXPR T clamp_branched(T val, T low, T high) {
    return (val < low) ? low : ((val > high) ? high : val);
}

// The branchless versions only work for int types
template <typename T> FORCE_INLINE
CONSTEXPR T max_branchless(T a, T b) {
    T mask = T(0) - T(a < b);
    return a ^ ((a ^ b) & mask);
}

template <typename T> FORCE_INLINE
CONSTEXPR T min_branchless(T a, T b) {
    T mask = T(0) - T(a < b);
    return b ^ ((a ^ b) & mask);
}

template <typename T> FORCE_INLINE
CONSTEXPR T clamp_branchless(T val, T low, T high) {
    T maskLow  = T(0) - T(val < low);
    T temp = val ^ ((val ^ low) & maskLow);

    T maskHigh = T(0) - T(temp < high);
    T result = high ^ ((temp ^ high) & maskHigh);

    return result;
}

// WARNING: May overflow for ints
template <typename T>
inline T max_branchless_general(T a, T b) NO_EXCEPT
{
    return a + (b - a) * (b > a);
}

// WARNING: May overflow for ints
template <typename T>
inline T min_branchless_general(T a, T b) NO_EXCEPT
{
    return a + (b - a) * (b < a);
}

// WARNING: May overflow for ints
template <typename T>
inline T clamp_branchless_general(T v, T lo, T hi) NO_EXCEPT
{
    T t = v + (hi - v) * (v > hi);

    return lo + (t - lo) * (t > lo);
}

// Abs
FORCE_INLINE
int8 __internal_abs(int8 a) NO_EXCEPT
{
    uint8 ua = (uint8)a;
    uint8 mask = ua >> 7;
    return (int8)((ua ^ mask) - mask);
}

FORCE_INLINE
int16 __internal_abs(int16 a) NO_EXCEPT
{
    uint16 ua = (uint16)a;
    uint16 mask = ua >> 15;
    return (int16)((ua ^ mask) - mask);
}

FORCE_INLINE
int32 __internal_abs(int32 a) NO_EXCEPT
{
    uint32 ua = (uint32)a;
    uint32 mask = ua >> 31;
    return (int32)((ua ^ mask) - mask);
}

FORCE_INLINE
int64 __internal_abs(int64 a) NO_EXCEPT
{
    uint64 ua = (uint64)a;
    uint64 mask = ua >> 63;
    return (int64)((ua ^ mask) - mask);
}

// For floats the high bit is still defining the sign
// But we need to reinterpret it as int to mask the sign
// WARNING: This is only faster on msvc
FORCE_INLINE
f32 __internal_abs(f32 a) NO_EXCEPT
{
    union { f32 f; uint32 i; } u;
    u.f = a;
    u.i &= 0x7FFFFFFF;
    return u.f;
}

FORCE_INLINE
f64 __internal_abs(f64 a) NO_EXCEPT
{
    union { f64 f; uint64 i; } u;
    u.f = a;
    u.i &= 0x7FFFFFFFFFFFFFFF;
    return u.f;
}

// GCC seems to heavily optimize this making the above functions redundant but MSVC doesn't
/*
FORCE_INLINE template <typename T>
T __internal_abs(T a) NO_EXCEPT {
    return (a > (T)0) ? a : (T)(-a);
}
*/

// Rounding
FORCE_INLINE CONSTEXPR
int32 oms_round_positive(f32 x) NO_EXCEPT {
    return (int32) (x + 0.5f);
}

FORCE_INLINE CONSTEXPR
int64 oms_round_positive(f64 x) NO_EXCEPT {
    return (int64) (x + 0.5);
}

template <typename T>
FORCE_INLINE CONSTEXPR
T __internal_round(T x) NO_EXCEPT {
    return (x >= (T) 0)
        ? (T)((int32)(x + 0.5f))
        : (T)((int32)(x - 0.5f));
}

template <typename T>
FORCE_INLINE CONSTEXPR
T ceil_div(T a, T b) NO_EXCEPT
{ return (a + b - 1) / b; }

template <typename F>
FORCE_INLINE CONSTEXPR
F __internal_ceil(F x) NO_EXCEPT
{
    uint64 xi = (uint64)x;
    if (x == (F)xi) {
        return xi;
    }

    return (x > (F) 0) ? (F) (xi + 1) : xi;
}

template <typename F>
FORCE_INLINE CONSTEXPR
F __internal_floor(F x) NO_EXCEPT {
    /*
    int32 xi = (int32) x;
    bool adjust = (x < 0.0f) && (x != (f32)(xi));

    return (f32)(xi - (int32)(adjust));
    */

    int32 xi = (int32)x;
    return (F)(xi - ((x < (F)0.0 && x != (F)xi) ? 1 : 0));
}

// Fast integer division and floor, IFF the divisor **is positive**
// (= (int) floorf((float)a/(float)b))
// This is required because -7 / 3 = -2 with normal int division, but we want -3
// However, 7 / 3 = 2 is what we would expect
template <typename T>
FORCE_INLINE
T floor_div(T a, T b) NO_EXCEPT
{
    T q = a / b;
    T r = a - q * b;
    return q - (r < 0);
}

// Trig
FORCE_INLINE
f32 deg2rad(f32 angle) NO_EXCEPT
{ return angle * OMS_PI_F32 / 180.0f; }

FORCE_INLINE
f32 rad2deg(f32 angle) NO_EXCEPT
{ return angle * 180.0f / OMS_PI_F32; }

// -pi / pi
FORCE_INLINE
f32 normalize_rad(f32 angle) NO_EXCEPT {
    return angle - OMS_TAU_F32 * __internal_floor((angle + OMS_PI_F32) / OMS_TAU_F32);
}

// 0 / 360
FORCE_INLINE
f32 normalize_deg(f32 angle) NO_EXCEPT {
    return angle - 360.0f * __internal_floor(angle / 360.0f);
}

// Only works on [-pi; pi] and then comes close to sinf in terms of performance
inline
f32 sin_approx(f32 x) NO_EXCEPT
{
    // Parabolic approximation
    const f32 B = 4.0f / OMS_PI_F32;
    const f32 C = -4.0f / (OMS_PI_F32 * OMS_PI_F32);

    f32 y = B * x + C * x * __internal_abs(x);

    // correction factor
    const f32 P = 0.225f;
    y = P * (y * __internal_abs(y) - y) + y;

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
    f32 y_sin = B * x + C * x * __internal_abs(x);
    y_sin = P * (y_sin * __internal_abs(y_sin) - y_sin) + y_sin;

    *s = y_sin;

    // cosine using sin(x + pi/2) trick
    // cos(x) ≈ sin(x + pi/2)
    f32 xp = x + OMS_PI_OVER_TWO_F32;

    // Wrap xp back into [-pi, pi]
    if (xp > OMS_PI_F32) {
        xp -= OMS_TWO_PI_F32;
    }

    f32 y_cos = B * xp + C * xp * __internal_abs(xp);
    y_cos = P * (y_cos * __internal_abs(y_cos) - y_cos) + y_cos;

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
    for (int i = 1; i < n; ++i) {
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

    f64 y = B * x + C * x * __internal_abs(x);

    // Correction
    const f64 P = 0.225f;
    y = P * (y * __internal_abs(y) - y) + y;

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
f64 exp_approx(f64 x) NO_EXCEPT
{
    // Range reduction: e^x = e^(x / n)^n
    const int n = 8;
    x /= n;

    // Taylor series approximation for e^x
    f64 result = 1.0;
    f64 term = 1.0;
    for (int i = 1; i <= 10; ++i) {
        term *= x / i;
        result += term;
    }

    // Raise to the nth power
    f64 final_result = 1.0;
    for (int i = 0; i < n; ++i) {
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
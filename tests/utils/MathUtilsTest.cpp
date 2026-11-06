#include "../TestFramework.h"
#include "../../utils/MathUtils.h"
#include <math.h>

/**
 * Tests for the math utilities (@see math_internal.h and MathUtils.h)
 */

static void test_min_max() {
    TEST_EQUALS(max_branched((int32) 1, (int32) 2), 2);
    TEST_EQUALS(max_branched((int32) 2, (int32) 1), 2);
    TEST_EQUALS(min_branched((int32) 1, (int32) 2), 1);
    TEST_EQUALS(min_branched((int32) 2, (int32) 1), 1);

    TEST_EQUALS(max_branchless((int32) 1, (int32) 2), 2);
    TEST_EQUALS(max_branchless((int32) 2, (int32) 1), 2);
    TEST_EQUALS(min_branchless((int32) 1, (int32) 2), 1);
    TEST_EQUALS(min_branchless((int32) 2, (int32) 1), 1);

    TEST_EQUALS(clamp_branched((int32) 5, (int32) 0, (int32) 10), 5);
    TEST_EQUALS(clamp_branched((int32) -5, (int32) 0, (int32) 10), 0);
    TEST_EQUALS(clamp_branched((int32) 15, (int32) 0, (int32) 10), 10);

    TEST_EQUALS(clamp_branchless((int32) 5, (int32) 0, (int32) 10), 5);
    TEST_EQUALS(clamp_branchless((int32) -5, (int32) 0, (int32) 10), 0);
    TEST_EQUALS(clamp_branchless((int32) 15, (int32) 0, (int32) 10), 10);
}

static void test_abs() {
    TEST_EQUALS(__internal_abs((int8) -5), 5);
    TEST_EQUALS(__internal_abs((int8) 5), 5);
    TEST_EQUALS(__internal_abs((int16) -1234), 1234);
    TEST_EQUALS(__internal_abs((int32) -123456), 123456);
    TEST_EQUALS(__internal_abs((int64) -1234567890123), 1234567890123);

    TEST_EQUALS_WITH_DELTA(__internal_abs(-1.5f), 1.5f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(__internal_abs(1.5f), 1.5f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(__internal_abs(-1.5), 1.5, 0.0001);
}

static void test_round_floor_ceil() {
    TEST_EQUALS(__internal_round(1.4f), 1.0f);
    TEST_EQUALS(__internal_round(1.6f), 2.0f);
    TEST_EQUALS(__internal_round(-1.4f), -1.0f);
    TEST_EQUALS(__internal_round(-1.6f), -2.0f);

    TEST_EQUALS(__internal_floor(1.4f), 1.0f);
    TEST_EQUALS(__internal_floor(1.0f), 1.0f);
    TEST_EQUALS(__internal_floor(-1.4f), -2.0f);
    TEST_EQUALS(__internal_floor(-1.0f), -1.0f);

    TEST_EQUALS(__internal_ceil(1.4f), 2.0f);
    TEST_EQUALS(__internal_ceil(1.0f), 1.0f);
    TEST_EQUALS(__internal_ceil(-1.4f), -1.0f);

    TEST_EQUALS(oms_round_positive(1.4f), 1);
    TEST_EQUALS(oms_round_positive(1.6f), 2);
}

static void test_floor_div() {
    // Normal integer division is not what we want for negative numbers
    TEST_EQUALS(floor_div((int32) 7, (int32) 3), 2);
    TEST_EQUALS(floor_div((int32) -7, (int32) 3), -3);
    TEST_EQUALS(floor_div((int32) 6, (int32) 3), 2);
    TEST_EQUALS(floor_div((int32) -6, (int32) 3), -2);

    TEST_EQUALS(floor_div((int64) -7, (int64) 2), -4);
}

static void test_ceil_div_pow2() {
    TEST_EQUALS(ceil_div_pow2<64>(64), 1);
    TEST_EQUALS(ceil_div_pow2<64>(65), 2);
    TEST_EQUALS(ceil_div_pow2<64>(128), 2);
    TEST_EQUALS(ceil_div_pow2<64>(129), 3);
    TEST_EQUALS(ceil_div_pow2<8>(9), 2);

    TEST_EQUALS(CEIL_DIV(10, 3), 4);
    TEST_EQUALS(CEIL_DIV(9, 3), 3);
}

static void test_angles() {
    TEST_EQUALS_WITH_DELTA(deg2rad((f32) 180.0f), OMS_PI_F32, 0.0001f);
    TEST_EQUALS_WITH_DELTA(deg2rad((f32) 90.0f), OMS_PI_OVER_TWO_F32, 0.0001f);
    TEST_EQUALS_WITH_DELTA(rad2deg((f32) OMS_PI_F32), 180.0f, 0.0001f);

    // Normalizing keeps the angle in [-pi, pi]
    TEST_EQUALS_WITH_DELTA(normalize_rad((f32) 0.0f), 0.0f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(normalize_rad((f32) OMS_TAU_F32), 0.0f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(normalize_rad((f32) (OMS_PI_F32 + 1.0f)), -OMS_PI_F32 + 1.0f, 0.0001f);

    // Normalizing keeps the angle in [0, 360]
    TEST_EQUALS_WITH_DELTA(normalize_deg((f32) 0.0f), 0.0f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(normalize_deg((f32) 360.0f), 0.0f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(normalize_deg((f32) 450.0f), 90.0f, 0.0001f);
    TEST_EQUALS_WITH_DELTA(normalize_deg((f32) -90.0f), 270.0f, 0.0001f);
}

// Correctness tests for f32 (float) approximate functions
static void test_sin_approx_f32() {
    TEST_EQUALS_WITH_DELTA(sin_approx(0.0f), sinf(0.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(sin_approx(1.0f), sinf(1.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(sin_approx(OMS_PI_OVER_TWO_F32), sinf(OMS_PI_OVER_TWO_F32), 0.001f);
    TEST_EQUALS_WITH_DELTA(sin_approx(-1.0f), sinf(-1.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(sin_approx(3.14f), sinf(3.14f), 0.001f);
}

static void test_cos_approx_f32() {
    TEST_EQUALS_WITH_DELTA(cos_approx(0.0f), cosf(0.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(cos_approx(1.0f), cosf(1.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(cos_approx(3.14f), cosf(3.14f), 0.001f);
}

static void test_sincos_approx_f32() {
    f32 s;
    f32 c;
    sincos_approx(1.0f, &s, &c);

    TEST_EQUALS_WITH_DELTA(s, sinf(1.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(c, cosf(1.0f), 0.001f);
}

static void test_tan_approx_f32() {
    TEST_EQUALS_WITH_DELTA(tan_approx(0.0f), tanf(0.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(tan_approx(1.0f), tanf(1.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(tan_approx(0.5f), tanf(0.5f), 0.001f);
}

static void test_sqrt_approx_f32() {
    TEST_EQUALS_WITH_DELTA(sqrt_approx(1.0f), intrin_sqrt_f32(1.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(sqrt_approx(2.0f), intrin_sqrt_f32(2.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(sqrt_approx(100.0f), intrin_sqrt_f32(100.0f), 0.001f);
}

static void test_rsqrt_approx_f32() {
    TEST_EQUALS_WITH_DELTA(rsqrt_approx(1.0f), 1.0f / intrin_sqrt_f32(1.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(rsqrt_approx(2.0f), 1.0f / intrin_sqrt_f32(2.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(rsqrt_approx(100.0f), 1.0f / intrin_sqrt_f32(100.0f), 0.001f);
}

static void test_asin_acos_atan_approx_f32() {
    TEST_EQUALS_WITH_DELTA(asin_approx(0.0f), asinf(0.0f), 0.01f);
    TEST_EQUALS_WITH_DELTA(asin_approx(0.5f), asinf(0.5f), 0.01f);
    TEST_EQUALS_WITH_DELTA(asin_approx(-0.5f), asinf(-0.5f), 0.01f);

    TEST_EQUALS_WITH_DELTA(acos_approx(0.0f), acosf(0.0f), 0.01f);
    TEST_EQUALS_WITH_DELTA(acos_approx(0.5f), acosf(0.5f), 0.01f);
    TEST_EQUALS_WITH_DELTA(acos_approx(-0.5f), acosf(-0.5f), 0.01f);

    TEST_EQUALS_WITH_DELTA(atan_approx(0.0f), atanf(0.0f), 0.01f);
    TEST_EQUALS_WITH_DELTA(atan_approx(1.0f), atanf(1.0f), 0.01f);
    TEST_EQUALS_WITH_DELTA(atan_approx(-1.0f), atanf(-1.0f), 0.01f);
}

static void test_exp_log_pow_approx_f32() {
    TEST_EQUALS_WITH_DELTA(exp_approx(0.0f), expf(0.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(exp_approx(1.0f), expf(1.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(exp_approx(-1.0f), expf(-1.0f), 0.001f);

    TEST_EQUALS_WITH_DELTA(log_approx(1.0f), logf(1.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(log_approx(2.0f), logf(2.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(log_approx(10.0f), logf(10.0f), 0.001f);

    TEST_EQUALS_WITH_DELTA(pow_approx(2.0f, 3.0f), powf(2.0f, 3.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(pow_approx(3.0f, 2.0f), powf(3.0f, 2.0f), 0.001f);
    TEST_EQUALS_WITH_DELTA(pow_approx(10.0f, 0.5f), powf(10.0f, 0.5f), 0.001f);
}

// Correctness tests for f64 (double) approximate functions
static void test_sin_approx_f64() {
    TEST_EQUALS_WITH_DELTA(sin_approx(0.0), sin(0.0), 0.001);
    TEST_EQUALS_WITH_DELTA(sin_approx(1.0), sin(1.0), 0.001);
    TEST_EQUALS_WITH_DELTA(sin_approx(3.14), sin(3.14), 0.001);
}

static void test_cos_approx_f64() {
    TEST_EQUALS_WITH_DELTA(cos_approx(0.0), cos(0.0), 0.001);
    TEST_EQUALS_WITH_DELTA(cos_approx(1.0), cos(1.0), 0.001);
    TEST_EQUALS_WITH_DELTA(cos_approx(3.14), cos(3.14), 0.001);
}

static void test_tan_approx_f64() {
    TEST_EQUALS_WITH_DELTA(tan_approx(0.0), tan(0.0), 0.001);
    TEST_EQUALS_WITH_DELTA(tan_approx(1.0), tan(1.0), 0.001);
    TEST_EQUALS_WITH_DELTA(tan_approx(0.5), tan(0.5), 0.001);
}

static void test_sqrt_approx_f64() {
    TEST_EQUALS_WITH_DELTA(sqrt_approx(1.0), sqrt(1.0), 0.001);
    TEST_EQUALS_WITH_DELTA(sqrt_approx(2.0), sqrt(2.0), 0.001);
    TEST_EQUALS_WITH_DELTA(sqrt_approx(100.0), sqrt(100.0), 0.001);
}

static void test_rsqrt_approx_f64() {
    TEST_EQUALS_WITH_DELTA(rsqrt_approx(1.0), 1.0 / sqrt(1.0), 0.001);
    TEST_EQUALS_WITH_DELTA(rsqrt_approx(2.0), 1.0 / sqrt(2.0), 0.001);
    TEST_EQUALS_WITH_DELTA(rsqrt_approx(100.0), 1.0 / sqrt(100.0), 0.001);
}

static void test_asin_acos_atan_approx_f64() {
    TEST_EQUALS_WITH_DELTA(asin_approx(0.0), asin(0.0), 0.01);
    TEST_EQUALS_WITH_DELTA(asin_approx(0.5), asin(0.5), 0.01);
    TEST_EQUALS_WITH_DELTA(asin_approx(-0.5), asin(-0.5), 0.01);

    TEST_EQUALS_WITH_DELTA(acos_approx(0.0), acos(0.0), 0.01);
    TEST_EQUALS_WITH_DELTA(acos_approx(0.5), acos(0.5), 0.01);
    TEST_EQUALS_WITH_DELTA(acos_approx(-0.5), acos(-0.5), 0.01);

    TEST_EQUALS_WITH_DELTA(atan_approx(0.0), atan(0.0), 0.01);
    TEST_EQUALS_WITH_DELTA(atan_approx(1.0), atan(1.0), 0.01);
    TEST_EQUALS_WITH_DELTA(atan_approx(-1.0), atan(-1.0), 0.01);
}

static void test_exp_log_pow_approx_f64() {
    TEST_EQUALS_WITH_DELTA(exp_approx(0.0), exp(0.0), 0.001);
    TEST_EQUALS_WITH_DELTA(exp_approx(1.0), exp(1.0), 0.001);
    TEST_EQUALS_WITH_DELTA(exp_approx(-1.0), exp(-1.0), 0.001);

    TEST_EQUALS_WITH_DELTA(log_approx(1.0), log(1.0), 0.001);
    TEST_EQUALS_WITH_DELTA(log_approx(2.0), log(2.0), 0.001);
    TEST_EQUALS_WITH_DELTA(log_approx(10.0), log(10.0), 0.001);

    TEST_EQUALS_WITH_DELTA(pow_approx(2.0, 3.0), pow(2.0, 3.0), 0.001);
    TEST_EQUALS_WITH_DELTA(pow_approx(3.0, 2.0), pow(3.0, 2.0), 0.001);
    TEST_EQUALS_WITH_DELTA(pow_approx(10.0, 0.5), pow(10.0, 0.5), 0.001);
}

static void test_factorial() {
    TEST_EQUALS(factorial(0), 1.0);
    TEST_EQUALS(factorial(1), 1.0);
    TEST_EQUALS(factorial(5), 120.0);
    TEST_EQUALS(factorial(10), 3628800.0);
}

#if PERFORMANCE_TEST
static void _sin_approx_f32(volatile void* val) {
    f32* res = (f32 *) val;
    *res += sin_approx((f32) rand() / RAND_MAX);
}

static void _sin_f32(volatile void* val) {
    f32* res = (f32 *) val;
    *res += sinf((f32) rand() / RAND_MAX);
}

static void test_sin_approx_performance_f32() {
    COMPARE_FUNCTION_TEST_TIME(_sin_approx_f32, _sin_f32, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_sin_approx_f32, _sin_f32, 5.0);
}

static void _sqrt_approx_f32(volatile void* val) {
    f32* res = (f32 *) val;
    *res += sqrt_approx((f32) rand() / RAND_MAX + 0.1f); // Avoid sqrt(0)
}

static void _sqrt_f32(volatile void* val) {
    f32* res = (f32 *) val;
    *res += intrin_sqrt_f32((f32) rand() / RAND_MAX + 0.1f); // Avoid sqrt(0)
}

static void test_sqrt_approx_performance_f32() {
    COMPARE_FUNCTION_TEST_TIME(_sqrt_approx_f32, _sqrt_f32, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_sqrt_approx_f32, _sqrt_f32, 5.0);
}
#endif

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UtilsMathUtilsTest
#endif

int main() {
    TEST_INIT(200);

    TEST_RUN(test_min_max);
    TEST_RUN(test_abs);
    TEST_RUN(test_round_floor_ceil);
    TEST_RUN(test_floor_div);
    TEST_RUN(test_ceil_div_pow2);
    TEST_RUN(test_angles);
    TEST_RUN(test_factorial);

    // Run correctness tests for f32 functions
    TEST_RUN(test_sin_approx_f32);
    TEST_RUN(test_cos_approx_f32);
    TEST_RUN(test_sincos_approx_f32);
    TEST_RUN(test_tan_approx_f32);
    TEST_RUN(test_sqrt_approx_f32);
    TEST_RUN(test_rsqrt_approx_f32);
    TEST_RUN(test_asin_acos_atan_approx_f32);
    TEST_RUN(test_exp_log_pow_approx_f32);

    // Run correctness tests for f64 functions
    TEST_RUN(test_sin_approx_f64);
    TEST_RUN(test_cos_approx_f64);
    TEST_RUN(test_tan_approx_f64);
    TEST_RUN(test_sqrt_approx_f64);
    TEST_RUN(test_rsqrt_approx_f64);
    TEST_RUN(test_asin_acos_atan_approx_f64);
    TEST_RUN(test_exp_log_pow_approx_f64);

    #if PERFORMANCE_TEST
        TEST_RUN(test_sin_approx_performance_f32);
        TEST_RUN(test_sqrt_approx_performance_f32);
    #endif

    TEST_FINALIZE();

    return 0;
}

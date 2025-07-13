#include "../TestFramework.h"
#include "../../utils/MathUtils.h"
#include <math.h>

// Correctness tests for f32 (float) approximate functions
static void test_sin_approx_f32() {
    ASSERT_EQUALS_WITH_DELTA(sin_approx(0.0f), sinf(0.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(sin_approx(1.0f), sinf(1.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(sin_approx(3.14f), sinf(3.14f), 0.001f);
}

static void test_cos_approx_f32() {
    ASSERT_EQUALS_WITH_DELTA(cos_approx(0.0f), cosf(0.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(cos_approx(1.0f), cosf(1.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(cos_approx(3.14f), cosf(3.14f), 0.001f);
}

static void test_tan_approx_f32() {
    ASSERT_EQUALS_WITH_DELTA(tan_approx(0.0f), tanf(0.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(tan_approx(1.0f), tanf(1.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(tan_approx(0.5f), tanf(0.5f), 0.001f);
}

static void test_sqrt_approx_f32() {
    ASSERT_EQUALS_WITH_DELTA(sqrt_approx(1.0f), sqrtf(1.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(sqrt_approx(2.0f), sqrtf(2.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(sqrt_approx(100.0f), sqrtf(100.0f), 0.001f);
}

static void test_asin_approx_f32() {
    ASSERT_EQUALS_WITH_DELTA(asin_approx(0.0f), asinf(0.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(asin_approx(0.5f), asinf(0.5f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(asin_approx(-0.5f), asinf(-0.5f), 0.001f);
}

static void test_acos_approx_f32() {
    ASSERT_EQUALS_WITH_DELTA(acos_approx(0.0f), acosf(0.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(acos_approx(0.5f), acosf(0.5f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(acos_approx(-0.5f), acosf(-0.5f), 0.001f);
}

static void test_atan_approx_f32() {
    ASSERT_EQUALS_WITH_DELTA(atan_approx(0.0f), atanf(0.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(atan_approx(1.0f), atanf(1.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(atan_approx(-1.0f), atanf(-1.0f), 0.001f);
}

static void test_rsqrt_approx_f32() {
    ASSERT_EQUALS_WITH_DELTA(rsqrt_approx(1.0f), 1.0f / sqrtf(1.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(rsqrt_approx(2.0f), 1.0f / sqrtf(2.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(rsqrt_approx(100.0f), 1.0f / sqrtf(100.0f), 0.001f);
}

static void test_exp_approx_f32() {
    ASSERT_EQUALS_WITH_DELTA(exp_approx(0.0f), expf(0.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(exp_approx(1.0f), expf(1.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(exp_approx(-1.0f), expf(-1.0f), 0.001f);
}

static void test_log_approx_f32() {
    ASSERT_EQUALS_WITH_DELTA(log_approx(1.0f), logf(1.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(log_approx(2.0f), logf(2.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(log_approx(10.0f), logf(10.0f), 0.001f);
}

static void test_pow_approx_f32() {
    ASSERT_EQUALS_WITH_DELTA(pow_approx(2.0f, 3.0f), powf(2.0f, 3.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(pow_approx(3.0f, 2.0f), powf(3.0f, 2.0f), 0.001f);
    ASSERT_EQUALS_WITH_DELTA(pow_approx(10.0f, 0.5f), powf(10.0f, 0.5f), 0.001f);
}

// Correctness tests for f64 (double) approximate functions
static void test_sin_approx_f64() {
    ASSERT_EQUALS_WITH_DELTA(sin_approx(0.0), sin(0.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(sin_approx(1.0), sin(1.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(sin_approx(3.14), sin(3.14), 0.001);
}

static void test_cos_approx_f64() {
    ASSERT_EQUALS_WITH_DELTA(cos_approx(0.0), cos(0.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(cos_approx(1.0), cos(1.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(cos_approx(3.14), cos(3.14), 0.001);
}

static void test_tan_approx_f64() {
    ASSERT_EQUALS_WITH_DELTA(tan_approx(0.0), tan(0.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(tan_approx(1.0), tan(1.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(tan_approx(0.5), tan(0.5), 0.001);
}

static void test_sqrt_approx_f64() {
    ASSERT_EQUALS_WITH_DELTA(sqrt_approx(1.0), sqrt(1.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(sqrt_approx(2.0), sqrt(2.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(sqrt_approx(100.0), sqrt(100.0), 0.001);
}

static void test_asin_approx_f64() {
    ASSERT_EQUALS_WITH_DELTA(asin_approx(0.0), asin(0.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(asin_approx(0.5), asin(0.5), 0.001);
    ASSERT_EQUALS_WITH_DELTA(asin_approx(-0.5), asin(-0.5), 0.001);
}

static void test_acos_approx_f64() {
    ASSERT_EQUALS_WITH_DELTA(acos_approx(0.0), acos(0.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(acos_approx(0.5), acos(0.5), 0.001);
    ASSERT_EQUALS_WITH_DELTA(acos_approx(-0.5), acos(-0.5), 0.001);
}

static void test_atan_approx_f64() {
    ASSERT_EQUALS_WITH_DELTA(atan_approx(0.0), atan(0.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(atan_approx(1.0), atan(1.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(atan_approx(-1.0), atan(-1.0), 0.001);
}

static void test_rsqrt_approx_f64() {
    ASSERT_EQUALS_WITH_DELTA(rsqrt_approx(1.0), 1.0 / sqrt(1.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(rsqrt_approx(2.0), 1.0 / sqrt(2.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(rsqrt_approx(100.0), 1.0 / sqrt(100.0), 0.001);
}

static void test_exp_approx_f64() {
    ASSERT_EQUALS_WITH_DELTA(exp_approx(0.0), exp(0.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(exp_approx(1.0), exp(1.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(exp_approx(-1.0), exp(-1.0), 0.001);
}

static void test_log_approx_f64() {
    ASSERT_EQUALS_WITH_DELTA(log_approx(1.0), log(1.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(log_approx(2.0), log(2.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(log_approx(10.0), log(10.0), 0.001);
}

static void test_pow_approx_f64() {
    ASSERT_EQUALS_WITH_DELTA(pow_approx(2.0, 3.0), pow(2.0, 3.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(pow_approx(3.0, 2.0), pow(3.0, 2.0), 0.001);
    ASSERT_EQUALS_WITH_DELTA(pow_approx(10.0, 0.5), pow(10.0, 0.5), 0.001);
}

#if PERFORMANCE_TEST
// Performance tests for f32 (float) approximate functions
static void _sin_approx_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += sin_approx((f32)rand() / RAND_MAX);
}

static void _sin_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += sinf((f32)rand() / RAND_MAX);
}

static void test_sin_approx_performance_f32() {
    COMPARE_FUNCTION_TEST_TIME(_sin_approx_f32, _sin_f32, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_sin_approx_f32, _sin_f32, 5.0);
}

static void _cos_approx_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += cos_approx((f32)rand() / RAND_MAX);
}

static void _cos_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += cosf((f32)rand() / RAND_MAX);
}

static void test_cos_approx_performance_f32() {
    COMPARE_FUNCTION_TEST_TIME(_cos_approx_f32, _cos_f32, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_cos_approx_f32, _cos_f32, 5.0);
}

static void _tan_approx_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += tan_approx((f32)rand() / RAND_MAX);
}

static void _tan_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += tanf((f32)rand() / RAND_MAX);
}

static void test_tan_approx_performance_f32() {
    COMPARE_FUNCTION_TEST_TIME(_tan_approx_f32, _tan_f32, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_tan_approx_f32, _tan_f32, 5.0);
}

static void _sqrt_approx_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += sqrt_approx((f32)rand() / RAND_MAX + 0.1f); // Avoid sqrt(0)
}

static void _sqrt_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += sqrtf((f32)rand() / RAND_MAX + 0.1f); // Avoid sqrt(0)
}

static void test_sqrt_approx_performance_f32() {
    COMPARE_FUNCTION_TEST_TIME(_sqrt_approx_f32, _sqrt_f32, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_sqrt_approx_f32, _sqrt_f32, 5.0);
}

static void _asin_approx_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += asin_approx((f32)rand() / RAND_MAX);
}

static void _asin_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += asinf((f32)rand() / RAND_MAX);
}

static void test_asin_approx_performance_f32() {
    COMPARE_FUNCTION_TEST_TIME(_asin_approx_f32, _asin_f32, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_asin_approx_f32, _asin_f32, 5.0);
}

static void _acos_approx_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += acos_approx((f32)rand() / RAND_MAX);
}

static void _acos_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += acosf((f32)rand() / RAND_MAX);
}

static void test_acos_approx_performance_f32() {
    COMPARE_FUNCTION_TEST_TIME(_acos_approx_f32, _acos_f32, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_acos_approx_f32, _acos_f32, 5.0);
}

static void _atan_approx_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += atan_approx((f32)rand() / RAND_MAX);
}

static void _atan_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += atanf((f32)rand() / RAND_MAX);
}

static void test_atan_approx_performance_f32() {
    COMPARE_FUNCTION_TEST_TIME(_atan_approx_f32, _atan_f32, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_atan_approx_f32, _atan_f32, 5.0);
}

static void _rsqrt_approx_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += rsqrt_approx((f32)rand() / RAND_MAX + 0.1f); // Avoid division by zero
}

static void _rsqrt_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += 1.0f / sqrtf((f32)rand() / RAND_MAX + 0.1f); // Avoid division by zero
}

static void test_rsqrt_approx_performance_f32() {
    COMPARE_FUNCTION_TEST_TIME(_rsqrt_approx_f32, _rsqrt_f32, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_rsqrt_approx_f32, _rsqrt_f32, 5.0);
}

static void _exp_approx_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += exp_approx((f32)rand() / RAND_MAX);
}

static void _exp_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += expf((f32)rand() / RAND_MAX);
}

static void test_exp_approx_performance_f32() {
    COMPARE_FUNCTION_TEST_TIME(_exp_approx_f32, _exp_f32, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_exp_approx_f32, _exp_f32, 5.0);
}

static void _log_approx_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += log_approx((f32)rand() / RAND_MAX + 0.1f); // Avoid log(0)
}

static void _log_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += logf((f32)rand() / RAND_MAX + 0.1f); // Avoid log(0)
}

static void test_log_approx_performance_f32() {
    COMPARE_FUNCTION_TEST_TIME(_log_approx_f32, _log_f32, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_log_approx_f32, _log_f32, 5.0);
}

static void _pow_approx_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += pow_approx((f32)rand() / RAND_MAX, (f32)rand() / RAND_MAX);
}

static void _pow_f32(volatile void* val) {
    f32* res = (f32*)val;
    srand((int32) *res);

    *res += powf((f32)rand() / RAND_MAX, (f32)rand() / RAND_MAX);
}

static void test_pow_approx_performance_f32() {
    COMPARE_FUNCTION_TEST_TIME(_pow_approx_f32, _pow_f32, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_pow_approx_f32, _pow_f32, 5.0);
}

// Performance tests for f64 (double) approximate functions
static void _sin_approx_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += sin_approx((f64)rand() / RAND_MAX);
}

static void _sin_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += sin((f64)rand() / RAND_MAX);
}

static void test_sin_approx_performance_f64() {
    COMPARE_FUNCTION_TEST_TIME(_sin_approx_f64, _sin_f64, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_sin_approx_f64, _sin_f64, 5.0);
}

static void _cos_approx_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += cos_approx((f64)rand() / RAND_MAX);
}

static void _cos_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += cos((f64)rand() / RAND_MAX);
}

static void test_cos_approx_performance_f64() {
    COMPARE_FUNCTION_TEST_TIME(_cos_approx_f64, _cos_f64, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_cos_approx_f64, _cos_f64, 5.0);
}

static void _tan_approx_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += tan_approx((f64)rand() / RAND_MAX);
}

static void _tan_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += tan((f64)rand() / RAND_MAX);
}

static void test_tan_approx_performance_f64() {
    COMPARE_FUNCTION_TEST_TIME(_tan_approx_f64, _tan_f64, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_tan_approx_f64, _tan_f64, 5.0);
}

static void _sqrt_approx_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += sqrt_approx((f64)rand() / RAND_MAX + 0.1); // Avoid sqrt(0)
}

static void _sqrt_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += sqrt((f64)rand() / RAND_MAX + 0.1); // Avoid sqrt(0)
}

static void test_sqrt_approx_performance_f64() {
    COMPARE_FUNCTION_TEST_TIME(_sqrt_approx_f64, _sqrt_f64, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_sqrt_approx_f64, _sqrt_f64, 5.0);
}

static void _asin_approx_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += asin_approx((f64)rand() / RAND_MAX);
}

static void _asin_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += asin((f64)rand() / RAND_MAX);
}

static void test_asin_approx_performance_f64() {
    COMPARE_FUNCTION_TEST_TIME(_asin_approx_f64, _asin_f64, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_asin_approx_f64, _asin_f64, 5.0);
}

static void _acos_approx_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += acos_approx((f64)rand() / RAND_MAX);
}

static void _acos_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += acos((f64)rand() / RAND_MAX);
}

static void test_acos_approx_performance_f64() {
    COMPARE_FUNCTION_TEST_TIME(_acos_approx_f64, _acos_f64, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_acos_approx_f64, _acos_f64, 5.0);
}

static void _atan_approx_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += atan_approx((f64)rand() / RAND_MAX);
}

static void _atan_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += atan((f64)rand() / RAND_MAX);
}

static void test_atan_approx_performance_f64() {
    COMPARE_FUNCTION_TEST_TIME(_atan_approx_f64, _atan_f64, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_atan_approx_f64, _atan_f64, 5.0);
}

static void _rsqrt_approx_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += rsqrt_approx((f64)rand() / RAND_MAX + 0.1); // Avoid division by zero
}

static void _rsqrt_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += 1.0 / sqrt((f64)rand() / RAND_MAX + 0.1); // Avoid division by zero
}

static void test_rsqrt_approx_performance_f64() {
    COMPARE_FUNCTION_TEST_TIME(_rsqrt_approx_f64, _rsqrt_f64, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_rsqrt_approx_f64, _rsqrt_f64, 5.0);
}

static void _exp_approx_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += exp_approx((f64)rand() / RAND_MAX);
}

static void _exp_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += exp((f64)rand() / RAND_MAX);
}

static void test_exp_approx_performance_f64() {
    COMPARE_FUNCTION_TEST_TIME(_exp_approx_f64, _exp_f64, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_exp_approx_f64, _exp_f64, 5.0);
}

static void _log_approx_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += log_approx((f64)rand() / RAND_MAX + 0.1); // Avoid log(0)
}

static void _log_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += log((f64)rand() / RAND_MAX + 0.1); // Avoid log(0)
}

static void test_log_approx_performance_f64() {
    COMPARE_FUNCTION_TEST_TIME(_log_approx_f64, _log_f64, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_log_approx_f64, _log_f64, 5.0);
}

static void _pow_approx_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += pow_approx((f64)rand() / RAND_MAX, (f64)rand() / RAND_MAX);
}

static void _pow_f64(volatile void* val) {
    f64* res = (f64*)val;
    srand((int32) *res);

    *res += pow((f64)rand() / RAND_MAX, (f64)rand() / RAND_MAX);
}

static void test_pow_approx_performance_f64() {
    COMPARE_FUNCTION_TEST_TIME(_pow_approx_f64, _pow_f64, 5.0);
    COMPARE_FUNCTION_TEST_CYCLE(_pow_approx_f64, _pow_f64, 5.0);
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

    // Run correctness tests for f32 functions
    TEST_RUN(test_sin_approx_f32);
    TEST_RUN(test_cos_approx_f32);
    TEST_RUN(test_tan_approx_f32);
    TEST_RUN(test_sqrt_approx_f32);
    TEST_RUN(test_asin_approx_f32);
    TEST_RUN(test_acos_approx_f32);
    TEST_RUN(test_atan_approx_f32);
    TEST_RUN(test_rsqrt_approx_f32);
    TEST_RUN(test_exp_approx_f32);
    TEST_RUN(test_log_approx_f32);
    TEST_RUN(test_pow_approx_f32);

    // Run correctness tests for f64 functions
    TEST_RUN(test_sin_approx_f64);
    TEST_RUN(test_cos_approx_f64);
    TEST_RUN(test_tan_approx_f64);
    TEST_RUN(test_sqrt_approx_f64);
    TEST_RUN(test_asin_approx_f64);
    TEST_RUN(test_acos_approx_f64);
    TEST_RUN(test_atan_approx_f64);
    TEST_RUN(test_rsqrt_approx_f64);
    TEST_RUN(test_exp_approx_f64);
    TEST_RUN(test_log_approx_f64);
    TEST_RUN(test_pow_approx_f64);

    #if PERFORMANCE_TEST
        // Run performance tests for f32 functions
        TEST_RUN(test_sin_approx_performance_f32);
        TEST_RUN(test_cos_approx_performance_f32);
        TEST_RUN(test_tan_approx_performance_f32);
        TEST_RUN(test_sqrt_approx_performance_f32);
        TEST_RUN(test_asin_approx_performance_f32);
        TEST_RUN(test_acos_approx_performance_f32);
        TEST_RUN(test_atan_approx_performance_f32);
        TEST_RUN(test_rsqrt_approx_performance_f32);
        TEST_RUN(test_exp_approx_performance_f32);
        TEST_RUN(test_log_approx_performance_f32);
        TEST_RUN(test_pow_approx_performance_f32);

        // Run performance tests for f64 functions
        TEST_RUN(test_sin_approx_performance_f64);
        TEST_RUN(test_cos_approx_performance_f64);
        TEST_RUN(test_tan_approx_performance_f64);
        TEST_RUN(test_sqrt_approx_performance_f64);
        TEST_RUN(test_asin_approx_performance_f64);
        TEST_RUN(test_acos_approx_performance_f64);
        TEST_RUN(test_atan_approx_performance_f64);
        TEST_RUN(test_rsqrt_approx_performance_f64);
        TEST_RUN(test_exp_approx_performance_f64);
        TEST_RUN(test_log_approx_performance_f64);
        TEST_RUN(test_pow_approx_performance_f64);
    #endif

    TEST_FINALIZE();

    return 0;
}
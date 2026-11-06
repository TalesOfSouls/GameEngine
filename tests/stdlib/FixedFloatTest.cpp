#include "../TestFramework.h"
#include "../../stdlib/FixedFloat.h"

/**
 * FixedFloat tests (Q16.16 fixed point math)
 */

static void test_fixed_float_constants() {
    TEST_EQUALS(FIXED_FLOAT_SHIFT, 16);
    TEST_EQUALS(FIXED_FLOAT_ONE, 65536);
    TEST_EQUALS(FIXED_FLOAT_HALF, 32768);
    TEST_EQUALS(FIXED_FLOAT_ZERO, 0);

    TEST_EQUALS(sizeof(FixedFloat), 4);
    TEST_EQUALS(sizeof(BinaryAngle), 4);

    TEST_EQUALS(FIXED_FLOAT_QUADRANT_BITS, 30);
    TEST_EQUALS(FIXED_FLOAT_QUADRANT_ONE, 0x40000000);
    TEST_EQUALS(FIXED_FLOAT_QUADRANT_ONE * 4, 0);
}

static void test_fixed_float_from_int() {
    TEST_EQUALS(fixed_float_from_int(0), 0);
    TEST_EQUALS(fixed_float_from_int(1), 65536);
    TEST_EQUALS(fixed_float_from_int(-1), -65536);
    TEST_EQUALS(fixed_float_from_int(100), 6553600);
    TEST_EQUALS(fixed_float_from_int(-100), -6553600);

    // Round trip for all integer values
    TEST_EQUALS(fixed_float_to_int(fixed_float_from_int(0)), 0);
    TEST_EQUALS(fixed_float_to_int(fixed_float_from_int(7)), 7);
    TEST_EQUALS(fixed_float_to_int(fixed_float_from_int(-7)), -7);
    TEST_EQUALS(fixed_float_to_int(fixed_float_from_int(12345)), 12345);

    // The conversion to int truncates (arithmetic shift, so negative values floor)
    TEST_EQUALS(fixed_float_to_int(FIXED_FLOAT_ONE - 1), 0);
    TEST_EQUALS(fixed_float_to_int(FIXED_FLOAT_ONE), 1);
    TEST_EQUALS(fixed_float_to_int(3 * FIXED_FLOAT_ONE + FIXED_FLOAT_HALF), 3);
    TEST_EQUALS(fixed_float_to_int(-FIXED_FLOAT_HALF), -1);
}

static void test_fixed_float_from_float() {
    TEST_EQUALS(fixed_float_from_float(0.0), 0);
    TEST_EQUALS(fixed_float_from_float(0.5), 32768);
    TEST_EQUALS(fixed_float_from_float(1.0), 65536);
    TEST_EQUALS(fixed_float_from_float(-1.0), -65536);
    TEST_EQUALS(fixed_float_from_float(2.5), 163840);
    TEST_EQUALS(fixed_float_from_float(-2.5), -163840);
    TEST_EQUALS(fixed_float_from_float(1.25), 81920);

    // Values are rounded to the nearest fixed point value
    TEST_EQUALS(fixed_float_from_float(0.4 / 65536.0), 0);
    TEST_EQUALS(fixed_float_from_float(1.0 / 65536.0), 1);
    TEST_EQUALS(fixed_float_from_float(1.5 / 65536.0), 2);
    TEST_EQUALS(fixed_float_from_float(-1.5 / 65536.0), -2);

    // Round trip stays inside the fixed point resolution
    TEST_EQUALS_WITH_DELTA(fixed_float_to_float(fixed_float_from_float(3.14159265358979)), 3.14159265358979, 1.0 / 65536.0);
    TEST_EQUALS_WITH_DELTA(fixed_float_to_float(fixed_float_from_float(-0.125)), -0.125, 1.0 / 65536.0);
}

static void test_fixed_float_to_float() {
    TEST_EQUALS_WITH_DELTA(fixed_float_to_float(FIXED_FLOAT_ONE), 1.0, 0.0000001);
    TEST_EQUALS_WITH_DELTA(fixed_float_to_float(FIXED_FLOAT_HALF), 0.5, 0.0000001);
    TEST_EQUALS_WITH_DELTA(fixed_float_to_float(-FIXED_FLOAT_HALF), -0.5, 0.0000001);
    TEST_EQUALS_WITH_DELTA(fixed_float_to_float(fixed_float_from_int(42)), 42.0, 0.0000001);
    TEST_EQUALS_WITH_DELTA(fixed_float_to_float(0), 0.0, 0.0000001);
}

static void test_fixed_float_abs_min_max() {
    TEST_EQUALS(fixed_abs(fixed_float_from_int(-3)), 3 * FIXED_FLOAT_ONE);
    TEST_EQUALS(fixed_abs(fixed_float_from_int(3)), 3 * FIXED_FLOAT_ONE);
    TEST_EQUALS(fixed_abs(0), 0);
    TEST_EQUALS(fixed_abs(-1), 1);

    TEST_EQUALS(fixed_min(fixed_float_from_int(1), fixed_float_from_int(2)), FIXED_FLOAT_ONE);
    TEST_EQUALS(fixed_min(fixed_float_from_int(2), fixed_float_from_int(1)), FIXED_FLOAT_ONE);
    TEST_EQUALS(fixed_max(fixed_float_from_int(1), fixed_float_from_int(2)), 2 * FIXED_FLOAT_ONE);

    TEST_EQUALS(fixed_clamp(fixed_float_from_int(5), 0, fixed_float_from_int(10)), 5 * FIXED_FLOAT_ONE);
    TEST_EQUALS(fixed_clamp(fixed_float_from_int(-5), 0, fixed_float_from_int(10)), 0);
    TEST_EQUALS(fixed_clamp(fixed_float_from_int(15), 0, fixed_float_from_int(10)), 10 * FIXED_FLOAT_ONE);
}

static void test_fixed_float_sqrt() {
    // Integer square root
    TEST_EQUALS((long long) fixed_sqrt((uint64) 0), 0);
    TEST_EQUALS((long long) fixed_sqrt((uint64) 1), 1);
    TEST_EQUALS((long long) fixed_sqrt((uint64) 15), 3);
    TEST_EQUALS((long long) fixed_sqrt((uint64) 16), 4);
    TEST_EQUALS((long long) fixed_sqrt((uint64) 1000000), 1000);

    // Fixed point square root (result is a FixedFloat)
    TEST_EQUALS(fixed_sqrt(fixed_float_from_int(0)), 0);
    TEST_EQUALS(fixed_sqrt(fixed_float_from_int(1)), FIXED_FLOAT_ONE);
    TEST_EQUALS(fixed_sqrt(fixed_float_from_int(4)), 2 * FIXED_FLOAT_ONE);
    TEST_EQUALS(fixed_sqrt(fixed_float_from_int(9)), 3 * FIXED_FLOAT_ONE);
    TEST_EQUALS(fixed_sqrt(fixed_float_from_int(100)), 10 * FIXED_FLOAT_ONE);

    TEST_EQUALS_WITH_DELTA(fixed_float_to_float(fixed_sqrt(fixed_float_from_int(2))), 1.41421356, 0.0001);
    TEST_EQUALS_WITH_DELTA(fixed_float_to_float(fixed_sqrt(FIXED_FLOAT_HALF)), 0.70710678, 0.0001);
    TEST_EQUALS_WITH_DELTA(fixed_float_to_float(fixed_sqrt(fixed_float_from_float(0.25))), 0.5, 0.0001);

    // Negative values have no square root
    TEST_EQUALS(fixed_sqrt(-FIXED_FLOAT_ONE), 0);
    TEST_EQUALS(fixed_sqrt(-1), 0);
}

static void test_fixed_float_hypot() {
    // fixed_hypot depends on fixed_mul/fixed_add which are missing in the engine (see workaround above)
    TEST_EQUALS(fixed_hypot(fixed_float_from_int(3), fixed_float_from_int(4)), 5 * FIXED_FLOAT_ONE);
    TEST_EQUALS(fixed_hypot(fixed_float_from_int(0), fixed_float_from_int(0)), 0);
    TEST_EQUALS_WITH_DELTA(
        fixed_float_to_float(fixed_hypot(fixed_float_from_int(1), fixed_float_from_int(1))),
        1.41421356, 0.0005
    );
    TEST_EQUALS(fixed_hypot(fixed_float_from_int(-3), fixed_float_from_int(-4)), 5 * FIXED_FLOAT_ONE);
}

static void test_fixed_float_binary_angle() {
    TEST_EQUALS(binary_angle_from_degrees(0.0), 0);
    TEST_EQUALS(binary_angle_from_degrees(90.0), 0x40000000);
    TEST_EQUALS(binary_angle_from_degrees(180.0), 0x80000000);
    TEST_EQUALS(binary_angle_from_degrees(270.0), 0xC0000000);
    TEST_EQUALS(binary_angle_from_degrees(360.0), 0);
    TEST_EQUALS(binary_angle_from_degrees(45.0), 0x20000000);

    TEST_EQUALS(binary_angle_from_radians(0.0), 0);
    TEST_EQUALS(binary_angle_from_radians(OMS_PI_F64 / 2.0), 0x40000000);
    TEST_EQUALS(binary_angle_from_radians(OMS_PI_F64), 0x80000000);
    TEST_EQUALS(binary_angle_from_radians(OMS_PI_F64 * 1.5), 0xC0000000);

    // float overloads
    TEST_EQUALS(binary_angle_from_degrees(90.0f), 0x40000000);
    TEST_EQUALS(binary_angle_from_degrees(180.0f), 0x80000000);
    TEST_EQUALS(binary_angle_from_radians(OMS_PI_F32 / 2.0f), 0x40000000);
    TEST_EQUALS(binary_angle_from_radians(OMS_PI_F32), 0x80000000);
}

static void test_fixed_float_sin_cos() {
    // The table value 65536 == 1.0
    TEST_EQUALS(fixed_sin(0), 0);
    TEST_EQUALS(fixed_sin(binary_angle_from_degrees(90.0)), 65536);
    TEST_EQUALS(fixed_sin(binary_angle_from_degrees(180.0)), 0);
    TEST_EQUALS(fixed_sin(binary_angle_from_degrees(270.0)), -65536);
    TEST_EQUALS(fixed_sin(binary_angle_from_degrees(0.0)), 0);

    TEST_EQUALS_WITH_DELTA(fixed_sin(binary_angle_from_degrees(30.0)), 32768, 64);
    TEST_EQUALS_WITH_DELTA(fixed_sin(binary_angle_from_degrees(45.0)), 46341, 64);
    TEST_EQUALS_WITH_DELTA(fixed_sin(binary_angle_from_degrees(60.0)), 56756, 64);

    TEST_EQUALS(fixed_cos(0), 65536);
    TEST_EQUALS(fixed_cos(binary_angle_from_degrees(90.0)), 0);
    TEST_EQUALS(fixed_cos(binary_angle_from_degrees(180.0)), -65536);
    TEST_EQUALS(fixed_cos(binary_angle_from_degrees(360.0)), 65536);
    TEST_EQUALS_WITH_DELTA(fixed_cos(binary_angle_from_degrees(60.0)), 32768, 64);

    // sin^2 + cos^2 == 1 (fixed point rounding allows a small error)
    const BinaryAngle angle = binary_angle_from_degrees(37.0);
    const f64 sin_value = fixed_float_to_float(fixed_sin(angle));
    const f64 cos_value = fixed_float_to_float(fixed_cos(angle));
    TEST_EQUALS_WITH_DELTA(sin_value * sin_value + cos_value * cos_value, 1.0, 0.001);
}

static void test_fixed_float_atan2() {
    const FixedFloat zero = fixed_float_from_int(0);
    const FixedFloat one = fixed_float_from_int(1);

    // Undefined input returns 0 by definition
    TEST_EQUALS(fixed_atan2(zero, zero), 0);

    // atan2 is returned as a BinaryAngle
    TEST_EQUALS_WITH_DELTA(fixed_atan2(zero, one), 0, 4096);
    TEST_EQUALS_WITH_DELTA(fixed_atan2(one, zero), 0x40000000, 16384);
    TEST_EQUALS_WITH_DELTA(fixed_atan2(one, one), 0x20000000, 16384);
    TEST_EQUALS_WITH_DELTA(fixed_atan2(fixed_float_from_int(-1), zero), 0xC0000000, 16384);
    TEST_EQUALS_WITH_DELTA(fixed_atan2(fixed_float_from_int(-1), fixed_float_from_int(-1)), 0xA0000000, 32768);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main StdlibFixedFloatTest
#endif

int main() {
    TEST_INIT(96);

    TEST_RUN(test_fixed_float_constants);
    TEST_RUN(test_fixed_float_from_int);
    TEST_RUN(test_fixed_float_from_float);
    TEST_RUN(test_fixed_float_to_float);
    TEST_RUN(test_fixed_float_abs_min_max);
    TEST_RUN(test_fixed_float_sqrt);
    TEST_RUN(test_fixed_float_hypot);
    TEST_RUN(test_fixed_float_binary_angle);
    TEST_RUN(test_fixed_float_sin_cos);
    TEST_RUN(test_fixed_float_atan2);

    TEST_FINALIZE();

    return 0;
}

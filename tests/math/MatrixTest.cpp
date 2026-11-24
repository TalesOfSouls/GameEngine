#include "../TestFramework.h"
#include "../../math/matrix/Matrix.h"

static void test_vec3_normalize_f64() {
    // f64 is exact: sqrt + division
    v3_f64 v = { 3.0, 4.0, 12.0 };
    vec3_normalize(&v);
    TEST_EQUALS(v.x, 3.0 / 13.0);
    TEST_EQUALS(v.y, 4.0 / 13.0);
    TEST_EQUALS(v.z, 12.0 / 13.0);

    // Value version = pointer version, the input is unchanged
    const v3_f64 in = { 1.0e9, -2.0e9, 0.5 };
    const v3_f64 out = vec3_normalize(in);
    v3_f64 ref = in;
    vec3_normalize(&ref);
    TEST_EQUALS(out.x, ref.x);
    TEST_EQUALS(out.y, ref.y);
    TEST_EQUALS(out.z, ref.z);
    TEST_EQUALS(in.x, 1.0e9);
    TEST_EQUALS_WITH_DELTA(vec3_dot(out, out), 1.0, 1.0e-15);
}

static void test_vec3_normalize_f32() {
    // f32 uses the reciprocal square root approximation
    v3_f32 v = { 3.0f, 4.0f, 12.0f };
    vec3_normalize(&v);
    TEST_EQUALS_WITH_DELTA(v.x, 3.0f / 13.0f, 0.001f);
    TEST_EQUALS_WITH_DELTA(v.y, 4.0f / 13.0f, 0.001f);
    TEST_EQUALS_WITH_DELTA(v.z, 12.0f / 13.0f, 0.001f);

    const v3_f32 out = vec3_normalize(v3_f32{ 0.0f, 0.0f, 2.0f });
    TEST_EQUALS_WITH_DELTA(out.z, 1.0f, 0.001f);
}

static void test_vec_atan2() {
    TEST_EQUALS(vec3_atan2(v3_f64{ 1.0, 0.0, 5.0 }), 0.0);
    TEST_EQUALS(vec3_atan2(v3_f64{ 0.0, 2.0, -1.0 }), atan2(2.0, 0.0));
    TEST_EQUALS(vec3_atan2(v3_f64{ -1.0, -1.0, 0.0 }), atan2(-1.0, -1.0));
    TEST_EQUALS(vec2_atan2(v2_f32{ 1.0f, 1.0f }), atan2f(1.0f, 1.0f));
    TEST_EQUALS(vec3_atan2(v3_f32{ -1.0f, 0.5f, 3.0f }), atan2f(0.5f, -1.0f));
}

static void test_vec3_rot() {
    const f64 half_pi = OMS_PI_F64 * 0.5;

    // Quarter turns (counter clockwise when looking against the axis)
    const v3_f64 z = vec3_rot_z(v3_f64{ 1.0, 0.0, 7.0 }, half_pi);
    TEST_EQUALS_WITH_DELTA(z.x, 0.0, 1.0e-15);
    TEST_EQUALS_WITH_DELTA(z.y, 1.0, 1.0e-15);
    TEST_EQUALS(z.z, 7.0);

    const v3_f64 x = vec3_rot_x(v3_f64{ 7.0, 1.0, 0.0 }, half_pi);
    TEST_EQUALS(x.x, 7.0);
    TEST_EQUALS_WITH_DELTA(x.y, 0.0, 1.0e-15);
    TEST_EQUALS_WITH_DELTA(x.z, 1.0, 1.0e-15);

    const v3_f64 y = vec3_rot_y(v3_f64{ 0.0, 7.0, 1.0 }, half_pi);
    TEST_EQUALS_WITH_DELTA(y.x, 1.0, 1.0e-15);
    TEST_EQUALS(y.y, 7.0);
    TEST_EQUALS_WITH_DELTA(y.z, 0.0, 1.0e-15);

    // Rotation keeps the length, the inverse rotation restores the vector
    const v3_f64 v = { 1.5, -2.25, 4.0 };
    const v3_f64 r = vec3_rot_z(vec3_rot_x(v, 0.7), -1.3);
    TEST_EQUALS_WITH_DELTA(vec3_dot(r, r), vec3_dot(v, v), 1.0e-12);

    const v3_f64 back = vec3_rot_x(vec3_rot_z(r, 1.3), -0.7);
    TEST_EQUALS_WITH_DELTA(back.x, v.x, 1.0e-12);
    TEST_EQUALS_WITH_DELTA(back.y, v.y, 1.0e-12);
    TEST_EQUALS_WITH_DELTA(back.z, v.z, 1.0e-12);

    // f32 version
    const v3_f32 f = vec3_rot_z(v3_f32{ 0.0f, 2.0f, 1.0f }, (f32) half_pi);
    TEST_EQUALS_WITH_DELTA(f.x, -2.0f, 1.0e-6f);
    TEST_EQUALS_WITH_DELTA(f.y, 0.0f, 1.0e-6f);
    TEST_EQUALS(f.z, 1.0f);
}

static void test_vec3_mul_f64_scalar() {
    // The scalar has the component type -> no f32 rounding for f64 vectors
    const f64 scale = 1.0 / 3.0;
    const v3_f64 v = vec3_mul(v3_f64{ 3.0e10, 1.0, -7.0 }, scale);
    TEST_EQUALS(v.x, 3.0e10 * scale);
    TEST_EQUALS(v.y, scale);
    TEST_EQUALS(v.z, -7.0 * scale);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main MathMatrixTest
#endif

int main() {
    TEST_INIT(10);

    TEST_RUN(test_vec3_normalize_f64);
    TEST_RUN(test_vec3_normalize_f32);
    TEST_RUN(test_vec_atan2);
    TEST_RUN(test_vec3_rot);
    TEST_RUN(test_vec3_mul_f64_scalar);

    TEST_FINALIZE();

    return 0;
}

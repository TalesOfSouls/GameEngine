#include "../TestFramework.h"
#include "../../stdlib/Simd.h"

/**
 * SIMD tests
 *
 * Simd.h exposes the architecture SIMD helpers of architecture/x86/simd/SIMD_*.h.
 * The build used here doesn't enable __SSE4_2__/__AVX2__/__AVX512F__, so the functions
 * take their scalar (or single 128 bit) path - the tests always compare against a scalar
 * reference implementation and therefore work with every SIMD level.
 *
 * NOTE (@see report): simd_mult(const int32*, const int32*, int32*, ...) is NOT tested,
 * its scalar loop increments the pointers before writing and therefore reads/writes one
 * element past the end of the arrays.
 * NOTE (@see report): the f32_8/i32_8 wrapper types of SIMD_*_AVX2.h are not reachable
 * through Simd.h because those headers reuse the include guard of their parent header.
 */

#define SIMD_TEST_SIZE 32

static void test_simd_f32_sum() {
    alignas(32) f32 data[SIMD_TEST_SIZE];
    for (int32 i = 0; i < SIMD_TEST_SIZE; ++i) {
        data[i] = (f32) (i + 1);
    }

    // Full vector width
    f32 reference = 0.0f;
    for (int32 i = 0; i < 16; ++i) {
        reference += data[i];
    }

    TEST_EQUALS_WITH_DELTA(simd_sum(data, 16, 16), reference, 0.0001);
    TEST_EQUALS_WITH_DELTA(simd_sum_unaligned(data, 16, 16), reference, 0.0001);

    // Size smaller than the step size -> pure scalar tail
    TEST_EQUALS_WITH_DELTA(simd_sum(data, 1, 16), 1.0f, 0.0001);
    TEST_EQUALS_WITH_DELTA(simd_sum(data, 7, 16), 28.0f, 0.0001);

    // Size not a multiple of the step size
    TEST_EQUALS_WITH_DELTA(simd_sum(data, 10, 4), 55.0f, 0.0001);
    TEST_EQUALS_WITH_DELTA(simd_sum(data, 17, 8), 153.0f, 0.0001);

    // steps == 1 disables the vector path completely
    TEST_EQUALS_WITH_DELTA(simd_sum(data, 16, 1), reference, 0.0001);

    // Sum of the whole array
    TEST_EQUALS_WITH_DELTA(simd_sum(data, SIMD_TEST_SIZE, 16), 528.0f, 0.0001);

    // Unaligned input (offset by one element) has to produce the same result
    TEST_EQUALS_WITH_DELTA(simd_sum_unaligned(data + 1, 15, 16), reference - data[0], 0.0001);

    // The aligned and unaligned versions agree
    TEST_EQUALS_WITH_DELTA(simd_sum(data, 16, 16), simd_sum_unaligned(data, 16, 16), 0.0001);

    // All zero
    alignas(32) f32 zeros[8] = {0};
    TEST_EQUALS_WITH_DELTA(simd_sum(zeros, 8, 8), 0.0f, 0.0001);
}

static void test_simd_i32_sum() {
    alignas(32) int32 data[SIMD_TEST_SIZE];
    for (int32 i = 0; i < SIMD_TEST_SIZE; ++i) {
        data[i] = i * 3 - 7;
    }

    int32 reference = 0;
    for (int32 i = 0; i < 16; ++i) {
        reference += data[i];
    }

    TEST_EQUALS(simd_sum(data, 16, 8), reference);
    TEST_EQUALS(simd_sum_unaligned(data, 16, 8), reference);

    // Odd sizes
    TEST_EQUALS(simd_sum(data, 1, 8), data[0]);
    TEST_EQUALS(simd_sum(data, 3, 8), data[0] + data[1] + data[2]);
    TEST_EQUALS(simd_sum(data, 13, 4), simd_sum(data, 13, 1));

    // Negative values only
    alignas(32) int32 negative[8] = {-1, -2, -3, -4, -5, -6, -7, -8};
    TEST_EQUALS(simd_sum(negative, 8, 8), -36);

    // steps == 1 -> scalar path
    TEST_EQUALS(simd_sum(data, 16, 1), reference);
}

static void test_simd_f32_mult() {
    alignas(32) f32 a[16];
    alignas(32) f32 b[16];
    alignas(32) f32 result[16] = {0};
    alignas(32) f32 expected[16] = {0};

    for (int32 i = 0; i < 16; ++i) {
        a[i] = (f32) (i + 1);
        b[i] = (f32) (i * 2 + 1);
        expected[i] = a[i] * b[i];
    }

    simd_mult(a, b, result, 16);
    for (int32 i = 0; i < 16; ++i) {
        TEST_EQUALS_WITH_DELTA(result[i], expected[i], 0.0001);
    }

    // Scalar multiplier
    for (int32 i = 0; i < 16; ++i) {
        expected[i] = a[i] * 2.5f;
        result[i] = 0.0f;
    }

    simd_mult(a, 2.5f, result, 16);
    for (int32 i = 0; i < 16; ++i) {
        TEST_EQUALS_WITH_DELTA(result[i], expected[i], 0.0001);
    }

    // Size that is not a multiple of the vector width
    for (int32 i = 0; i < 16; ++i) {
        result[i] = -1.0f;
    }

    simd_mult(a, a, result, 7);
    for (int32 i = 0; i < 7; ++i) {
        TEST_EQUALS_WITH_DELTA(result[i], a[i] * a[i], 0.0001);
    }

    // The bytes behind the size must not be touched
    TEST_EQUALS_WITH_DELTA(result[7], -1.0f, 0.0001);

    // Single element
    result[0] = 0.0f;
    simd_mult(a, b, result, 1);
    TEST_EQUALS_WITH_DELTA(result[0], a[0] * b[0], 0.0001);

    // Zero size does nothing
    result[0] = 42.0f;
    simd_mult(a, b, result, 0);
    TEST_EQUALS_WITH_DELTA(result[0], 42.0f, 0.0001);
}

static void test_simd_f32_div() {
    alignas(32) f32 a[16];
    alignas(32) f32 result[16] = {0};

    for (int32 i = 0; i < 16; ++i) {
        a[i] = (f32) (i + 1) * 4.0f;
    }

    simd_div(a, 4.0f, result, 16);
    for (int32 i = 0; i < 16; ++i) {
        TEST_EQUALS_WITH_DELTA(result[i], (f32) (i + 1), 0.0001);
    }

    // Negative divisor
    simd_div(a, -2.0f, result, 16);
    for (int32 i = 0; i < 16; ++i) {
        TEST_EQUALS_WITH_DELTA(result[i], -((f32) (i + 1) * 2.0f), 0.0001);
    }

    // Non multiple size
    for (int32 i = 0; i < 16; ++i) {
        result[i] = -1.0f;
    }

    simd_div(a, 8.0f, result, 5);
    TEST_EQUALS_WITH_DELTA(result[0], 0.5f, 0.0001);
    TEST_EQUALS_WITH_DELTA(result[4], 2.5f, 0.0001);
    TEST_EQUALS_WITH_DELTA(result[5], -1.0f, 0.0001);
}

static void test_simd_i32_add_mult_div() {
    alignas(32) int32 a[16];
    alignas(32) int32 b[16];
    alignas(32) int32 add_result[16] = {0};
    alignas(32) f32 mult_result[16] = {0};
    alignas(32) int32 int_mult_result[16] = {0};
    alignas(32) f32 div_result[16] = {0};
    alignas(32) f32 floats[16];

    for (int32 i = 0; i < 16; ++i) {
        a[i] = i + 1;
        b[i] = (i + 1) * 10;
        floats[i] = (f32) (i + 1) * 0.5f;
    }

    // Integer addition
    simd_add(a, b, add_result, 16);
    for (int32 i = 0; i < 16; ++i) {
        TEST_EQUALS(add_result[i], a[i] + b[i]);
    }

    // Integer * float -> float
    simd_mult(a, floats, mult_result, 16);
    for (int32 i = 0; i < 16; ++i) {
        TEST_EQUALS_WITH_DELTA(mult_result[i], (f32) a[i] * floats[i], 0.0001);
    }

    // Integer * float -> int (truncated)
    simd_mult(a, floats, int_mult_result, 16);
    for (int32 i = 0; i < 16; ++i) {
        TEST_EQUALS(int_mult_result[i], (int32) ((f32) a[i] * floats[i]));
    }

    // Integer * scalar float -> int
    simd_mult(a, 1.5f, int_mult_result, 16);
    for (int32 i = 0; i < 16; ++i) {
        TEST_EQUALS(int_mult_result[i], (int32) ((f32) a[i] * 1.5f));
    }

    // Integer / float -> float
    simd_div(a, 4.0f, div_result, 16);
    for (int32 i = 0; i < 16; ++i) {
        TEST_EQUALS_WITH_DELTA(div_result[i], (f32) a[i] / 4.0f, 0.0001);
    }

    // Size smaller than the vector width only touches the first elements
    for (int32 i = 0; i < 16; ++i) {
        add_result[i] = -1;
    }

    simd_add(a, b, add_result, 3);
    TEST_EQUALS(add_result[0], a[0] + b[0]);
    TEST_EQUALS(add_result[2], a[2] + b[2]);
    TEST_EQUALS(add_result[3], -1);
}

static void test_simd_equal() {
    alignas(32) byte a[32];
    alignas(32) byte b[32];

    for (int32 i = 0; i < 32; ++i) {
        a[i] = (byte) (i * 7);
        b[i] = (byte) (i * 7);
    }

    TEST_TRUE(simd_equal(a, b, 32));
    TEST_TRUE(simd_equal(a, b, 16));
    TEST_TRUE(simd_equal(a, b, 1));
    TEST_TRUE(simd_equal(a, b, 0));

    // Difference at the very end
    b[31] = (byte) (b[31] + 1);
    TEST_FALSE(simd_equal(a, b, 32));
    TEST_TRUE(simd_equal(a, b, 31));
    b[31] = a[31];

    // Difference at the beginning
    b[0] = (byte) (b[0] ^ 0xFF);
    TEST_FALSE(simd_equal(a, b, 32));
    TEST_FALSE(simd_equal(a, b, 4));
    TEST_FALSE(simd_equal(a, b, 1));
    b[0] = a[0];

    // Non multiple of the vector width
    b[5] = (byte) (b[5] ^ 0xFF);
    TEST_FALSE(simd_equal(a, b, 7));
    TEST_TRUE(simd_equal(a, b, 5));
    b[5] = a[5];

    TEST_TRUE(simd_equal(a, b, 32));
}

static void test_simd_endian_swap() {
    const int32 int_values[4] = {0x11223344, 0x01020304, -1, 0x7F000001};
    int32 int_result[4] = {0};

    endian_swap(int_values, int_result, 4, 4);
    TEST_EQUALS((uint32) int_result[0], 0x44332211);
    TEST_EQUALS((uint32) int_result[1], 0x04030201);
    TEST_EQUALS((uint32) int_result[2], 0xFFFFFFFF);
    TEST_EQUALS((uint32) int_result[3], 0x0100007F);

    // Swapping twice returns the original values
    int32 double_swap[4] = {0};
    endian_swap(int_result, double_swap, 4, 4);
    TEST_MEMORY_EQUALS(int_values, double_swap, sizeof(int_values));

    const uint32 uint_values[3] = {0xDEADBEEF, 0x00000001, 0x80000000};
    uint32 uint_result[3] = {0};

    endian_swap(uint_values, uint_result, 3, 4);
    TEST_EQUALS(uint_result[0], 0xEFBEADDE);
    TEST_EQUALS(uint_result[1], 0x01000000);
    TEST_EQUALS(uint_result[2], 0x00000080);

    const uint16 u16_values[2] = {0x1234, 0xABCD};
    uint16 u16_result[2] = {0};

    endian_swap(u16_values, u16_result, 2, 2);
    TEST_EQUALS(u16_result[0], 0x3412);
    TEST_EQUALS(u16_result[1], 0xCDAB);

    const int16 i16_values[2] = {0x1234, -2};
    int16 i16_result[2] = {0};

    endian_swap(i16_values, i16_result, 2, 2);
    TEST_EQUALS((uint16) i16_result[0], 0x3412);
    TEST_EQUALS((uint16) i16_result[1], 0xFEFF);

    // An element count that isn't a multiple of the step size still swaps everything
    const uint16 tail_values[5] = {0x0001, 0x0002, 0x0003, 0x0004, 0x0005};
    uint16 tail_result[5] = {0};

    endian_swap(tail_values, tail_result, 5, 2);
    TEST_EQUALS(tail_result[0], 0x0100);
    TEST_EQUALS(tail_result[3], 0x0400);
    TEST_EQUALS(tail_result[4], 0x0500);
}

static void test_simd_intrinsic_helpers() {
    // intrin_validate_steps has to report the alignment based step size
    alignas(64) byte aligned_64[64] = {0};
    byte unaligned[64] = {0};

    TEST_EQUALS(intrin_validate_steps(aligned_64, 16), 16);
    TEST_EQUALS(intrin_validate_steps(aligned_64, 8), 8);
    TEST_EQUALS(intrin_validate_steps(aligned_64, 4), 4);
    TEST_EQUALS(intrin_validate_steps(aligned_64, 1), 1);

    // Only alignment is checked, not the requested step size
    TEST_EQUALS(intrin_validate_steps(aligned_64 + 1, 16), 1);
    TEST_EQUALS(intrin_validate_steps(unaligned + 1, 8), 1);

    // 16 byte aligned but not 32/64
    TEST_EQUALS(intrin_validate_steps(aligned_64 + 16, 16), 4);
    TEST_EQUALS(intrin_validate_steps(aligned_64 + 32, 16), 8);

    // 4 byte aligned is not enough for any vector width
    TEST_EQUALS(intrin_validate_steps(aligned_64 + 4, 16), 1);

    // The potential step size is a compile time constant
    TEST_TRUE(SIMD_POTENTIAL_STEP >= 1);
    TEST_TRUE(SIMD_MAX_STEP_SIZE(16) <= 16);
}

static void test_simd_scalar_reference() {
    // Bigger mixed data set compared against a plain scalar implementation
    alignas(32) f32 data[32];
    alignas(32) f32 factors[32];
    alignas(32) f32 product[32] = {0};

    f32 scalar_sum = 0.0f;
    for (int32 i = 0; i < 32; ++i) {
        data[i] = (f32) ((i % 7) - 3) * 1.25f;
        factors[i] = (f32) ((i % 5) + 1) * 0.5f;
        scalar_sum += data[i];
    }

    simd_mult(data, factors, product, 32);
    for (int32 i = 0; i < 32; ++i) {
        TEST_EQUALS_WITH_DELTA(product[i], data[i] * factors[i], 0.0001);
    }

    TEST_EQUALS_WITH_DELTA(simd_sum(data, 32, 8), scalar_sum, 0.001);

    // The sum of a multiplied array equals the scalar sum of the products
    f32 product_sum = 0.0f;
    for (int32 i = 0; i < 32; ++i) {
        product_sum += product[i];
    }

    TEST_EQUALS_WITH_DELTA(simd_sum(product, 32, 8), product_sum, 0.001);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main StdlibSimdTest
#endif

int main() {
    TEST_INIT(128);

    TEST_RUN(test_simd_f32_sum);
    TEST_RUN(test_simd_i32_sum);
    TEST_RUN(test_simd_f32_mult);
    TEST_RUN(test_simd_f32_div);
    TEST_RUN(test_simd_i32_add_mult_div);
    TEST_RUN(test_simd_equal);
    TEST_RUN(test_simd_endian_swap);
    TEST_RUN(test_simd_intrinsic_helpers);
    TEST_RUN(test_simd_scalar_reference);

    TEST_FINALIZE();

    return 0;
}

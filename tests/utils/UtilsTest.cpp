#include "../TestFramework.h"
#include "../../utils/Utils.h"

static void test_is_equal() {
    const uint8_t region1[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    const uint8_t region2[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    const uint8_t region3[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x09};

    // Test equal regions
    TEST_TRUE(is_equal(region1, region2, sizeof(region1)));

    // Test unequal regions
    TEST_FALSE(is_equal(region1, region3, sizeof(region1)));

    // Test partial equality
    TEST_TRUE(is_equal(region1, region2, 4));

    // Test unequal partial regions
    TEST_FALSE(is_equal(region1 + 4, region3 + 4, 4));

    // Test smaller than 4 bytes
    TEST_TRUE(is_equal(region1, region2, 2));

    // Test empty regions
    TEST_TRUE(is_equal(region1, region2, 0));
}

static void test_is_empty() {
    const uint8_t region1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    const uint8_t region2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    const uint8_t region3[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    const uint8_t region4[] = {0x01, 0x02, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00};

    // Test empty region
    TEST_TRUE(is_empty(region1, sizeof(region1)));

    // Test non-empty region
    TEST_FALSE(is_empty(region2, sizeof(region2)));

    // Test partial empty region
    TEST_TRUE(is_empty(region1, 4));

    // Test partial non-empty region
    TEST_FALSE(is_empty(region2 + 4, 4));

    // Test smaller than 4 bytes
    TEST_TRUE(is_empty(region3, sizeof(region3)));

    // Test non-empty smaller than 4 bytes
    TEST_FALSE(is_empty(region4, sizeof(region4)));

    // Test empty region with size 0
    TEST_TRUE(is_empty(region1, 0));
}

// Every alignment/length combination has to behave like memcmp
static void test_is_equal_unaligned() {
    alignas(64) uint8_t buffer1[256];
    alignas(64) uint8_t buffer2[256];
    alignas(64) uint8_t zero[256];

    memset(buffer1, 0x5A, sizeof(buffer1));
    memset(buffer2, 0x5A, sizeof(buffer2));
    memset(zero, 0, sizeof(zero));

    for (int32 offset = 0; offset < 8; ++offset) {
        for (int32 length = 1; length <= 64; length += 7) {
            TEST_EQUALS(
                is_equal(buffer1 + offset, buffer2 + offset, length),
                memcmp(buffer1 + offset, buffer2 + offset, length) == 0
            );

            // Non zero data is never empty
            TEST_FALSE(is_empty(buffer1 + offset, length));

            // Zero data is always empty (the SIMD paths are alignment dependent)
            TEST_TRUE(is_empty(zero + offset, length));

            // One changed byte has to be detected by both functions
            buffer2[offset + length - 1] ^= 0xFF;

            TEST_FALSE(is_equal(buffer1 + offset, buffer2 + offset, length));
            TEST_FALSE(is_empty(buffer2 + offset, length));

            buffer2[offset + length - 1] ^= 0xFF;
        }
    }
}

static void test_swap_memory() {
    // Size is a multiple of sizeof(size_t)
    {
        alignas(8) byte a[32];
        alignas(8) byte b[32];
        byte a_copy[32];
        byte b_copy[32];

        for (int32 i = 0; i < 32; ++i) {
            a[i] = (byte) (i + 1);
            b[i] = (byte) (0x80 + i);
        }

        memcpy(a_copy, a, sizeof(a));
        memcpy(b_copy, b, sizeof(b));

        swap_memory(a, b, sizeof(a));

        TEST_MEMORY_EQUALS(a, b_copy, sizeof(a));
        TEST_MEMORY_EQUALS(b, a_copy, sizeof(b));
    }

    // Size with a remainder (tail is swapped byte wise)
    {
        byte a[13];
        byte b[13];
        byte a_copy[13];
        byte b_copy[13];

        for (int32 i = 0; i < 13; ++i) {
            a[i] = (byte) (i * 3);
            b[i] = (byte) (0xF0 - i);
        }

        memcpy(a_copy, a, sizeof(a));
        memcpy(b_copy, b, sizeof(b));

        swap_memory(a, b, sizeof(a));

        TEST_MEMORY_EQUALS(a, b_copy, sizeof(a));
        TEST_MEMORY_EQUALS(b, a_copy, sizeof(b));
    }

    // Size 0 doesn't change anything
    {
        byte a[4] = {1, 2, 3, 4};
        byte b[4] = {5, 6, 7, 8};

        swap_memory(a, b, 0);

        TEST_EQUALS(a[0], 1);
        TEST_EQUALS(b[0], 5);
    }
}

#if PERFORMANCE_TEST
static void _is_equal(volatile void* val) {
    volatile bool* res = (volatile bool *) val;

    uint8_t region1[64];
    uint8_t region2[64];
    memset(region1, 0xAA, sizeof(region1));
    memset(region2, 0xAA, sizeof(region2));

    *res |= is_equal(region1, region2, sizeof(region1));
}

static void _memcmp(volatile void* val) {
    volatile bool* res = (volatile bool *) val;

    uint8_t region1[64];
    uint8_t region2[64];
    memset(region1, 0xAA, sizeof(region1));
    memset(region2, 0xAA, sizeof(region2));

    *res |= (bool) (memcmp(region1, region2, sizeof(region1)) == 0);
}

static void test_is_equal_performance() {
    COMPARE_FUNCTION_TEST_TIME(_is_equal, _memcmp, 10.0);
    COMPARE_FUNCTION_TEST_CYCLE(_is_equal, _memcmp, 10.0);
}

static void _is_empty(volatile void* val) {
    volatile bool* res = (volatile bool *) val;

    alignas(64) uint8_t region1[64];
    memset(region1, 0xAA, sizeof(region1));

    *res |= is_empty(region1, sizeof(region1));
}

static void _memcmp_empty(volatile void* val) {
    volatile bool* res = (volatile bool *) val;

    alignas(64) uint8_t region1[64];
    memset(region1, 0xAA, sizeof(region1));

    *res |= *region1 == 0 && memcmp(region1, region1 + 1, sizeof(region1) - 1) == 0;
}

static void _is_empty2(volatile void* val) {
    volatile bool* res = (volatile bool *) val;

    alignas(64) uint8_t region1[64];
    memset(region1, 0, sizeof(region1));

    *res |= is_empty(region1, sizeof(region1));
}

static void _memcmp_empty2(volatile void* val) {
    volatile bool* res = (volatile bool *) val;

    alignas(64) uint8_t region1[64];
    memset(region1, 0, sizeof(region1));

    *res |= *region1 == 0 && memcmp(region1, region1 + 1, sizeof(region1) - 1) == 0;
}

static void test_is_empty_performance() {
    COMPARE_FUNCTION_TEST_TIME(_is_empty, _memcmp_empty, 10.0);
    COMPARE_FUNCTION_TEST_CYCLE(_is_empty, _memcmp_empty, 10.0);

    COMPARE_FUNCTION_TEST_TIME(_is_empty2, _memcmp_empty2, 10.0);
    COMPARE_FUNCTION_TEST_CYCLE(_is_empty2, _memcmp_empty2, 10.0);
}
#endif

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UtilsUtilsTest
#endif

int main() {
    TEST_INIT(10);

    TEST_RUN(test_is_equal);
    TEST_RUN(test_is_empty);
    TEST_RUN(test_is_equal_unaligned);
    TEST_RUN(test_swap_memory);

    #if PERFORMANCE_TEST
        TEST_RUN(test_is_equal_performance);
        TEST_RUN(test_is_empty_performance);
    #endif

    TEST_FINALIZE();

    return 0;
}
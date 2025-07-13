#include "../TestFramework.h"
#include "../../utils/Utils.h"

static void test_is_equal() {
    uint8_t region1[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint8_t region2[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    uint8_t region3[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x09};

    // Test equal regions
    ASSERT_TRUE(is_equal(region1, region2, sizeof(region1)));

    // Test unequal regions
    ASSERT_FALSE(is_equal(region1, region3, sizeof(region1)));

    // Test partial equality
    ASSERT_TRUE(is_equal(region1, region2, 4));

    // Test unequal partial regions
    ASSERT_FALSE(is_equal(region1 + 4, region3 + 4, 4));

    // Test smaller than 4 bytes
    ASSERT_TRUE(is_equal(region1, region2, 2));

    // Test empty regions
    ASSERT_TRUE(is_equal(region1, region2, 0));
}

static void test_is_empty() {
    uint8_t region1[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t region2[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
    uint8_t region3[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    uint8_t region4[] = {0x01, 0x02, 0x03, 0x04, 0x00, 0x00, 0x00, 0x00};

    // Test empty region
    ASSERT_TRUE(is_empty(region1, sizeof(region1)));

    // Test non-empty region
    ASSERT_FALSE(is_empty(region2, sizeof(region2)));

    // Test partial empty region
    ASSERT_TRUE(is_empty(region1, 4));

    // Test partial non-empty region
    ASSERT_FALSE(is_empty(region2 + 4, 4));

    // Test smaller than 4 bytes
    ASSERT_TRUE(is_empty(region3, sizeof(region3)));

    // Test non-empty smaller than 4 bytes
    ASSERT_FALSE(is_empty(region4, sizeof(region4)));

    // Test empty region with size 0
    ASSERT_TRUE(is_empty(region1, 0));
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

    #if PERFORMANCE_TEST
        TEST_RUN(test_is_equal_performance);
        TEST_RUN(test_is_empty_performance);
    #endif

    TEST_FINALIZE();

    return 0;
}
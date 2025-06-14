#include <string.h>
#include "../TestFramework.h"
#include "../../utils/EndianUtils.h"

static void test_swap_endian_16() {
    uint16_t val = 0x1234;
    uint16_t swapped = SWAP_ENDIAN_16(val);
    ASSERT_EQUALS(0x3412, swapped);

    val = 0xABCD;
    swapped = SWAP_ENDIAN_16(val);
    ASSERT_EQUALS(0xCDAB, swapped);
}

static void test_swap_endian_32() {
    uint32_t val = 0x12345678;
    uint32_t swapped = SWAP_ENDIAN_32(val);
    ASSERT_EQUALS(0x78563412, swapped);

    val = 0xABCDEF01;
    swapped = SWAP_ENDIAN_32(val);
    ASSERT_EQUALS(0x01EFCDAB, swapped);
}

static void test_swap_endian_64() {
    uint64_t val = 0x123456789ABCDEF0;
    uint64_t swapped = SWAP_ENDIAN_64(val);
    ASSERT_EQUALS(0xF0DEBC9A78563412, swapped);

    val = 0xAABBCCDDEEFF1122;
    swapped = SWAP_ENDIAN_64(val);
    ASSERT_EQUALS(0x2211FFEEDDCCBBAA, swapped);
}

static void test_is_little_endian() {
    bool is_little = is_little_endian();
    #if _WIN32 || __LITTLE_ENDIAN__
        ASSERT_TRUE(is_little);
    #else
        ASSERT_FALSE(is_little);
    #endif
}

static void test_endian_swap_uint16() {
    uint16 val = 0x1234;
    uint16 swapped = endian_swap(val);
    ASSERT_EQUALS(0x3412, swapped);

    val = 0xABCD;
    swapped = endian_swap(val);
    ASSERT_EQUALS(0xCDAB, swapped);
}

static void test_endian_swap_int16() {
    int16 val = 0x1234;
    int16 swapped = endian_swap(val);
    ASSERT_EQUALS(0x3412, swapped);
}

static void test_endian_swap_uint32() {
    uint32 val = 0x12345678;
    uint32 swapped = endian_swap(val);
    ASSERT_EQUALS(0x78563412, swapped);

    val = 0xABCDEF01;
    swapped = endian_swap(val);
    ASSERT_EQUALS(0x01EFCDAB, swapped);
}

static void test_endian_swap_int32() {
    int32 val = 0x12345678;
    int32 swapped = endian_swap(val);
    ASSERT_EQUALS(0x78563412, swapped);
}

static void test_endian_swap_uint64() {
    uint64 val = 0x123456789ABCDEF0;
    uint64 swapped = endian_swap(val);
    ASSERT_EQUALS(0xF0DEBC9A78563412, swapped);

    val = 0xAABBCCDDEEFF1122;
    swapped = endian_swap(val);
    ASSERT_EQUALS(0x2211FFEEDDCCBBAA, swapped);
}

static void test_endian_swap_int64() {
    int64 val = 0x123456789ABCDEF0;
    int64 swapped = endian_swap(val);
    ASSERT_EQUALS((int64) 0xF0DEBC9A78563412, swapped);
}

static void test_endian_swap_float() {
    float val = 1234.5678f;
    uint32_t val_as_int;
    memcpy(&val_as_int, &val, sizeof(val_as_int));
    val_as_int = endian_swap(val_as_int);
    float swapped;
    memcpy(&swapped, &val_as_int, sizeof(swapped));
    ASSERT_TRUE(swapped != val); // Ensure the value has changed due to endian swap
}

static void test_endian_swap_double() {
    double val = 1234.5678;
    uint64_t val_as_int;
    memcpy(&val_as_int, &val, sizeof(val_as_int));
    val_as_int = endian_swap(val_as_int);
    double swapped;
    memcpy(&swapped, &val_as_int, sizeof(swapped));
    ASSERT_TRUE(swapped != val); // Ensure the value has changed due to endian swap
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UtilsEndianUtilsTest
#endif

int main() {
    TEST_INIT(50);

    TEST_RUN(test_swap_endian_16);
    TEST_RUN(test_swap_endian_32);
    TEST_RUN(test_swap_endian_64);

    TEST_RUN(test_is_little_endian);
    TEST_RUN(test_endian_swap_uint16);
    TEST_RUN(test_endian_swap_int16);
    TEST_RUN(test_endian_swap_uint32);
    TEST_RUN(test_endian_swap_int32);
    TEST_RUN(test_endian_swap_uint64);
    TEST_RUN(test_endian_swap_int64);
    TEST_RUN(test_endian_swap_float);
    TEST_RUN(test_endian_swap_double);

    TEST_FINALIZE();

    return 0;
}

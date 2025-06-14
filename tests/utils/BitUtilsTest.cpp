#include "../TestFramework.h"
#include "../../utils/BitUtils.h"

static void test_is_bit_set_l2r()
{
    ASSERT_FALSE_CONST(IS_BIT_SET_L2R((uint8_t) 0b01000000, 0, sizeof(uint8_t) * 8));
    ASSERT_TRUE_CONST(IS_BIT_SET_L2R((uint8_t) 0b10000000, 0, sizeof(uint8_t) * 8));
    ASSERT_TRUE_CONST(IS_BIT_SET_L2R((uint8_t) 0b01000000, 1, sizeof(uint8_t) * 8));
    ASSERT_FALSE_CONST(IS_BIT_SET_L2R((uint8_t) 0b01000000, 2, sizeof(uint8_t) * 8));
}

static void test_bit_set_l2r()
{
    uint8_t num = 0b00000000;
    ASSERT_TRUE(IS_BIT_SET_L2R(BIT_SET_L2R(num, 0, sizeof(uint8_t) * 8), 0, sizeof(uint8_t) * 8));
    ASSERT_TRUE(IS_BIT_SET_L2R(BIT_SET_L2R(num, 1, sizeof(uint8_t) * 8), 1, sizeof(uint8_t) * 8));
    ASSERT_FALSE(IS_BIT_SET_L2R(BIT_SET_L2R(num, 1, sizeof(uint8_t) * 8), 0, sizeof(uint8_t) * 8));
}

static void test_bit_unset_l2r() {
    uint8_t num = 0b11111111;
    ASSERT_FALSE(IS_BIT_SET_L2R(BIT_UNSET_L2R(num, 0, sizeof(uint8_t) * 8), 0, sizeof(uint8_t) * 8));
    ASSERT_FALSE(IS_BIT_SET_L2R(BIT_UNSET_L2R(num, 1, sizeof(uint8_t) * 8), 1, sizeof(uint8_t) * 8));
    ASSERT_TRUE(IS_BIT_SET_L2R(BIT_UNSET_L2R(num, 1, sizeof(uint8_t) * 8), 0, sizeof(uint8_t) * 8));
}

static void test_bit_flip_l2r() {
    uint8_t num = 0b10101010;
    ASSERT_FALSE(IS_BIT_SET_L2R(BIT_FLIP_L2R(num, 0, sizeof(uint8_t) * 8), 0, sizeof(uint8_t) * 8));
    ASSERT_TRUE(IS_BIT_SET_L2R(BIT_FLIP_L2R(num, 1, sizeof(uint8_t) * 8), 1, sizeof(uint8_t) * 8));
}

static void test_bit_set_to_l2r() {
    uint8_t num = 0b00000000;
    ASSERT_TRUE(IS_BIT_SET_L2R(BIT_SET_TO_L2R(num, 0, 1, sizeof(uint8_t) * 8), 0, sizeof(uint8_t) * 8));
    ASSERT_FALSE(IS_BIT_SET_L2R(BIT_SET_TO_L2R(num, 1, 0, sizeof(uint8_t) * 8), 1, sizeof(uint8_t) * 8));
}

static void test_bits_get_8_l2r() {
    uint8_t num = 0b11001100;
    ASSERT_TRUE(BITS_GET_8_L2R(num, 0, 2) == 0b11);
    ASSERT_TRUE(BITS_GET_8_L2R(num, 2, 2) == 0b00);
}

static void test_bits_get_16_l2r() {
    uint16_t num = 0b1100110011001100;
    ASSERT_TRUE(BITS_GET_16_L2R(num, 0, 4) == 0b1100);
    ASSERT_TRUE(BITS_GET_16_L2R(num, 4, 4) == 0b1100);
}

static void test_bits_get_32_l2r() {
    uint32_t num = 0b11001100110011001100110011001100;
    ASSERT_TRUE(BITS_GET_32_L2R(num, 0, 8) == 0b11001100);
    ASSERT_TRUE(BITS_GET_32_L2R(num, 8, 8) == 0b11001100);
}

static void test_bits_get_64_l2r() {
    uint64_t num = 0b1100110011001100110011001100110011001100110011001100110011001100;
    ASSERT_TRUE(BITS_GET_64_L2R(num, 0, 16) == 0b1100110011001100);
    ASSERT_TRUE(BITS_GET_64_L2R(num, 16, 16) == 0b1100110011001100);
}

static void test_bytes_merge_2_l2r() {
    uint8_t arr[] = {0x12, 0x34};
    ASSERT_TRUE(BYTES_MERGE_2_L2R(arr) == 0x1234);
}

static void test_bytes_merge_4_l2r() {
    uint8_t arr[] = {0x12, 0x34, 0x56, 0x78};
    ASSERT_TRUE(BYTES_MERGE_4_L2R(arr) == 0x12345678);
}

static void test_bytes_merge_8_l2r() {
    uint8_t arr[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    ASSERT_TRUE(BYTES_MERGE_8_L2R(arr) == 0x123456789ABCDEF0);
}

static void test_is_bit_set_r2l() {
    ASSERT_FALSE_CONST(IS_BIT_SET_R2L((uint8_t) 0b00000010, 0));
    ASSERT_TRUE_CONST(IS_BIT_SET_R2L((uint8_t) 0b00000001, 0));
    ASSERT_TRUE_CONST(IS_BIT_SET_R2L((uint8_t) 0b00000010, 1));
    ASSERT_FALSE_CONST(IS_BIT_SET_R2L((uint8_t) 0b00000010, 2));
}

static void test_is_bit_set_64_r2l() {
    ASSERT_FALSE_CONST(IS_BIT_SET_64_R2L((uint64_t) 0b00000010, 0));
    ASSERT_TRUE_CONST(IS_BIT_SET_64_R2L((uint64_t) 0b00000001, 0));
    ASSERT_TRUE_CONST(IS_BIT_SET_64_R2L((uint64_t) 0b00000010, 1));
    ASSERT_FALSE_CONST(IS_BIT_SET_64_R2L((uint64_t) 0b00000010, 2));
}

static void test_bit_set_r2l() {
    uint8_t num = 0b00000000;
    ASSERT_TRUE(IS_BIT_SET_R2L(BIT_SET_R2L(num, 0), 0));
    ASSERT_TRUE(IS_BIT_SET_R2L(BIT_SET_R2L(num, 1), 1));
    ASSERT_FALSE(IS_BIT_SET_R2L(BIT_SET_R2L(num, 1), 0));
}

static void test_bit_unset_r2l() {
    uint8_t num = 0b11111111;
    ASSERT_FALSE(IS_BIT_SET_R2L(BIT_UNSET_R2L(num, 0), 0));
    ASSERT_FALSE(IS_BIT_SET_R2L(BIT_UNSET_R2L(num, 1), 1));
    ASSERT_TRUE(IS_BIT_SET_R2L(BIT_UNSET_R2L(num, 1), 0));
}

static void test_bit_flip_r2l() {
    uint8_t num = 0b10101010;
    ASSERT_TRUE(IS_BIT_SET_R2L(BIT_FLIP_R2L(num, 0), 0));
    ASSERT_FALSE(IS_BIT_SET_R2L(BIT_FLIP_R2L(num, 1), 1));
}

static void test_bit_set_to_r2l() {
    uint8_t num = 0b00000000;
    ASSERT_TRUE(IS_BIT_SET_R2L(BIT_SET_TO_R2L(num, 0, 1), 0));
    ASSERT_FALSE(IS_BIT_SET_R2L(BIT_SET_TO_R2L(num, 1, 0), 1));
}

static void test_bits_get_8_r2l() {
    uint8_t num = 0b11001100;
    ASSERT_TRUE(BITS_GET_8_R2L(num, 0, 2) == 0b00);
    ASSERT_TRUE(BITS_GET_8_R2L(num, 2, 2) == 0b11);
}

static void test_bits_get_16_r2l() {
    uint16_t num = 0b1100110011001100;
    ASSERT_TRUE(BITS_GET_16_R2L(num, 0, 4) == 0b1100);
    ASSERT_TRUE(BITS_GET_16_R2L(num, 4, 4) == 0b1100);
}

static void test_bits_get_32_r2l() {
    uint32_t num = 0b11001100110011001100110011001100;
    ASSERT_TRUE(BITS_GET_32_R2L(num, 0, 8) == 0b11001100);
    ASSERT_TRUE(BITS_GET_32_R2L(num, 8, 8) == 0b11001100);
}

static void test_bits_get_64_r2l() {
    uint64_t num = 0b1100110011001100110011001100110011001100110011001100110011001100;
    ASSERT_TRUE(BITS_GET_64_R2L(num, 0, 16) == 0b1100110011001100);
    ASSERT_TRUE(BITS_GET_64_R2L(num, 16, 16) == 0b1100110011001100);
    ASSERT_FALSE(BITS_GET_64_R2L(num, 15, 16) == 0b1100110011001100);
}

static void test_bytes_merge_2_r2l() {
    uint8_t arr[] = {0x12, 0x34};
    ASSERT_TRUE(BYTES_MERGE_2_R2L(arr) == 0x3412);
}

static void test_bytes_merge_4_r2l() {
    uint8_t arr[] = {0x12, 0x34, 0x56, 0x78};
    ASSERT_TRUE(BYTES_MERGE_4_R2L(arr) == 0x78563412);
}

static void test_bytes_merge_8_r2l() {
    uint8_t arr[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    ASSERT_TRUE(BYTES_MERGE_8_R2L(arr) == 0xF0DEBC9A78563412);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UtilsBitUtilsTest
#endif

int main() {
    TEST_INIT(75);

    TEST_RUN(test_is_bit_set_l2r);
    TEST_RUN(test_bit_set_l2r);
    TEST_RUN(test_bit_unset_l2r);
    TEST_RUN(test_bit_flip_l2r);
    TEST_RUN(test_bit_set_to_l2r);
    TEST_RUN(test_bits_get_8_l2r);
    TEST_RUN(test_bits_get_16_l2r);
    TEST_RUN(test_bits_get_32_l2r);
    TEST_RUN(test_bits_get_64_l2r);
    TEST_RUN(test_bytes_merge_2_l2r);
    TEST_RUN(test_bytes_merge_4_l2r);
    TEST_RUN(test_bytes_merge_8_l2r);

    TEST_RUN(test_is_bit_set_r2l);
    TEST_RUN(test_is_bit_set_64_r2l);
    TEST_RUN(test_bit_set_r2l);
    TEST_RUN(test_bit_unset_r2l);
    TEST_RUN(test_bit_flip_r2l);
    TEST_RUN(test_bit_set_to_r2l);
    TEST_RUN(test_bits_get_8_r2l);
    TEST_RUN(test_bits_get_16_r2l);
    TEST_RUN(test_bits_get_32_r2l);
    TEST_RUN(test_bits_get_64_r2l);
    TEST_RUN(test_bytes_merge_2_r2l);
    TEST_RUN(test_bytes_merge_4_r2l);
    TEST_RUN(test_bytes_merge_8_r2l);

    TEST_FINALIZE();

    return 0;
}
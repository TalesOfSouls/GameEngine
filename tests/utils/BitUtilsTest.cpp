#include "../TestFramework.h"
#include "../../utils/BitUtils.h"

static void test_is_bit_set_l2r()
{
    TEST_FALSE_CONST(IS_BIT_SET_L2R((uint8_t) 0b01000000, 0, sizeof(uint8_t) * 8));
    TEST_TRUE_CONST(IS_BIT_SET_L2R((uint8_t) 0b10000000, 0, sizeof(uint8_t) * 8));
    TEST_TRUE_CONST(IS_BIT_SET_L2R((uint8_t) 0b01000000, 1, sizeof(uint8_t) * 8));
    TEST_FALSE_CONST(IS_BIT_SET_L2R((uint8_t) 0b01000000, 2, sizeof(uint8_t) * 8));
}

static void test_bit_set_l2r()
{
    uint8_t num = 0b00000000;
    TEST_TRUE(IS_BIT_SET_L2R(BIT_SET_L2R(num, 0, sizeof(uint8_t) * 8), 0, sizeof(uint8_t) * 8));
    TEST_TRUE(IS_BIT_SET_L2R(BIT_SET_L2R(num, 1, sizeof(uint8_t) * 8), 1, sizeof(uint8_t) * 8));
    TEST_FALSE(IS_BIT_SET_L2R(BIT_SET_L2R(num, 1, sizeof(uint8_t) * 8), 0, sizeof(uint8_t) * 8));
}

static void test_bit_unset_l2r() {
    uint8_t num = 0b11111111;
    TEST_FALSE(IS_BIT_SET_L2R(BIT_UNSET_L2R(num, 0, sizeof(uint8_t) * 8), 0, sizeof(uint8_t) * 8));
    TEST_FALSE(IS_BIT_SET_L2R(BIT_UNSET_L2R(num, 1, sizeof(uint8_t) * 8), 1, sizeof(uint8_t) * 8));
    TEST_TRUE(IS_BIT_SET_L2R(BIT_UNSET_L2R(num, 1, sizeof(uint8_t) * 8), 0, sizeof(uint8_t) * 8));
}

static void test_bit_flip_l2r() {
    uint8_t num = 0b10101010;
    TEST_FALSE(IS_BIT_SET_L2R(BIT_FLIP_L2R(num, 0, sizeof(uint8_t) * 8), 0, sizeof(uint8_t) * 8));
    TEST_TRUE(IS_BIT_SET_L2R(BIT_FLIP_L2R(num, 1, sizeof(uint8_t) * 8), 1, sizeof(uint8_t) * 8));
}

static void test_bit_set_to_l2r() {
    uint8_t num = 0b00000000;
    TEST_TRUE(IS_BIT_SET_L2R(BIT_SET_TO_L2R(num, 0, 1, sizeof(uint8_t) * 8), 0, sizeof(uint8_t) * 8));
    TEST_FALSE(IS_BIT_SET_L2R(BIT_SET_TO_L2R(num, 1, 0, sizeof(uint8_t) * 8), 1, sizeof(uint8_t) * 8));
}

static void test_bits_get_8_l2r() {
    uint8_t num = 0b11001100;
    TEST_TRUE(BITS_GET_8_L2R(num, 0, 2) == 0b11);
    TEST_TRUE(BITS_GET_8_L2R(num, 2, 2) == 0b00);
}

static void test_bits_get_16_l2r() {
    uint16_t num = 0b1100110011001100;
    TEST_TRUE(BITS_GET_16_L2R(num, 0, 4) == 0b1100);
    TEST_TRUE(BITS_GET_16_L2R(num, 4, 4) == 0b1100);
}

static void test_bits_get_32_l2r() {
    uint32_t num = 0b11001100110011001100110011001100;
    TEST_TRUE(BITS_GET_32_L2R(num, 0, 8) == 0b11001100);
    TEST_TRUE(BITS_GET_32_L2R(num, 8, 8) == 0b11001100);
}

static void test_bits_get_64_l2r() {
    uint64_t num = 0b1100110011001100110011001100110011001100110011001100110011001100;
    TEST_TRUE(BITS_GET_64_L2R(num, 0, 16) == 0b1100110011001100);
    TEST_TRUE(BITS_GET_64_L2R(num, 16, 16) == 0b1100110011001100);
}

static void test_bytes_merge_2_l2r() {
    const uint8_t arr[] = {0x12, 0x34};
    TEST_TRUE(BYTES_MERGE_2_L2R(arr) == 0x1234);
}

static void test_bytes_merge_4_l2r() {
    const uint8_t arr[] = {0x12, 0x34, 0x56, 0x78};
    TEST_TRUE(BYTES_MERGE_4_L2R(arr) == 0x12345678);
}

static void test_bytes_merge_8_l2r() {
    const uint8_t arr[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    TEST_TRUE(BYTES_MERGE_8_L2R(arr) == 0x123456789ABCDEF0);
}

static void test_is_bit_set_r2l() {
    TEST_FALSE_CONST(IS_BIT_SET_R2L((uint8_t) 0b00000010, 0));
    TEST_TRUE_CONST(IS_BIT_SET_R2L((uint8_t) 0b00000001, 0));
    TEST_TRUE_CONST(IS_BIT_SET_R2L((uint8_t) 0b00000010, 1));
    TEST_FALSE_CONST(IS_BIT_SET_R2L((uint8_t) 0b00000010, 2));
}

static void test_is_bit_set_64_r2l() {
    TEST_FALSE_CONST(IS_BIT_SET_64_R2L((uint64_t) 0b00000010, 0));
    TEST_TRUE_CONST(IS_BIT_SET_64_R2L((uint64_t) 0b00000001, 0));
    TEST_TRUE_CONST(IS_BIT_SET_64_R2L((uint64_t) 0b00000010, 1));
    TEST_FALSE_CONST(IS_BIT_SET_64_R2L((uint64_t) 0b00000010, 2));
}

static void test_bit_set_r2l() {
    uint8_t num = 0b00000000;
    TEST_TRUE(IS_BIT_SET_R2L(BIT_SET_R2L(num, 0), 0));
    TEST_TRUE(IS_BIT_SET_R2L(BIT_SET_R2L(num, 1), 1));
    TEST_FALSE(IS_BIT_SET_R2L(BIT_SET_R2L(num, 1), 0));
}

static void test_bit_unset_r2l() {
    uint8_t num = 0b11111111;
    TEST_FALSE(IS_BIT_SET_R2L(BIT_UNSET_R2L(num, 0), 0));
    TEST_FALSE(IS_BIT_SET_R2L(BIT_UNSET_R2L(num, 1), 1));
    TEST_TRUE(IS_BIT_SET_R2L(BIT_UNSET_R2L(num, 1), 0));
}

static void test_bit_flip_r2l() {
    uint8_t num = 0b10101010;
    TEST_TRUE(IS_BIT_SET_R2L(BIT_FLIP_R2L(num, 0), 0));
    TEST_FALSE(IS_BIT_SET_R2L(BIT_FLIP_R2L(num, 1), 1));
}

static void test_bit_set_to_r2l() {
    uint8_t num = 0b00000000;
    TEST_TRUE(IS_BIT_SET_R2L(BIT_SET_TO_R2L(num, 0, 1), 0));
    TEST_FALSE(IS_BIT_SET_R2L(BIT_SET_TO_R2L(num, 1, 0), 1));
}

static void test_bits_get_8_r2l() {
    uint8_t num = 0b11001100;
    TEST_TRUE(BITS_GET_8_R2L(num, 0, 2) == 0b00);
    TEST_TRUE(BITS_GET_8_R2L(num, 2, 2) == 0b11);
}

static void test_bits_get_16_r2l() {
    uint16_t num = 0b1100110011001100;
    TEST_TRUE(BITS_GET_16_R2L(num, 0, 4) == 0b1100);
    TEST_TRUE(BITS_GET_16_R2L(num, 4, 4) == 0b1100);
}

static void test_bits_get_32_r2l() {
    uint32_t num = 0b11001100110011001100110011001100;
    TEST_TRUE(BITS_GET_32_R2L(num, 0, 8) == 0b11001100);
    TEST_TRUE(BITS_GET_32_R2L(num, 8, 8) == 0b11001100);
}

static void test_bits_get_64_r2l() {
    uint64_t num = 0b1100110011001100110011001100110011001100110011001100110011001100;
    TEST_TRUE(BITS_GET_64_R2L(num, 0, 16) == 0b1100110011001100);
    TEST_TRUE(BITS_GET_64_R2L(num, 16, 16) == 0b1100110011001100);
    TEST_FALSE(BITS_GET_64_R2L(num, 15, 16) == 0b1100110011001100);
}

static void test_bytes_merge_2_r2l() {
    const uint8_t arr[] = {0x12, 0x34};
    TEST_TRUE(BYTES_MERGE_2_R2L(arr) == 0x3412);
}

static void test_bytes_merge_4_r2l() {
    const uint8_t arr[] = {0x12, 0x34, 0x56, 0x78};
    TEST_TRUE(BYTES_MERGE_4_R2L(arr) == 0x78563412);
}

static void test_bytes_merge_8_r2l() {
    const uint8_t arr[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    TEST_TRUE(BYTES_MERGE_8_R2L(arr) == 0xF0DEBC9A78563412);
}

static void test_first_set_bit() {
    // 1 based, 0 = no bit set
    TEST_EQUALS(first_set_bit_r2l((int32) 0), 0);
    TEST_EQUALS(first_set_bit_r2l((int32) 1), 1);
    TEST_EQUALS(first_set_bit_r2l((int32) 2), 2);
    TEST_EQUALS(first_set_bit_r2l((int32) 0b1010), 2);
    TEST_EQUALS(first_set_bit_r2l((int32) 0b1000), 4);

    TEST_EQUALS(first_set_bit_r2l((int64) 0), 0);
    TEST_EQUALS(first_set_bit_r2l((int64) 0x100000000LL), 33);

    // Left to right: index 1 = most significant bit
    TEST_EQUALS(first_set_bit_l2r((int32) 0), 0);
    TEST_EQUALS(first_set_bit_l2r((int32) (1 << 30)), 2);
    TEST_EQUALS(first_set_bit_l2r((int32) 0b1010), 29);

    TEST_EQUALS(first_set_bit_l2r((int64) 0), 0);
    TEST_EQUALS(first_set_bit_l2r((int64) 0x8000000000000000LL), 1);
}

static void test_bits_reverse() {
    TEST_EQUALS(bits_reverse((uint32) 0b110, 3), 0b011);
    TEST_EQUALS(bits_reverse((uint32) 0b10000000000000000000000000000000, 32), 1);
    TEST_EQUALS(bits_reverse((uint32) 0xFFFFFFFF, 32), 0xFFFFFFFF);

    TEST_EQUALS(bits_reverse((uint64) 0b110, 3), 0b011);
    TEST_EQUALS(bits_reverse((uint64) 0xFFFFFFFFFFFFFFFFULL, 64), 0xFFFFFFFFFFFFFFFFULL);
}

static void test_bits_count() {
    TEST_EQUALS(bits_count((uint8_t) 0), 0);
    TEST_EQUALS(bits_count((uint8_t) 0xFF), 8);
    TEST_EQUALS(bits_count((uint8_t) 0b10101010), 4);

    TEST_EQUALS(bits_count((uint16_t) 0), 0);
    TEST_EQUALS(bits_count((uint16_t) 0xFFFF), 16);
    TEST_EQUALS(bits_count((uint16_t) 0b1010101010101010), 8);

    TEST_EQUALS(bits_count((uint32) 0), 0);
    TEST_EQUALS(bits_count((uint32) 0xFFFFFFFF), 32);
    TEST_EQUALS(bits_count((uint32) 0b1101), 3);

    TEST_EQUALS(bits_count((uint64) 0), 0);
    TEST_EQUALS(bits_count((uint64) 0xFFFFFFFFFFFFFFFFULL), 64);
    TEST_EQUALS(bits_count((uint64) 0b1101), 3);

    // lookup table and the hardware instruction have to agree
    TEST_EQUALS(bits_count((uint32) 0b1101, true), bits_count((uint32) 0b1101));
    TEST_EQUALS(bits_count((uint64) 0x123456789ABCDEF0ULL, true), bits_count((uint64) 0x123456789ABCDEF0ULL));
}

// Bit stream (MSB first, @see BitWalk)
static void test_bits_walk() {
    byte data[4] = {0};
    BitWalk stream = {data, 0};

    bits_walk(&stream, 3);
    TEST_EQUALS(stream.pos, data);
    TEST_EQUALS(stream.bit_pos, 3);

    // 3 + 6 = 9 bits -> one full byte + 1 bit
    bits_walk(&stream, 6);
    TEST_EQUALS(stream.pos, data + 1);
    TEST_EQUALS(stream.bit_pos, 1);

    bits_flush(&stream);
    TEST_EQUALS(stream.pos, data + 2);
    TEST_EQUALS(stream.bit_pos, 0);

    // Flushing an aligned stream doesn't move it
    bits_flush(&stream);
    TEST_EQUALS(stream.pos, data + 2);
    TEST_EQUALS(stream.bit_pos, 0);
}

static void test_bits_consume_8() {
    byte data[2] = {0b10110010, 0b01101100};
    BitWalk stream = {data, 0};

    TEST_EQUALS(bits_consume_8(&stream, 3), 0b101);
    TEST_EQUALS(bits_consume_8(&stream, 5), 0b10010);
    TEST_EQUALS(stream.pos, data + 1);
    TEST_EQUALS(stream.bit_pos, 0);

    TEST_EQUALS(bits_consume_8(&stream, 2), 0b01);
    TEST_EQUALS(bits_consume_8(&stream, 1), 0b1);
    TEST_EQUALS(bits_consume_8(&stream, 5), 0b01100);
    TEST_EQUALS(stream.pos, data + 2);
    TEST_EQUALS(stream.bit_pos, 0);
}

static void test_bits_consume_16() {
    byte data[4] = {0xAB, 0xCD, 0xEF, 0x01};
    BitWalk stream = {data, 0};

    TEST_EQUALS(bits_consume_16(&stream, 16), 0xABCD);
    TEST_EQUALS(stream.pos, data + 2);

    TEST_EQUALS(bits_consume_16(&stream, 16), 0xEF01);
    TEST_EQUALS(stream.pos, data + 4);
}

static void test_bits_consume_32_64() {
    byte data[8] = {0x0F, 0xF0, 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC};

    BitWalk stream = {data, 0};

    uint32 bits = bits_consume_8(&stream, 4);
    TEST_EQUALS(bits, 0x0); // high nibble of 0x0F

    bits = bits_consume_8(&stream, 4);
    TEST_EQUALS(bits, 0xF); // low nibble of 0x0F

    // 12 bits = 0xF0 + high nibble of 0x12
    bits = bits_consume_32(&stream, 12);
    TEST_EQUALS(bits, 0xF01);
    TEST_EQUALS(stream.pos, data + 2);
    TEST_EQUALS(stream.bit_pos, 4);

    bits = bits_consume_32(&stream, 12);
    TEST_EQUALS(bits, 0x234); // low nibble of 0x12 + 0x34
    TEST_EQUALS(stream.pos, data + 4);
    TEST_EQUALS(stream.bit_pos, 0);

    uint64 bits64 = bits_consume_64(&stream, 16);
    TEST_EQUALS(bits64, 0x5678);

    bits = bits_consume_32(&stream, 16);
    TEST_EQUALS(bits, 0x9ABC);
    TEST_EQUALS(stream.pos, data + 8);
    TEST_EQUALS(stream.bit_pos, 0);
}

static void test_bits_peek() {
    byte data[4] = {0xAB, 0xCD, 0xEF, 0x01};
    BitWalk stream = {data, 0};

    // Peeking doesn't consume anything
    TEST_EQUALS(bits_peek_8(&stream, 8), 0xAB);
    TEST_EQUALS(stream.pos, data);
    TEST_EQUALS(stream.bit_pos, 0);

    TEST_EQUALS(bits_peek_16(&stream, 16), 0xABCD);
    TEST_EQUALS(stream.pos, data);
    TEST_EQUALS(stream.bit_pos, 0);

    TEST_EQUALS(bits_peek_32(&stream, 32), 0xABCDEF01);
    TEST_EQUALS(bits_peek_64(&stream, 32), 0xABCDEF01);
    TEST_EQUALS(stream.pos, data);
    TEST_EQUALS(stream.bit_pos, 0);

    // Peeking respects the current position
    bits_consume_8(&stream, 4);
    TEST_EQUALS(bits_peek_8(&stream, 4), 0xB);
    TEST_EQUALS(bits_peek_8(&stream, 8), 0xBC);
    TEST_EQUALS(stream.pos, data);
    TEST_EQUALS(stream.bit_pos, 4);

    TEST_EQUALS(bits_consume_8(&stream, 8), 0xBC);
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

    TEST_RUN(test_first_set_bit);
    TEST_RUN(test_bits_reverse);
    TEST_RUN(test_bits_count);

    TEST_RUN(test_bits_walk);
    TEST_RUN(test_bits_consume_8);
    TEST_RUN(test_bits_consume_16);
    TEST_RUN(test_bits_consume_32_64);
    TEST_RUN(test_bits_peek);

    TEST_FINALIZE();

    return 0;
}
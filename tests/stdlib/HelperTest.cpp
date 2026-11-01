#include "../TestFramework.h"
#include "../../stdlib/Helper.h"

// OMS_IS_ZERO_F32 / OMS_FEQUAL_F32 use an unqualified abs().
// MSVC only provides the floating point abs() overloads through <cmath> (std::abs),
// without it the macro calls are ambiguous between abs(int), abs(long) and abs(int64).
// The using declaration is kept function local so the combined UBER_TEST translation unit
// isn't polluted.
#include <cmath>

/**
 * Helper macro tests
 *
 * The header only contains macros and a few small inline helpers, so the tests are
 * grouped by macro family (arithmetic, alignment, flags/bits, bit arrays, bit
 * iteration, endianness, serialization helpers).
 */

struct HelperMemberTest {
    char name[16];
    int32 value;
    uint8 data[3][4];
};

static void test_helper_array_and_member_count() {
    int32 ints[7] = {0};
    const char chars[13] = {0};
    f64 matrix[3][4] = {{0}};

    TEST_EQUALS(ARRAY_COUNT(ints), 7);
    TEST_EQUALS(ARRAY_COUNT(chars), 13);
    TEST_EQUALS(ARRAY_COUNT(matrix), 3);
    TEST_EQUALS(ARRAY_COUNT(matrix[0]), 4);

    TEST_EQUALS(MEMBER_SIZEOF(HelperMemberTest, name), 16);
    TEST_EQUALS(MEMBER_SIZEOF(HelperMemberTest, value), 4);
    TEST_EQUALS(MEMBER_ARRAY_COUNT(HelperMemberTest, name), 16);
    TEST_EQUALS(MEMBER_ARRAY_COUNT(HelperMemberTest, data), 3);
    TEST_EQUALS(ARRAY_COUNT_MEMBER(HelperMemberTest, data), 3);

    // ARRAY_COUNT is a constant expression -> usable in a static assert style comparison
    TEST_EQUALS(ARRAY_COUNT(ints) * sizeof(ints[0]), sizeof(ints));
}

static void test_helper_math() {
    TEST_EQUALS(OMS_ABS_I8((int8) -5), 5);
    TEST_EQUALS(OMS_ABS_I8((int8) 5), 5);
    TEST_EQUALS(OMS_ABS_I16((int16) -1234), 1234);
    TEST_EQUALS(OMS_ABS_I32((int32) -123456), 123456);
    TEST_EQUALS(OMS_ABS_I64((int64) -1234567890123LL), 1234567890123LL);

    TEST_EQUALS(OMS_POW2_I32(10), 1024);
    TEST_EQUALS(OMS_POW2_I64(40), 1099511627776LL);
    TEST_EQUALS(OMS_DIV2_I32(64), 32);
    TEST_EQUALS(OMS_DIV2_I64(64), 32);
    TEST_EQUALS(OMS_MUL2_I32(21), 42);
    TEST_EQUALS(OMS_MUL2_I64(21), 42);

    TEST_TRUE(OMS_IS_POW2(1));
    TEST_TRUE(OMS_IS_POW2(2));
    TEST_TRUE(OMS_IS_POW2(1024));
    TEST_FALSE(OMS_IS_POW2(3));
    TEST_FALSE(OMS_IS_POW2(1000));

    TEST_EQUALS(DIV_RECIPROCAL_F32(4.0f), 0.25f);
    TEST_EQUALS(DIV_RECIPROCAL_F64(4.0), 0.25);

    TEST_EQUALS(div_pow2(2), 1);
    TEST_EQUALS(div_pow2(8), 3);
    TEST_EQUALS(div_pow2(1024), 10);
    TEST_EQUALS(div_pow2(65536), 16);
}

static void test_helper_float_compare() {
    using std::abs;

    TEST_TRUE(OMS_IS_ZERO_F32(0.0f));
    TEST_TRUE(OMS_IS_ZERO_F32(OMS_EPSILON_F32 / 2.0f));
    TEST_FALSE(OMS_IS_ZERO_F32(0.001f));

    TEST_TRUE(OMS_IS_ZERO_F64(0.0));
    TEST_TRUE(OMS_IS_ZERO_F64(OMS_EPSILON_F64 / 2.0));
    TEST_FALSE(OMS_IS_ZERO_F64(0.001));

    TEST_TRUE(OMS_FEQUAL_F32(1.0f, 1.0f));
    TEST_TRUE(OMS_FEQUAL_F32(1.0f, 1.0f + OMS_EPSILON_F32 / 2.0f));
    TEST_FALSE(OMS_FEQUAL_F32(1.0f, 1.1f));

    TEST_TRUE(OMS_FEQUAL_F64(1.0, 1.0));
    TEST_TRUE(OMS_FEQUAL_F64(-2.5, -2.5));
    TEST_FALSE(OMS_FEQUAL_F64(1.0, 1.5));

    TEST_EQUALS(OMS_SIGN_32(5), 1);
    TEST_EQUALS(OMS_SIGN_32(-5), -1);
    TEST_EQUALS(OMS_SIGN_64((int64) 5), 1);
    TEST_EQUALS(OMS_SIGN_64((int64) -5), -1);
}

static void test_helper_align() {
    TEST_EQUALS(ALIGN_UP(0, 8), 0);
    TEST_EQUALS(ALIGN_UP(1, 8), 8);
    TEST_EQUALS(ALIGN_UP(8, 8), 8);
    TEST_EQUALS(ALIGN_UP(9, 8), 16);
    TEST_EQUALS(ALIGN_UP(17, 16), 32);

    TEST_EQUALS(ALIGN_DOWN(0, 8), 0);
    TEST_EQUALS(ALIGN_DOWN(7, 8), 0);
    TEST_EQUALS(ALIGN_DOWN(8, 8), 8);
    TEST_EQUALS(ALIGN_DOWN(15, 8), 8);
    TEST_EQUALS(ALIGN_DOWN(32, 16), 32);

    TEST_TRUE(OMS_IS_ALIGNED(0, 8));
    TEST_TRUE(OMS_IS_ALIGNED(64, 8));
    TEST_FALSE(OMS_IS_ALIGNED(65, 8));

    // The alignment of a 64 bit pointer must not be broken by the 32 bit mask trap
    const uintptr_t pointer = (uintptr_t) 0x0000'7FFF'1234'5678ULL;
    const uintptr_t aligned = (uintptr_t) ALIGN_UP(pointer, (uintptr_t) 64);
    const uintptr_t down = (uintptr_t) ALIGN_DOWN(pointer, (uintptr_t) 64);
    TEST_EQUALS((long long) (aligned - down), 64);
    TEST_EQUALS((long long) (aligned & 63), 0);
    TEST_EQUALS((long long) (down & 63), 0);
    TEST_TRUE(pointer > 0xFFFFFFFFULL ? (aligned / 0x100000000ULL) != 0 : true);
}

static void test_helper_flags_and_bits() {
    uint32 flags = 0;
    flags = OMS_FLAG_SET(flags, 1U << 3);
    TEST_TRUE(OMS_FLAG_CHECK(flags, 1U << 3));
    TEST_TRUE(OMS_FLAG_IS_SET(flags, 1U << 3));
    TEST_FALSE(OMS_FLAG_CHECK(flags, 1U << 4));

    flags = OMS_FLAG_TOGGLE(flags, 1U << 4);
    TEST_TRUE(OMS_FLAG_CHECK(flags, 1U << 4));
    flags = OMS_FLAG_FLIP(flags, 1U << 4);
    TEST_FALSE(OMS_FLAG_CHECK(flags, 1U << 4));

    flags = OMS_FLAG_CLEAR(flags, 1U << 3);
    TEST_EQUALS(flags, 0);
    flags = OMS_FLAG_SET(flags, 1U << 1);
    flags = OMS_FLAG_REMOVE(flags, 1U << 1);
    TEST_EQUALS(flags, 0);
    flags = OMS_FLAG_SET(flags, 1U << 5);
    flags = OMS_FLAG_DELETE(flags, 1U << 5);
    TEST_EQUALS(flags, 0);

    // Index based bit manipulation on a single value
    uint32 value = 0;
    value = OMS_BIT_SET(value, 5);
    TEST_EQUALS(value, 32);
    TEST_TRUE(OMS_BIT_IS_SET(value, 5));
    TEST_FALSE(OMS_BIT_IS_SET(value, 4));

    TEST_EQUALS(OMS_BIT_WORD_INDEX(0), 0);
    TEST_EQUALS(OMS_BIT_WORD_INDEX(sizeof(size_t) * 8 - 1), 0);
    TEST_EQUALS(OMS_BIT_WORD_INDEX(sizeof(size_t) * 8), 1);
    TEST_EQUALS(OMS_BIT_WORD_INDEX(sizeof(size_t) * 8 * 3 + 2), 3);
    TEST_EQUALS(OMS_BIT_INDEX(0), 0);
    TEST_EQUALS(OMS_BIT_INDEX(sizeof(size_t) * 8), 0);
    TEST_EQUALS(OMS_BIT_INDEX(sizeof(size_t) * 8 + 3), 3);
}

static void test_helper_bitarray() {
    const int32 word_bits = (int32) (sizeof(size_t) * 8);
    size_t flags[2] = {0, 0};

    OMS_BITARRAY_SET(flags, 0);
    OMS_BITARRAY_SET(flags, 5);
    OMS_BITARRAY_SET(flags, (uint32) word_bits);

    TEST_EQUALS(OMS_BITARRAY_CHECK(flags, 0), 1);
    TEST_EQUALS(OMS_BITARRAY_CHECK(flags, 5), 1);
    TEST_EQUALS(OMS_BITARRAY_CHECK(flags, 6), 0);
    TEST_EQUALS(OMS_BITARRAY_CHECK(flags, (uint32) word_bits), 1);
    TEST_EQUALS(flags[0], (size_t) 33);
    TEST_EQUALS(flags[1], (size_t) 1);

    OMS_BITARRAY_CLEAR(flags, 5);
    TEST_EQUALS(OMS_BITARRAY_CHECK(flags, 5), 0);
    OMS_BITARRAY_REMOVE(flags, 0);
    TEST_EQUALS(OMS_BITARRAY_CHECK(flags, 0), 0);

    OMS_BITARRAY_TOGGLE(flags, 7);
    TEST_EQUALS(OMS_BITARRAY_CHECK(flags, 7), 1);
    OMS_BITARRAY_FLIP(flags, 7);
    TEST_EQUALS(OMS_BITARRAY_CHECK(flags, 7), 0);

    OMS_BITARRAY_SET_TO(flags, 9, true);
    TEST_EQUALS(OMS_BITARRAY_CHECK(flags, 9), 1);
    OMS_BITARRAY_SET_TO(flags, 9, false);
    TEST_EQUALS(OMS_BITARRAY_CHECK(flags, 9), 0);

    OMS_BITARRAY_DELETE(flags, (uint32) word_bits);
    TEST_EQUALS(OMS_BITARRAY_CHECK(flags, (uint32) word_bits), 0);
    TEST_EQUALS(flags[1], (size_t) 0);
}

static void test_helper_bit_l2r_r2l() {
    // Left to right (MSB first)
    TEST_TRUE((bool) IS_BIT_SET_L2R((uint32) 0x80000000, 0, 32));
    TEST_TRUE((bool) IS_BIT_SET_L2R((uint32) 0x1, 31, 32));
    TEST_FALSE((bool) IS_BIT_SET_L2R((uint32) 0x1, 0, 32));

    TEST_EQUALS(BIT_SET_L2R((uint32) 0, 0, 32), 0x80000000);
    TEST_EQUALS(BIT_SET_32_L2R((uint32) 0, 31, 32), 0x1);
    TEST_EQUALS(BIT_UNSET_L2R((uint32) 0xFFFFFFFF, 31, 32), 0xFFFFFFFE);
    TEST_EQUALS(BIT_FLIP_L2R((uint32) 0, 31, 32), 0x1);
    TEST_EQUALS(BIT_SET_TO_L2R((uint32) 0, 0, 1, 32), 0x80000000);
    TEST_EQUALS(BIT_SET_TO_L2R((uint32) 0xFFFFFFFF, 0, 0, 32), 0x7FFFFFFF);

    TEST_TRUE((bool) IS_BIT_SET_64_L2R(0x8000000000000000ULL, 0, 64));
    TEST_EQUALS(BIT_SET_64_L2R((uint64) 0, 63, 64), 1);
    TEST_EQUALS(BIT_UNSET_64_L2R(0xFFFFFFFFFFFFFFFFULL, 63, 64), 0xFFFFFFFFFFFFFFFEULL);
    TEST_EQUALS(BIT_FLIP_64_L2R((uint64) 0, 0, 64), 0x8000000000000000ULL);
    TEST_EQUALS(BIT_SET_TO_64_L2R((uint64) 0xFFFFFFFFFFFFFFFFULL, 0, 0, 64), 0x7FFFFFFFFFFFFFFFULL);
    TEST_EQUALS(BIT_SET_TO_64_L2R((uint64) 0, 0, 1, 64), 0x8000000000000000ULL);

    // Right to left (LSB first)
    TEST_TRUE((bool) IS_BIT_SET_R2L((uint32) 1, 0));
    TEST_TRUE((bool) IS_BIT_SET_R2L((uint32) 0x80000000, 31));
    TEST_FALSE((bool) IS_BIT_SET_R2L((uint32) 1, 1));
    TEST_EQUALS(BIT_SET_R2L((uint32) 0, 0), 1);
    TEST_EQUALS(BIT_SET_32_R2L((uint32) 0, 31), 0x80000000);
    TEST_EQUALS(BIT_UNSET_R2L((uint32) 0xFFFFFFFF, 0), 0xFFFFFFFE);
    TEST_EQUALS(BIT_FLIP_R2L((uint32) 0, 0), 1);
    TEST_EQUALS(BIT_SET_TO_R2L((uint32) 0, 4, 1), 16);
    TEST_EQUALS(BIT_SET_TO_R2L((uint32) 16, 4, 0), 0);

    TEST_TRUE((bool) IS_BIT_SET_64_R2L((uint64) 1, 0));
    TEST_EQUALS(BIT_SET_64_R2L((uint64) 0, 63), 0x8000000000000000ULL);
    TEST_EQUALS(BIT_UNSET_64_R2L(0xFFFFFFFFFFFFFFFFULL, 0), 0xFFFFFFFFFFFFFFFEULL);
    TEST_EQUALS(BIT_FLIP_64_R2L((uint64) 1, 0), 0);
    TEST_EQUALS(BIT_SET_TO_64_R2L((uint64) 0, 40, 1), 0x10000000000ULL);
    TEST_EQUALS(BIT_SET_TO_64_R2L((uint64) 0x10000000000ULL, 40, 0), 0);

    // Bit field extraction
    TEST_EQUALS(BITS_GET_8_L2R((uint8) 0xB4, 1, 3), 3);
    TEST_EQUALS(BITS_GET_8_L2R((uint8) 0xB4, 0, 4), 11);
    TEST_EQUALS(BITS_GET_16_L2R((uint16) 0xABCD, 0, 8), 0xAB);
    TEST_EQUALS(BITS_GET_16_L2R((uint16) 0xABCD, 8, 8), 0xCD);
    TEST_EQUALS(BITS_GET_32_L2R((uint32) 0x12345678, 0, 8), 0x12);
    TEST_EQUALS(BITS_GET_64_L2R(0x0102030405060708ULL, 0, 8), 0x01);
    TEST_EQUALS(BITS_GET_64_L2R(0x0102030405060708ULL, 56, 8), 0x08);

    TEST_EQUALS(BITS_GET_8_R2L((uint8) 0xB4, 2, 3), 5);
    TEST_EQUALS(BITS_GET_16_R2L((uint16) 0xABCD, 0, 8), 0xCD);
    TEST_EQUALS(BITS_GET_16_R2L((uint16) 0xABCD, 8, 8), 0xAB);
    TEST_EQUALS(BITS_GET_32_R2L((uint32) 0x12345678, 8, 8), 0x56);
    TEST_EQUALS(BITS_GET_64_R2L(0x0102030405060708ULL, 0, 8), 0x08);
    TEST_EQUALS(BITS_GET_64_R2L(0x0102030405060708ULL, 56, 8), 0x01);
}

static void test_helper_bytes_merge() {
    const byte l2r_2[2] = {0x12, 0x34};
    const byte l2r_4[4] = {0x11, 0x22, 0x33, 0x44};
    const byte l2r_8[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

    const byte r2l_2[2] = {0x34, 0x12};
    const byte r2l_4[4] = {0x44, 0x33, 0x22, 0x11};
    const byte r2l_8[8] = {0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11};

    #if OMS_LITTLE_ENDIAN
        // On little endian the L2R merge has to build the big endian value
        TEST_EQUALS(BYTES_MERGE_2_L2R(l2r_2), 0x1234);
        TEST_EQUALS(BYTES_MERGE_4_L2R(l2r_4), 0x11223344);
        TEST_EQUALS((uint64) BYTES_MERGE_8_L2R(l2r_8), 0x1122334455667788ULL);

        // The R2L merge basically reinterprets memory as a little endian value
        TEST_EQUALS(BYTES_MERGE_2_R2L(r2l_2), 0x1234);
        TEST_EQUALS(BYTES_MERGE_4_R2L(r2l_4), 0x11223344);
        TEST_EQUALS((uint64) BYTES_MERGE_8_R2L(r2l_8), 0x1122334455667788ULL);
    #else
        TEST_EQUALS(BYTES_MERGE_2_L2R(l2r_2), 0x1234);
        TEST_EQUALS(BYTES_MERGE_4_L2R(l2r_4), 0x11223344);
        TEST_EQUALS((uint64) BYTES_MERGE_8_L2R(l2r_8), 0x1122334455667788ULL);

        TEST_EQUALS(BYTES_MERGE_2_R2L(r2l_2), 0x3412);
        TEST_EQUALS(BYTES_MERGE_4_R2L(r2l_4), 0x44332211);
        TEST_EQUALS((uint64) BYTES_MERGE_8_R2L(r2l_8), 0x8877665544332211ULL);
    #endif

    // Writes 2 bytes, the sign of the byte values must not matter
    const byte signed_bytes[2] = {0xFF, 0x01};
    TEST_EQUALS(BYTES_MERGE_2_L2R(signed_bytes), 0xFF01);
}

static void test_helper_endian() {
    #if OMS_LITTLE_ENDIAN
        TEST_TRUE(is_little_endian());
    #else
        TEST_FALSE(is_little_endian());
    #endif

    TEST_EQUALS(endian_swap((uint16) 0x1234), 0x3412);
    TEST_EQUALS(endian_swap((int16) 0x1234), 0x3412);
    TEST_EQUALS(endian_swap((uint32) 0x12345678), 0x78563412);
    TEST_EQUALS(endian_swap((int32) 0x12345678), 0x78563412);
    TEST_EQUALS((long long) endian_swap((uint64) 0x1122334455667788ULL), (long long) 0x8877665544332211ULL);
    TEST_EQUALS((long long) endian_swap((int64) 0x1122334455667788LL), (long long) 0x8877665544332211ULL);

    // Swapping twice is the identity
    TEST_EQUALS(endian_swap(endian_swap((uint32) 0xDEADBEEF)), 0xDEADBEEF);
    TEST_EQUALS(endian_swap(endian_swap((uint16) 0xABCD)), 0xABCD);

    // Float swapping keeps the bit pattern
    const f32 swapped_float = endian_swap(1.0f);
    TEST_EQUALS((uint32) BITCAST(swapped_float, uint32), 0x0000803F);
    TEST_EQUALS_WITH_DELTA(endian_swap(swapped_float), 1.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(endian_swap(endian_swap(2.5f)), 2.5f, 0.000001);
    TEST_EQUALS_WITH_DELTA(endian_swap(endian_swap(2.5)), 2.5, 0.000001);

    // The automatic swap macros depend on the platform endianness
    TEST_EQUALS(SWAP_ENDIAN_LITTLE_32(0x12345678), 0x12345678);
    TEST_EQUALS(SWAP_ENDIAN_BIG_32(0x12345678), 0x78563412);

    uint32 self = 0x11223344;
    SWAP_ENDIAN_LITTLE_SELF(self);
    TEST_EQUALS(self, 0x11223344);
    SWAP_ENDIAN_BIG_SELF(self);
    TEST_EQUALS(self, 0x44332211);

    #if OMS_LITTLE_ENDIAN
        TEST_EQUALS(SWAP_ENDIAN_LITTLE((uint32) 0xAABBCCDD), 0xAABBCCDD);
        TEST_EQUALS(SWAP_ENDIAN_BIG((uint32) 0xAABBCCDD), 0xDDCCBBAA);
    #else
        TEST_EQUALS(SWAP_ENDIAN_LITTLE((uint32) 0xAABBCCDD), 0xDDCCBBAA);
        TEST_EQUALS(SWAP_ENDIAN_BIG((uint32) 0xAABBCCDD), 0xAABBCCDD);
    #endif
}

static void test_helper_read_write_le() {
    byte buffer[64] = {0};
    byte* write = buffer;

    write = write_le(write, (uint16) 0x1234);
    write = write_le(write, (int16) -2);
    write = write_le(write, (int32) 0x11223344);
    write = write_le(write, (int64) 0x0102030405060708LL);
    write = write_le(write, 1.0f);
    write = write_le(write, -2.5);

    TEST_EQUALS((long long) (write - buffer), 2 + 2 + 4 + 8 + 4 + 8);

    // Little endian byte order on disk
    TEST_EQUALS(buffer[0], 0x34);
    TEST_EQUALS(buffer[1], 0x12);
    TEST_EQUALS(buffer[2], 0xFE);
    TEST_EQUALS(buffer[3], 0xFF);

    const byte* read = buffer;
    uint16 v16 = 0;
    int16 i16 = 0;
    int32 i32 = 0;
    int64 i64 = 0;
    f32 f32_value = 0;
    f64 f64_value = 0;

    read = read_le(read, &v16);
    read = read_le(read, &i16);
    read = read_le(read, &i32);
    read = read_le(read, &i64);
    read = read_le(read, &f32_value);
    read = read_le(read, &f64_value);

    TEST_EQUALS((long long) (read - buffer), (long long) (write - buffer));
    TEST_EQUALS(v16, 0x1234);
    TEST_EQUALS(i16, -2);
    TEST_EQUALS(i32, 0x11223344);
    TEST_EQUALS((long long) i64, (long long) 0x0102030405060708LL);
    TEST_EQUALS_WITH_DELTA(f32_value, 1.0f, 0.000001);
    TEST_EQUALS_WITH_DELTA(f64_value, -2.5, 0.0000001);

    // Round trip of negative and float values
    byte roundtrip[16] = {0};
    write = write_le(roundtrip, -12345.5);
    read = read_le(roundtrip, &f64_value);
    TEST_EQUALS((long long) (write - roundtrip), 8);
    TEST_EQUALS((long long) (read - roundtrip), 8);
    TEST_EQUALS_WITH_DELTA(f64_value, -12345.5, 0.0000001);

    // Unaligned buffers have to work as well (the helpers memcpy internally)
    byte unaligned[16] = {0};
    write = write_le(unaligned + 1, (uint32) 0xCAFEBABE);
    uint32 v32 = 0;
    read_le(unaligned + 1, &v32);
    TEST_EQUALS(v32, 0xCAFEBABE);
    TEST_EQUALS((long long) (write - unaligned), 5);
}

static void test_helper_bit_iterate() {
    const int32 word_bits = (int32) (sizeof(size_t) * 8);
    size_t bits[2] = {0, 0};

    OMS_BITARRAY_SET(bits, 0);
    OMS_BITARRAY_SET(bits, 5);
    OMS_BITARRAY_SET(bits, (uint32) word_bits - 1);
    OMS_BITARRAY_SET(bits, (uint32) word_bits);
    OMS_BITARRAY_SET(bits, 100);

    uint32 found[8] = {0};
    int32 found_count = 0;
    uint32 bit_id = 0;

    bit_iterate_start(bits, 101, bit_id) {
        if (found_count < ARRAY_COUNT(found)) {
            found[found_count] = bit_id;
        }
        ++found_count;
    } bit_iterate_end

    TEST_EQUALS(found_count, 5);
    TEST_EQUALS(found[0], 0);
    TEST_EQUALS(found[1], 5);
    TEST_EQUALS(found[2], word_bits - 1);
    TEST_EQUALS(found[3], word_bits);
    TEST_EQUALS(found[4], 100);

    // An empty bit field has to iterate zero times
    size_t empty[2] = {0, 0};
    int32 empty_count = 0;
    bit_id = 0;
    bit_iterate_start(empty, 128, bit_id) {
        ++empty_count;
    } bit_iterate_end
    TEST_EQUALS(empty_count, 0);

    // bit_iterate_continue_n skips n bits (produces 0 and 2 here)
    // WARNING: bit_iterate_continue_n hard codes the iterator variable name `_bit_id`
    size_t skip_bits[1] = {0};
    OMS_BITARRAY_SET(skip_bits, 0);
    OMS_BITARRAY_SET(skip_bits, 1);
    OMS_BITARRAY_SET(skip_bits, 2);

    int32 skip_count = 0;
    uint32 last_id = 0xFFFF;
    uint32 _bit_id = 0;
    bit_iterate_start(skip_bits, 4, _bit_id) {
        ++skip_count;
        last_id = _bit_id;

        if (skip_count == 1) {
            bit_iterate_continue_n(2);
        }
    } bit_iterate_end

    TEST_EQUALS(skip_count, 2);
    TEST_EQUALS(last_id, 2);

    // bit_iterate_continue just skips the current element
    int32 continue_count = 0;
    bit_id = 0;
    bit_iterate_start(skip_bits, 4, bit_id) {
        ++continue_count;
        bit_iterate_continue;
    } bit_iterate_end
    TEST_EQUALS(continue_count, 3);
}

static void test_helper_wrapped_and_swap() {
    int32 value = 0;
    OMS_WRAPPED_INCREMENT(value, 10);
    TEST_EQUALS(value, 1);

    value = 9;
    OMS_WRAPPED_INCREMENT(value, 10);
    TEST_EQUALS(value, 0);

    value = 0;
    OMS_WRAPPED_DECREMENT(value, 10);
    TEST_EQUALS(value, 9);

    value = 5;
    OMS_WRAPPED_INC_SE(value, 4, 8);
    TEST_EQUALS(value, 6);

    value = 7;
    OMS_WRAPPED_INC_SE(value, 4, 8);
    TEST_EQUALS(value, 4);

    value = 4;
    OMS_WRAPPED_DEC_SE(value, 4, 8);
    TEST_EQUALS(value, 7);

    value = 6;
    OMS_WRAPPED_DEC_SE(value, 4, 8);
    TEST_EQUALS(value, 5);

    int32 a = 1;
    int32 b = 2;
    OMS_SWAP(int32, a, b);
    TEST_EQUALS(a, 2);
    TEST_EQUALS(b, 1);

    TEST_EQUALS(MODULO_2(37, 16), 5);
    TEST_EQUALS(MODULO_2(64, 64), 0);
    TEST_EQUALS(MODULO_2(63, 64), 63);

    TEST_EQUALS(OMS_HAS_ALPHA(0xFF0000FF), 0xFF);
    TEST_EQUALS(OMS_HAS_ALPHA(0x12345678), 0x78);
    TEST_FALSE(OMS_HAS_ALPHA(0x12345600));

    // Zeroing helpers
    int64 zero_target = -1;
    MEMSET_ZERO_64(&zero_target);
    TEST_EQUALS((long long) zero_target, 0);
    int32 zero32 = -1;
    MEMSET_ZERO_32(&zero32);
    TEST_EQUALS(zero32, 0);
    HelperMemberTest member_zero = {{0}, -1, {{0}}};
    MEMORY_ELEMENT_ZERO(&member_zero);
    TEST_EQUALS(member_zero.value, 0);
    TEST_EQUALS(member_zero.name[0], 0);

    // MEMORY_OFFSET works on addresses (no pointer subtraction UB)
    HelperMemberTest memory_element;
    const size_t offset = MEMORY_OFFSET(&memory_element.value, &memory_element);
    TEST_EQUALS((long long) offset, (long long) MEMBER_SIZEOF(HelperMemberTest, name));
}

static void test_helper_misc() {
    // macro_fnv1a_32 is used to hash strings known at compile time
    TEST_EQUALS(macro_fnv1a_32("abc"), macro_fnv1a_32("abc"));
    TEST_NOT_EQUALS(macro_fnv1a_32("abc"), macro_fnv1a_32("abd"));
    TEST_NOT_EQUALS(macro_fnv1a_32(""), macro_fnv1a_32("a"));
    TEST_EQUALS((uint32) macro_fnv1a_32(""), 2166136261u);

    // Bit level casts
    TEST_EQUALS((uint32) BITCAST(1.0f, uint32), 0x3F800000);
    TEST_EQUALS((uint32) BITCAST(-0.0f, uint32), 0x80000000);
    TEST_EQUALS((uint32) BITCAST((uint32) 0x40000000, uint32), 0x40000000);
    TEST_EQUALS_WITH_DELTA(BITCAST((uint32) 0x3F800000, f32), 1.0f, 0.000001);
    TEST_EQUALS((long long) BITCAST(1.0, uint64), (long long) 0x3FF0000000000000ULL);

    // Type traits (the template arguments contain commas -> extra parentheses for the macro)
    TEST_EQUALS((is_same<int32, int32>::value), 1);
    TEST_EQUALS((is_same<int32, uint32>::value), 0);
    TEST_TRUE((is_convertible_v<int32, f64>));
    TEST_FALSE((is_convertible_v<const char*, int32>));

    // The character type helper used by the string utils
    // NOTE: CharTypeOf_v<T> is not usable, it cannot be initialized from the uint8 value (see report)
    TEST_EQUALS(CharTypeOf<char>::value, CHAR_TYPE_CHAR);
    TEST_EQUALS(CharTypeOf<wchar_t>::value, CHAR_TYPE_WCHAR);
    TEST_EQUALS(CharTypeOf<char32_t>::value, CHAR_TYPE_UTF32);

    // Type sizes the whole engine relies on
    TEST_EQUALS(sizeof(int8), 1);
    TEST_EQUALS(sizeof(int16), 2);
    TEST_EQUALS(sizeof(int32), 4);
    TEST_EQUALS(sizeof(int64), 8);
    TEST_EQUALS(sizeof(byte), 1);
    TEST_EQUALS(sizeof(f32), 4);
    TEST_EQUALS(sizeof(f64), 8);
    TEST_EQUALS(sizeof(umm), sizeof(void*));
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main StdlibHelperTest
#endif

int main() {
    TEST_INIT(96);

    TEST_RUN(test_helper_array_and_member_count);
    TEST_RUN(test_helper_math);
    TEST_RUN(test_helper_float_compare);
    TEST_RUN(test_helper_align);
    TEST_RUN(test_helper_flags_and_bits);
    TEST_RUN(test_helper_bitarray);
    TEST_RUN(test_helper_bit_l2r_r2l);
    TEST_RUN(test_helper_bytes_merge);
    TEST_RUN(test_helper_endian);
    TEST_RUN(test_helper_read_write_le);
    TEST_RUN(test_helper_bit_iterate);
    TEST_RUN(test_helper_wrapped_and_swap);
    TEST_RUN(test_helper_misc);

    TEST_FINALIZE();

    return 0;
}

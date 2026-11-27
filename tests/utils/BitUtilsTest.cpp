#include "../TestFramework.h"
#include "../../utils/StringUtils.h"
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

// Bit stream (MSB first)
static void test_bits_walk() {
    byte data[4] = {};
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

// Bounded bit streams + serialize (BitWriter/BitReader)
static void test_bit_stream_roundtrip() {
    byte buffer[64];
    BitWriter writer;
    bit_writer_init(&writer, buffer, sizeof(buffer));

    // Every width from 1 to 32 bits, values with all bits set and an odd pattern
    for (uint32 bits = 1; bits <= 32; ++bits) {
        bit_write(&writer, bits_mask(bits), bits);
    }

    // 1 + 2 + ... + 32 = 528 bits = 66 bytes don't fit into 64 -> the writer must report it
    TEST_TRUE(writer.overflow);
    TEST_EQUALS(bit_writer_flush(&writer), 0);

    BitWriter big_writer;
    byte big[128];
    bit_writer_init(&big_writer, big, sizeof(big));

    for (uint32 bits = 1; bits <= 32; ++bits) {
        bit_write(&big_writer, bits_mask(bits) ^ 0x5, bits);
    }

    TEST_EQUALS(bit_writer_flush(&big_writer), 66);

    BitReader reader;
    bit_reader_init(&reader, big, 66);

    for (uint32 bits = 1; bits <= 32; ++bits) {
        TEST_EQUALS(bit_read(&reader, bits), (bits_mask(bits) ^ 0x5) & bits_mask(bits));
    }

    TEST_FALSE(reader.overflow);
}

static void test_bit_stream_overflow() {
    byte buffer[2];
    BitWriter writer;
    bit_writer_init(&writer, buffer, sizeof(buffer));

    bit_write(&writer, 0xFFF, 12);
    TEST_FALSE(writer.overflow);

    // 12 + 5 > 16 bits
    bit_write(&writer, 1, 5);
    TEST_TRUE(writer.overflow);
    TEST_EQUALS(bit_writer_flush(&writer), 0);

    // Reading past the end returns 0 and sets overflow, never reads out of bounds
    const byte data[2] = {0xFF, 0xF0};
    BitReader reader;
    bit_reader_init(&reader, data, sizeof(data));

    TEST_EQUALS(bit_read(&reader, 12), 0xFFF);
    TEST_EQUALS(bit_read(&reader, 8), 0);
    TEST_TRUE(reader.overflow);

    // Further reads stay failed
    TEST_EQUALS(bit_read(&reader, 1), 0);

    // Bytes past the end
    byte out[4];
    bit_reader_init(&reader, data, sizeof(data));
    bit_read_bytes(&reader, out, 3);
    TEST_TRUE(reader.overflow);
}

static void test_bit_stream_bytes_alignment() {
    byte buffer[16];
    BitWriter writer;
    bit_writer_init(&writer, buffer, sizeof(buffer));

    const byte payload[3] = {0xAA, 0xBB, 0xCC};
    bit_write(&writer, 5, 3);
    bit_write_bytes(&writer, payload, sizeof(payload));
    bit_write(&writer, 1, 1);

    // 3 bits + padding to 8 + 24 bits + 1 bit -> 5 bytes
    TEST_EQUALS(bit_writer_flush(&writer), 5);

    BitReader reader;
    bit_reader_init(&reader, buffer, 5);

    byte out[3];
    TEST_EQUALS(bit_read(&reader, 3), 5);
    bit_read_bytes(&reader, out, sizeof(out));
    TEST_MEMORY_EQUALS(out, payload, sizeof(payload));
    TEST_EQUALS(bit_read(&reader, 1), 1);
    TEST_FALSE(reader.overflow);

    // Non zero padding is invalid
    buffer[0] |= 0x01;
    bit_reader_init(&reader, buffer, 5);
    bit_read(&reader, 3);
    bit_read_bytes(&reader, out, sizeof(out));
    TEST_TRUE(reader.overflow);
}

static void test_bit_stream_serialize_ranges() {
    byte buffer[32];
    BitWriter writer;
    bit_writer_init(&writer, buffer, sizeof(buffer));

    uint8 level = 99;
    int32 offset = -7;
    bool flag = true;
    uint64 big = 0x0123456789ABCDEFULL;

    TEST_TRUE(serialize_uint(&writer, &level, 7));
    TEST_TRUE(serialize_int_range(&writer, &offset, -10, 10));
    TEST_TRUE(serialize_bool(&writer, &flag));
    TEST_TRUE(serialize_u64(&writer, &big));

    // 7 + 5 + 1 + 64 = 77 bits
    TEST_EQUALS(bit_writer_bits(&writer), 77);

    const uint32 size = bit_writer_flush(&writer);

    BitReader reader;
    bit_reader_init(&reader, buffer, size);

    uint8 level_read = 0;
    int32 offset_read = 0;
    bool flag_read = false;
    uint64 big_read = 0;

    TEST_TRUE(serialize_uint(&reader, &level_read, 7));
    TEST_TRUE(serialize_int_range(&reader, &offset_read, -10, 10));
    TEST_TRUE(serialize_bool(&reader, &flag_read));
    TEST_TRUE(serialize_u64(&reader, &big_read));

    TEST_EQUALS(level_read, 99);
    TEST_EQUALS(offset_read, -7);
    TEST_TRUE(flag_read);
    TEST_TRUE(big_read == 0x0123456789ABCDEFULL);

    // Values that don't fit are rejected when writing
    bit_writer_init(&writer, buffer, sizeof(buffer));
    uint8 too_large = 200;
    TEST_FALSE(serialize_uint(&writer, &too_large, 7));

    bit_writer_init(&writer, buffer, sizeof(buffer));
    int32 outside = 11;
    TEST_FALSE(serialize_int_range(&writer, &outside, -10, 10));

    // A corrupted value outside of the range is rejected when reading (5 bits allow 31 > range 20)
    const byte corrupted[1] = {0xF8};
    bit_reader_init(&reader, corrupted, sizeof(corrupted));
    TEST_FALSE(serialize_int_range(&reader, &offset_read, -10, 10));
}

static void test_bit_stream_quantization() {
    // Every quantized value is within half a step
    const f32 precision = quantize_precision(0.0f, 32.0f, 12);
    TEST_LESSER_THAN(precision, 0.004f);

    for (int32 i = 0; i <= 1000; ++i) {
        const f32 value = (f32) i * 0.032f;
        const f32 restored = dequantize_float(quantize_float(value, 0.0f, 32.0f, 12), 0.0f, 32.0f, 12);

        TEST_LESSER_THAN(fabsf(restored - value), precision + 0.00001f);
    }

    // Bounds are exact, outside values are clamped
    TEST_EQUALS_WITH_DELTA(dequantize_float(quantize_float(-5.0f, -5.0f, 5.0f, 10), -5.0f, 5.0f, 10), -5.0, 0.00001);
    TEST_EQUALS_WITH_DELTA(dequantize_float(quantize_float(5.0f, -5.0f, 5.0f, 10), -5.0f, 5.0f, 10), 5.0, 0.00001);
    TEST_EQUALS(quantize_float(100.0f, -5.0f, 5.0f, 10), 1023);
    TEST_EQUALS(quantize_float(-100.0f, -5.0f, 5.0f, 10), 0);

    // Through the stream
    byte buffer[16];
    BitWriter writer;
    bit_writer_init(&writer, buffer, sizeof(buffer));

    v3_f32 position = {};
    position.x = 1.25f;
    position.y = 31.9f;
    position.z = 17.0f;

    TEST_TRUE(serialize_v3_range(&writer, &position, 0.0f, 32.0f, 12));
    TEST_EQUALS(bit_writer_bits(&writer), 36);

    BitReader reader;
    bit_reader_init(&reader, buffer, bit_writer_flush(&writer));

    v3_f32 restored = {};
    TEST_TRUE(serialize_v3_range(&reader, &restored, 0.0f, 32.0f, 12));

    TEST_EQUALS_WITH_DELTA(restored.x, 1.25, precision);
    TEST_EQUALS_WITH_DELTA(restored.y, 31.9, precision);
    TEST_EQUALS_WITH_DELTA(restored.z, 17.0, precision);
}

static void test_bit_stream_quaternion() {
    // Several rotations incl. a negative largest component (q == -q)
    const f32 rotations[][4] = {
        {0.0f, 0.0f, 0.0f, 1.0f},
        {0.7071068f, 0.0f, 0.0f, 0.7071068f},
        {0.1f, -0.9f, 0.3f, 0.2f},
        {0.5f, 0.5f, -0.5f, -0.5f},
        {0.0f, 0.0f, -1.0f, 0.0f},
    };

    for (int32 r = 0; r < (int32) ARRAY_COUNT(rotations); ++r) {
        quaternion q = {};
        q.x = rotations[r][0];
        q.y = rotations[r][1];
        q.z = rotations[r][2];
        q.w = rotations[r][3];

        const f32 length = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
        q.x /= length;
        q.y /= length;
        q.z /= length;
        q.w /= length;

        byte buffer[8];
        BitWriter writer;
        bit_writer_init(&writer, buffer, sizeof(buffer));

        TEST_TRUE(serialize_quaternion(&writer, &q, 9));
        TEST_EQUALS(bit_writer_bits(&writer), 29);

        BitReader reader;
        bit_reader_init(&reader, buffer, bit_writer_flush(&writer));

        quaternion restored = {};
        TEST_TRUE(serialize_quaternion(&reader, &restored, 9));

        // Same rotation: |dot| ~ 1 (q and -q are equal)
        const f32 dot = q.x * restored.x + q.y * restored.y + q.z * restored.z + q.w * restored.w;
        TEST_GREATER_THAN(fabsf(dot), 0.999f);

        const f32 restored_length = restored.x * restored.x + restored.y * restored.y + restored.z * restored.z + restored.w * restored.w;
        TEST_EQUALS_WITH_DELTA(restored_length, 1.0, 0.01);
    }
}

static void test_bit_stream_string() {
    byte buffer[64];
    BitWriter writer;
    bit_writer_init(&writer, buffer, sizeof(buffer));

    char name[32] = "Aldric";
    TEST_TRUE(serialize_string(&writer, name, sizeof(name)));

    // 5 bits length + padding + 6 bytes instead of 32 bytes
    const uint32 size = bit_writer_flush(&writer);
    TEST_EQUALS(size, 7);

    BitReader reader;
    bit_reader_init(&reader, buffer, size);

    char restored[32];
    memset(restored, 'X', sizeof(restored));
    TEST_TRUE(serialize_string(&reader, restored, sizeof(restored)));
    TEST_EQUALS(str_compare(restored, "Aldric"), 0);
    TEST_EQUALS(restored[31], 0);

    // A length longer than the data is rejected
    bit_reader_init(&reader, buffer, 3);
    TEST_FALSE(serialize_string(&reader, restored, sizeof(restored)));

    // Not terminated
    char full[4] = {'a', 'b', 'c', 'd'};
    bit_writer_init(&writer, buffer, sizeof(buffer));
    TEST_FALSE(serialize_string(&writer, full, sizeof(full)));
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main UtilsBitUtilsTest
#endif

int main() {
    TEST_INIT(2048);

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

    TEST_RUN(test_bit_stream_roundtrip);
    TEST_RUN(test_bit_stream_overflow);
    TEST_RUN(test_bit_stream_bytes_alignment);
    TEST_RUN(test_bit_stream_serialize_ranges);
    TEST_RUN(test_bit_stream_quantization);
    TEST_RUN(test_bit_stream_quaternion);
    TEST_RUN(test_bit_stream_string);

    TEST_RUN(test_bits_walk);
    TEST_RUN(test_bits_consume_8);
    TEST_RUN(test_bits_consume_16);
    TEST_RUN(test_bits_consume_32_64);
    TEST_RUN(test_bits_peek);

    TEST_FINALIZE();

    return 0;
}
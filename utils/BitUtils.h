/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_UTILS_BIT_H
#define COMS_UTILS_BIT_H

#include <math.h>
#include "../stdlib/Stdlib.h"

struct BitWalk {
    byte* pos;
    uint32 bit_pos;
};

inline
void bits_walk(BitWalk* stream, uint32 bits_to_walk) NO_EXCEPT
{
    stream->bit_pos += bits_to_walk;
    stream->pos += stream->bit_pos >> div_pow2(8);
    stream->bit_pos = MODULO_2(stream->bit_pos, 8);
}

inline
void bits_flush(BitWalk* stream) NO_EXCEPT
{
    if (stream->bit_pos > 0) {
        stream->bit_pos = 0;
        ++stream->pos;
    }
}

inline
uint8 bits_consume_8(BitWalk* stream, uint32 bits_to_consume)
{
    uint8 result;

    uint32 remaining = 8 - stream->bit_pos;
    uint32 range_1 = bits_to_consume >= remaining
        ? remaining
        : bits_to_consume;

    result = (*stream->pos >> (remaining - range_1)) & ((1 << range_1) - 1);
    stream->bit_pos += range_1;

    if (bits_to_consume < remaining) {
        return result;
    }

    ++stream->pos;
    stream->bit_pos = 0;
    bits_to_consume -= range_1;

    /*
    uint32 full_bytes = bits_to_consume >> div_pow2(8);
    if (full_bytes > 0) {
        for (int i = 0; i < full_bytes; ++i) {
            result = (result << 8) | *stream->pos;

            ++stream->pos;
        }
    }
    */

    if (bits_to_consume == 0) {
        return result;
    }

    stream->bit_pos += bits_to_consume;

    return (result << bits_to_consume) | ((*stream->pos >> (8 - bits_to_consume)) & ((1 << bits_to_consume) - 1));
}

inline
uint16 bits_consume_16(BitWalk* stream, uint32 bits_to_consume)
{
    uint16 result;

    uint32 remaining = 8 - stream->bit_pos;
    uint32 range_1 = bits_to_consume >= remaining
        ? remaining
        : bits_to_consume;

    result = (*stream->pos >> (remaining - range_1)) & ((1 << range_1) - 1);
    stream->bit_pos += range_1;

    if (bits_to_consume < remaining) {
        return result;
    }

    ++stream->pos;
    stream->bit_pos = 0;
    bits_to_consume -= range_1;

    uint32 full_bytes = bits_to_consume >> div_pow2(8);
    if (full_bytes > 0) {
        for (uint32 i = 0; i < full_bytes; ++i) {
            result = (result << 8) | *stream->pos;

            ++stream->pos;
        }
    }

    uint32 range_2 = bits_to_consume - full_bytes * 8;
    if (range_2 == 0) {
        return result;
    }

    stream->bit_pos += range_2;

    return (result << range_2) | ((*stream->pos >> (8 - range_2)) & ((1 << range_2) - 1));
}

inline
uint32 bits_consume_32(BitWalk* stream, uint32 bits_to_consume)
{
    uint32 result;

    uint32 remaining = 8 - stream->bit_pos;
    uint32 range_1 = bits_to_consume >= remaining
        ? remaining
        : bits_to_consume;

    result = (*stream->pos >> (remaining - range_1)) & ((1 << range_1) - 1);
    stream->bit_pos += range_1;

    if (bits_to_consume < remaining) {
        return result;
    }

    ++stream->pos;
    stream->bit_pos = 0;
    bits_to_consume -= range_1;

    uint32 full_bytes = bits_to_consume >> div_pow2(8);
    if (full_bytes > 0) {
        for (uint32 i = 0; i < full_bytes; ++i) {
            result = (result << 8) | *stream->pos;

            ++stream->pos;
        }
    }

    uint32 range_2 = bits_to_consume - full_bytes * 8;
    if (range_2 == 0) {
        return result;
    }

    stream->bit_pos += range_2;

    return (result << range_2) | ((*stream->pos >> (8 - range_2)) & ((1 << range_2) - 1));
}

inline
uint64 bits_consume_64(BitWalk* stream, uint32 bits_to_consume)
{
    uint64 result;

    uint32 remaining = 8 - stream->bit_pos;
    uint32 range_1 = bits_to_consume >= remaining
        ? remaining
        : bits_to_consume;

    result = (*stream->pos >> (remaining - range_1)) & ((1 << range_1) - 1);
    stream->bit_pos += range_1;

    if (bits_to_consume < remaining) {
        return result;
    }

    ++stream->pos;
    stream->bit_pos = 0;
    bits_to_consume -= range_1;

    uint32 full_bytes = bits_to_consume >> div_pow2(8);
    if (full_bytes > 0) {
        for (uint32 i = 0; i < full_bytes; ++i) {
            result = (result << 8) | *stream->pos;

            ++stream->pos;
        }
    }

    uint32 range_2 = bits_to_consume - full_bytes * 8;
    if (range_2 == 0) {
        return result;
    }

    stream->bit_pos += range_2;

    return (result << range_2) | ((*stream->pos >> (8 - range_2)) & ((1 << range_2) - 1));
}

uint8 bits_peek_8(BitWalk* stream, uint32 bits_to_consume) {
    byte* pos = stream->pos;
    uint32 bit_pos = stream->bit_pos;

    uint8 bits = bits_consume_8(stream, bits_to_consume);

    stream->pos = pos;
    stream->bit_pos = bit_pos;

    return bits;
}

uint16 bits_peek_16(BitWalk* stream, uint32 bits_to_consume) {
    byte* pos = stream->pos;
    uint32 bit_pos = stream->bit_pos;

    uint16 bits = bits_consume_16(stream, bits_to_consume);

    stream->pos = pos;
    stream->bit_pos = bit_pos;

    return bits;
}

uint32 bits_peek_32(BitWalk* stream, uint32 bits_to_consume) {
    byte* pos = stream->pos;
    uint32 bit_pos = stream->bit_pos;

    uint32 bits = bits_consume_32(stream, bits_to_consume);

    stream->pos = pos;
    stream->bit_pos = bit_pos;

    return bits;
}

uint64 bits_peek_64(BitWalk* stream, uint32 bits_to_consume) {
    byte* pos = stream->pos;
    uint32 bit_pos = stream->bit_pos;

    uint64 bits = bits_consume_64(stream, bits_to_consume);

    stream->pos = pos;
    stream->bit_pos = bit_pos;

    return bits;
}

CONSTEXPR inline
int32 first_set_bit_r2l(int32 value) NO_EXCEPT
{
    if (value == 0) {
        return 0;
    }

    int32 index = 1;
    while (value) {
        if (value & 1) {
            return index;
        }

        value >>= 1;
        ++index;
    }

    return 0;
}

CONSTEXPR inline
int32 first_set_bit_r2l(int64 value) NO_EXCEPT
{
    if (value == 0) {
        return 0;
    }

    int32 index = 1;
    while (value) {
        if (value & 1ULL) return index;
        value >>= 1;
        ++index;
    }

    return 0;
}

CONSTEXPR inline
int32 first_set_bit_l2r(int32 value) NO_EXCEPT
{
    if (value == 0) {
        return 0;
    }

    // This still maintains the correct bits
    uint32 u = (uint32) value;

    int32 index = 1;
    for (uint32 mask = 0x80000000u; mask != 0; mask >>= 1) {
        if (u & mask) {
            return index;
        }

        ++index;
    }

    return 0;
}

CONSTEXPR inline
int32 first_set_bit_l2r(int64 value) NO_EXCEPT
{
    if (value == 0) {
        return 0;
    }

    uint64 u = (uint64)value;
    int32 index = 1;

    uint64 mask = 0x8000000000000000ULL;

    while (mask != 0) {
        if (u & mask) {
            return index;
        }

        ++index;
        mask >>= 1;
    }

    return 0;
}

CONSTEXPR inline
uint32 bits_reverse(uint32 data, int32 count) NO_EXCEPT
{
    uint32 reversed = 0;
    for (int32 i = 0; i <= (count / 2); ++i) {
        uint32 inv = count - i - 1;
        reversed |= ((data >> i) & 0x1) << inv;
        reversed |= ((data >> inv) & 0x1) << i;
    }

    return reversed;
}

CONSTEXPR inline
uint64 bits_reverse(uint64 data, int32 count) NO_EXCEPT
{
    uint64 reversed = 0;
    for (int32 i = 0; i <= (count / 2); ++i) {
        int32 inv = count - i - 1;
        reversed |= ((data >> i) & 0x1ULL) << inv;
        reversed |= ((data >> inv) & 0x1ULL) << i;
    }

    return reversed;
}

static const byte BIT_COUNT_LOOKUP_TABLE[256] = {
    0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7,
    4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

FORCE_INLINE
byte bits_count(uint64 data, bool use_abm = false) NO_EXCEPT
{
    if (use_abm) {
        return (byte) intrin_bits_count_64(data);
    } else { LIKELY
        return BIT_COUNT_LOOKUP_TABLE[data & 0xFF]
            + BIT_COUNT_LOOKUP_TABLE[(data >> 8) & 0xFF]
            + BIT_COUNT_LOOKUP_TABLE[(data >> 16) & 0xFF]
            + BIT_COUNT_LOOKUP_TABLE[(data >> 24) & 0xFF]
            + BIT_COUNT_LOOKUP_TABLE[(data >> 32) & 0xFF]
            + BIT_COUNT_LOOKUP_TABLE[(data >> 40) & 0xFF]
            + BIT_COUNT_LOOKUP_TABLE[(data >> 48) & 0xFF]
            + BIT_COUNT_LOOKUP_TABLE[(data >> 56) & 0xFF];
    }
}

FORCE_INLINE
byte bits_count(uint32 data, bool use_abm = false) NO_EXCEPT
{
    if (use_abm) UNLIKELY {
        return (byte) intrin_bits_count_32(data);
    } else { LIKELY
        return BIT_COUNT_LOOKUP_TABLE[data & 0xFF]
            + BIT_COUNT_LOOKUP_TABLE[(data >> 8) & 0xFF]
            + BIT_COUNT_LOOKUP_TABLE[(data >> 16) & 0xFF]
            + BIT_COUNT_LOOKUP_TABLE[(data >> 24) & 0xFF];
    }
}

CONSTEXPR FORCE_INLINE
byte bits_count(uint16 data) NO_EXCEPT
{
    return BIT_COUNT_LOOKUP_TABLE[data & 0xFF]
        + BIT_COUNT_LOOKUP_TABLE[(data >> 8) & 0xFF];
}

CONSTEXPR FORCE_INLINE
byte bits_count(uint8 data) NO_EXCEPT
{
    return BIT_COUNT_LOOKUP_TABLE[data];
}

/**
 * Writes the lowest bits of a value (MSB first, the counterpart of bits_consume_*)
 *
 * The written bits are cleared first, the rest of the current byte is not touched.
 *
 * @param BitWalk*  stream  Stream
 * @param uint32    value   Value
 * @param uint32    bits    Amount of bits (0 - 32)
 */
inline
void bits_write_32(BitWalk* stream, uint32 value, uint32 bits) NO_EXCEPT
{
    while (bits) {
        const uint32 remaining = 8 - stream->bit_pos;
        const uint32 take = bits < remaining ? bits : remaining;
        const uint32 shift = remaining - take;
        const byte mask = (byte) (((1U << take) - 1) << shift);
        const byte chunk = (byte) (((value >> (bits - take)) << shift) & mask);

        *stream->pos = (byte) ((*stream->pos & ~mask) | chunk);

        bits -= take;
        bits_walk(stream, take);
    }
}

inline
void bits_write_64(BitWalk* stream, uint64 value, uint32 bits) NO_EXCEPT
{
    if (bits > 32) {
        bits_write_32(stream, (uint32) (value >> 32), bits - 32);
        bits = 32;
    }

    bits_write_32(stream, (uint32) value, bits);
}

/**
 * Amount of bits to store any value in [0, range]
 */
CONSTEXPR FORCE_INLINE
uint32 bits_required(uint32 range) NO_EXCEPT
{
    return range ? 33 - (uint32) first_set_bit_l2r((int32) range) : 0;
}

FORCE_INLINE
uint32 bits_mask(uint32 bits) NO_EXCEPT
{
    return bits >= 32 ? 0xFFFFFFFF : ((1U << bits) - 1);
}

/**
 * Bounded bit streams for untrusted data (e.g. network packets)
 *
 * BitWalk has no bounds, BitWriter/BitReader check every access against the
 * buffer size. A corrupted or malicious input only sets overflow, it never reads
 * or writes out of bounds.
 *
 * Bit order: MSB first like bits_consume_* (independent of the endianness).
 *
 * A message has ONE serialize function that is used for both directions:
 *
 *      template <typename Stream>
 *      bool net_packet_serialize(Stream* stream, MyMessage* msg) {
 *          if (!serialize_uint(stream, &msg->level, 7)
 *              || !serialize_float_range(stream, &msg->x, 0.0f, 32.0f, 12)
 *          ) {
 *              return false;
 *          }
 *
 *          return true;
 *      }
 *
 *      BitWriter -> packs the struct, BitReader -> unpacks it
 *
 * The reader validates ranges: invalid data makes net_packet_serialize() return false
 * instead of producing values outside of the declared range.
 */

struct BitWriter {
    static const bool IS_WRITING = true;
    static const bool IS_READING = false;

    BitWalk walk;

    byte* start;
    uint32 capacity;

    bool overflow;
};

struct BitReader {
    static const bool IS_WRITING = false;
    static const bool IS_READING = true;

    // Only read, BitWalk has no const variant
    BitWalk walk;

    const byte* start;
    uint32 size;

    // Read past the end or invalid data
    bool overflow;
};

// Writer
////////////////////////////////////////

inline
void bit_writer_init(BitWriter* const __restrict writer, byte* const __restrict data, uint32 capacity) NO_EXCEPT
{
    writer->walk.pos = data;
    writer->walk.bit_pos = 0;
    writer->start = data;
    writer->capacity = capacity;
    writer->overflow = false;
}

FORCE_INLINE
uint32 bit_writer_bits(const BitWriter* const writer) NO_EXCEPT
{
    return (uint32) (writer->walk.pos - writer->start) * 8 + writer->walk.bit_pos;
}

/**
 * @param uint32 value  Value (only the lowest bits are written)
 * @param uint32 bits   Amount of bits (0 - 32)
 */
inline
void bit_write(BitWriter* const writer, uint32 value, uint32 bits) NO_EXCEPT
{
    ASSERT_TRUE(bits <= 32);

    if (writer->overflow || !bits) {
        return;
    }

    if (bit_writer_bits(writer) + bits > writer->capacity * 8) {
        writer->overflow = true;

        return;
    }

    bits_write_32(&writer->walk, value, bits);
}

/**
 * Pads with 0 bits to the next byte boundary
 */
inline
void bit_write_align(BitWriter* const writer) NO_EXCEPT
{
    if (writer->walk.bit_pos) {
        bit_write(writer, 0, 8 - writer->walk.bit_pos);
    }
}

inline
void bit_write_bytes(BitWriter* const __restrict writer, const byte* const __restrict data, uint32 size) NO_EXCEPT
{
    bit_write_align(writer);

    if (writer->overflow || !size) {
        return;
    }

    if (bit_writer_bits(writer) / 8 + size > writer->capacity) {
        writer->overflow = true;

        return;
    }

    memcpy(writer->walk.pos, data, size);
    writer->walk.pos += size;
}

/**
 * Pads the last partial byte
 *
 * @return uint32 Size in bytes, 0 on overflow
 */
inline
uint32 bit_writer_flush(BitWriter* const writer) NO_EXCEPT
{
    bit_write_align(writer);

    return writer->overflow ? 0 : (uint32) (writer->walk.pos - writer->start);
}

// Reader
////////////////////////////////////////

inline
void bit_reader_init(BitReader* const __restrict reader, const byte* const __restrict data, uint32 size) NO_EXCEPT
{
    reader->walk.pos = (byte *) data;
    reader->walk.bit_pos = 0;
    reader->start = data;
    reader->size = size;
    reader->overflow = false;
}

FORCE_INLINE
uint32 bit_reader_bits(const BitReader* const reader) NO_EXCEPT
{
    return (uint32) (reader->walk.pos - reader->start) * 8 + reader->walk.bit_pos;
}

FORCE_INLINE
uint32 bit_reader_remaining(const BitReader* const reader) NO_EXCEPT
{
    return reader->size * 8 - bit_reader_bits(reader);
}

/**
 * @param uint32 bits Amount of bits (0 - 32)
 *
 * @return uint32 Value, 0 if the end was reached (overflow is set)
 */
inline
uint32 bit_read(BitReader* const reader, uint32 bits) NO_EXCEPT
{
    ASSERT_TRUE(bits <= 32);

    if (reader->overflow || !bits) {
        return 0;
    }

    if (bits > bit_reader_remaining(reader)) {
        reader->overflow = true;

        return 0;
    }

    return bits_consume_32(&reader->walk, bits);
}

/**
 * Skips the padding to the next byte boundary (the padding must be 0)
 */
inline
void bit_read_align(BitReader* const reader) NO_EXCEPT
{
    if (reader->walk.bit_pos
        && bit_read(reader, 8 - reader->walk.bit_pos) != 0
    ) {
        reader->overflow = true;
    }
}

inline
void bit_read_bytes(BitReader* const __restrict reader, byte* const __restrict data, uint32 size) NO_EXCEPT
{
    bit_read_align(reader);

    if (reader->overflow || !size) {
        return;
    }

    if (size * 8 > bit_reader_remaining(reader)) {
        reader->overflow = true;

        return;
    }

    memcpy(data, reader->walk.pos, size);
    reader->walk.pos += size;
}

// Quantization
////////////////////////////////////////

/**
 * Maps a float in [min, max] to an integer with the given amount of bits (values outside are clamped)
 */
inline
uint32 quantize_float(f32 value, f32 min, f32 max, uint32 bits) NO_EXCEPT
{
    const f64 steps = (f64) bits_mask(bits);

    f64 t = ((f64) value - min) / ((f64) max - min);
    t = t < 0.0 ? 0.0 : (t > 1.0 ? 1.0 : t);

    return (uint32) (t * steps + 0.5);
}

inline
f32 dequantize_float(uint32 value, f32 min, f32 max, uint32 bits) NO_EXCEPT
{
    const f64 steps = (f64) bits_mask(bits);

    return (f32) ((f64) min + ((f64) value / steps) * ((f64) max - min));
}

/**
 * Max. error of a quantized value (half a step)
 */
FORCE_INLINE
f32 quantize_precision(f32 min, f32 max, uint32 bits) NO_EXCEPT
{
    return (f32) (((f64) max - min) / (f64) bits_mask(bits) * 0.5);
}

// Serialize (same code for both directions)
////////////////////////////////////////

/**
 * The only functions that differ between writing and reading
 *
 * Every serialize_* function is a template for both stream types. Before C++17
 * IF_CONSTEXPR is a normal if and both branches are compiled for both stream
 * types, so the direction is selected by overloading instead of
 * IF_CONSTEXPR (Stream::IS_WRITING) { bit_write(...) } else { bit_read(...) }.
 * IF_CONSTEXPR is only used for code that compiles for both streams (e.g. the
 * range checks).
 */
FORCE_INLINE
bool serialize_bits(BitWriter* stream, uint32* value, uint32 bits) NO_EXCEPT
{
    bit_write(stream, *value, bits);

    return !stream->overflow;
}

FORCE_INLINE
bool serialize_bits(BitReader* stream, uint32* value, uint32 bits) NO_EXCEPT
{
    *value = bit_read(stream, bits);

    return !stream->overflow;
}

FORCE_INLINE
bool serialize_align(BitWriter* stream) NO_EXCEPT
{
    bit_write_align(stream);

    return !stream->overflow;
}

FORCE_INLINE
bool serialize_align(BitReader* stream) NO_EXCEPT
{
    bit_read_align(stream);

    return !stream->overflow;
}

/**
 * Fixed amount of raw bytes (byte aligned)
 */
FORCE_INLINE
bool serialize_bytes(BitWriter* stream, byte* data, uint32 size) NO_EXCEPT
{
    bit_write_bytes(stream, data, size);

    return !stream->overflow;
}

FORCE_INLINE
bool serialize_bytes(BitReader* stream, byte* data, uint32 size) NO_EXCEPT
{
    bit_read_bytes(stream, data, size);

    return !stream->overflow;
}

/**
 * Unsigned integer with a fixed amount of bits
 *
 * Writing fails if the value doesn't fit (a value is never silently truncated).
 */
template <typename Stream, typename T>
inline
bool serialize_uint(Stream* stream, T* value, uint32 bits) NO_EXCEPT
{
    static_assert(sizeof(T) <= 4, "Use serialize_u64 for 64 bit values");

    uint32 v = 0;
    IF_CONSTEXPR (Stream::IS_WRITING) {
        v = (uint32) *value;
        if (bits < 32 && v > bits_mask(bits)) {
            stream->overflow = true;

            return false;
        }
    }

    serialize_bits(stream, &v, bits);

    IF_CONSTEXPR (Stream::IS_READING) {
        *value = (T) v;
    }

    return !stream->overflow;
}

/**
 * Integer in [min, max], only the bits for the range are used
 */
template <typename Stream, typename T>
inline
bool serialize_int_range(Stream* stream, T* value, int32 min, int32 max) NO_EXCEPT
{
    ASSERT_TRUE(min <= max);

    const uint32 range = (uint32) ((int64) max - min);
    const uint32 bits = bits_required(range);

    uint32 v = 0;
    IF_CONSTEXPR (Stream::IS_WRITING) {
        const int64 current = (int64) *value;
        if (current < min || current > max) {
            stream->overflow = true;

            return false;
        }

        v = (uint32) (current - min);
    }

    serialize_bits(stream, &v, bits);

    IF_CONSTEXPR (Stream::IS_READING) {
        // A corrupted value may use all bits of the field
        if (v > range) {
            stream->overflow = true;

            return false;
        }

        *value = (T) ((int64) min + v);
    }

    return !stream->overflow;
}

template <typename Stream>
inline
bool serialize_bool(Stream* stream, bool* value) NO_EXCEPT
{
    uint32 v = *value ? 1 : 0;
    serialize_bits(stream, &v, 1);
    *value = v != 0;

    return !stream->overflow;
}

template <typename Stream>
inline
bool serialize_u64(Stream* stream, uint64* value) NO_EXCEPT
{
    uint32 low = (uint32) *value;
    uint32 high = (uint32) (*value >> 32);

    serialize_bits(stream, &low, 32);
    serialize_bits(stream, &high, 32);

    *value = ((uint64) high << 32) | low;

    return !stream->overflow;
}

/**
 * Float without precision loss (32 bits)
 */
template <typename Stream>
inline
bool serialize_float(Stream* stream, f32* value) NO_EXCEPT
{
    uint32 v;
    memcpy(&v, value, sizeof(v));
    serialize_bits(stream, &v, 32);
    memcpy(value, &v, sizeof(v));

    return !stream->overflow;
}

/**
 * Float in [min, max] quantized to the given amount of bits
 *
 * Precision = (max - min) / (2^bits - 1), e.g. [0, 32] with 12 bits = 0.0078
 */
template <typename Stream>
inline
bool serialize_float_range(Stream* stream, f32* value, f32 min, f32 max, uint32 bits) NO_EXCEPT
{
    uint32 v = 0;
    IF_CONSTEXPR (Stream::IS_WRITING) {
        v = quantize_float(*value, min, max, bits);
    }

    serialize_bits(stream, &v, bits);

    IF_CONSTEXPR (Stream::IS_READING) {
        *value = dequantize_float(v, min, max, bits);
    }

    return !stream->overflow;
}

template <typename Stream>
inline
bool serialize_v3_range(Stream* stream, v3_f32* value, f32 min, f32 max, uint32 bits) NO_EXCEPT
{
    if (!serialize_float_range(stream, &value->x, min, max, bits)
        || !serialize_float_range(stream, &value->y, min, max, bits)
        || !serialize_float_range(stream, &value->z, min, max, bits)
    ) {
        return false;
    }

    return true;
}

/**
 * Rotation ("smallest three"): the largest component is left out and reconstructed
 * (|q| = 1), the other three are in [-1/sqrt(2), 1/sqrt(2)].
 *
 * Size = 2 + 3 * bits, e.g. 9 bits -> 29 bits instead of 128
 */
template <typename Stream>
inline
bool serialize_quaternion(Stream* stream, quaternion* value, uint32 bits) NO_EXCEPT
{
    const f32 limit = 0.70710678f;

    uint32 largest = 0;
    f32 others[3] = {0.0f, 0.0f, 0.0f};

    IF_CONSTEXPR (Stream::IS_WRITING) {
        f32 q[4] = {value->x, value->y, value->z, value->w};

        const f32 length = sqrtf(q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
        if (length > 0.0f) {
            for (int32 i = 0; i < 4; ++i) {
                q[i] /= length;
            }
        } else {
            q[3] = 1.0f;
        }

        for (uint32 i = 1; i < 4; ++i) {
            if (fabsf(q[i]) > fabsf(q[largest])) {
                largest = i;
            }
        }

        // q and -q are the same rotation -> the left out component is always positive
        const f32 sign = q[largest] < 0.0f ? -1.0f : 1.0f;

        int32 n = 0;
        for (uint32 i = 0; i < 4; ++i) {
            if (i != largest) {
                others[n++] = q[i] * sign;
            }
        }
    }

    serialize_bits(stream, &largest, 2);

    for (int32 i = 0; i < 3; ++i) {
        if (!serialize_float_range(stream, &others[i], -limit, limit, bits)) {
            return false;
        }
    }

    IF_CONSTEXPR (Stream::IS_READING) {
        if (stream->overflow) {
            return false;
        }

        const f32 sum = others[0] * others[0] + others[1] * others[1] + others[2] * others[2];
        const f32 missing = sum < 1.0f ? sqrtf(1.0f - sum) : 0.0f;

        f32 q[4];
        int32 n = 0;
        for (uint32 i = 0; i < 4; ++i) {
            q[i] = i == largest ? missing : others[n++];
        }

        value->x = q[0];
        value->y = q[1];
        value->z = q[2];
        value->w = q[3];
    }

    return !stream->overflow;
}

/**
 * Null terminated string in a fixed size field, only the used length is sent
 *
 * @param uint32 capacity Size of the field incl. the null terminator
 */
template <typename Stream>
inline
bool serialize_string(Stream* stream, char* value, uint32 capacity) NO_EXCEPT
{
    ASSERT_TRUE(capacity > 0);

    uint32 length = 0;
    IF_CONSTEXPR (Stream::IS_WRITING) {
        while (length < capacity && value[length]) {
            ++length;
        }

        if (length >= capacity) {
            stream->overflow = true;

            return false;
        }
    }

    if (!serialize_int_range(stream, &length, 0, (int32) capacity - 1)
        || !serialize_bytes(stream, (byte *) value, length)
    ) {
        return false;
    }

    IF_CONSTEXPR (Stream::IS_READING) {
        memset(value + length, 0, capacity - length);
    }

    return true;
}

/**
 * String in a fixed size field that doesn't need a null terminator (e.g. zero padded names), only the used length is sent
 *
 * @param uint32 size Size of the field
 */
template <typename Stream>
inline
bool serialize_string_fixed(Stream* stream, char* value, uint32 size) NO_EXCEPT
{
    uint32 length = 0;
    IF_CONSTEXPR (Stream::IS_WRITING) {
        while (length < size && value[length]) {
            ++length;
        }
    }

    if (!serialize_int_range(stream, &length, 0, (int32) size)
        || !serialize_bytes(stream, (byte *) value, length)
    ) {
        return false;
    }

    IF_CONSTEXPR (Stream::IS_READING) {
        memset(value + length, 0, size - length);
    }

    return true;
}

/**
 * String with an explicit length field (e.g. name + name_length), the length is sent once
 *
 * @param T*        length  Length field of the string
 * @param uint32    size    Size of the string field
 */
template <typename Stream, typename T>
inline
bool serialize_string_length(Stream* stream, char* value, T* length, uint32 size) NO_EXCEPT
{
    if (!serialize_int_range(stream, length, 0, (int32) size)
        || !serialize_bytes(stream, (byte *) value, (uint32) *length)
    ) {
        return false;
    }

    IF_CONSTEXPR (Stream::IS_READING) {
        memset(value + *length, 0, size - (uint32) *length);
    }

    return true;
}

#if OMS_LITTLE_ENDIAN
    #define SWAP_ENDIAN_LITTLE_SIMD(val, result, size, steps) ((void)0)
    #define SWAP_ENDIAN_BIG_SIMD(val, result, size, steps) endian_swap((val), (result), (size), (steps))
#else
    #define SWAP_ENDIAN_LITTLE_SIMD(val, result, size, steps) endian_swap((val), (result), (size), (steps))
    #define SWAP_ENDIAN_BIG_SIMD(val, result, size, steps) ((void)0)
#endif

#endif
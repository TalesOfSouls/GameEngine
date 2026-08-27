/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_UTILS_BIT_H
#define COMS_UTILS_BIT_H

#include "../stdlib/Stdlib.h"

struct BitWalk {
    byte* pos;
    uint32 bit_pos;
};

inline
void bits_walk(BitWalk* stream, uint32 bits_to_walk) NO_EXCEPT
{
    stream->bit_pos += bits_to_walk;
    stream->pos += stream->bit_pos / 8;
    stream->bit_pos %= 8;
}

inline
void bits_flush(BitWalk* stream) NO_EXCEPT
{
    if (stream->bit_pos > 0) {
        stream->bit_pos = 0;
        ++stream->pos;
    }
}

// inline
// uint8 bits_consume_8(BitWalk* stream, uint32 bits_to_consume)
// {
//     uint8 result;

//     uint32 remaining = 8 - stream->bit_pos;
//     uint32 range_1 = bits_to_consume >= remaining
//         ? remaining
//         : bits_to_consume;

//     result = (*stream->pos >> (remaining - range_1)) & ((1 << range_1) - 1);
//     stream->bit_pos += range_1;

//     if (bits_to_consume < remaining) {
//         return result;
//     }

//     ++stream->pos;
//     stream->bit_pos = 0;
//     bits_to_consume -= range_1;

//     /*
//     uint32 full_bytes = bits_to_consume / 8;
//     if (full_bytes > 0) {
//         for (int i = 0; i < full_bytes; ++i) {
//             result = (result << 8) | *stream->pos;

//             ++stream->pos;
//         }
//     }
//     */

//     if (bits_to_consume == 0) {
//         return result;
//     }

//     stream->bit_pos += bits_to_consume;

//     return (result << bits_to_consume) | ((*stream->pos >> (8 - bits_to_consume)) & ((1 << bits_to_consume) - 1));
// }

// inline
// uint16 bits_consume_16(BitWalk* stream, uint32 bits_to_consume)
// {
//     uint16 result;

//     uint32 remaining = 8 - stream->bit_pos;
//     uint32 range_1 = bits_to_consume >= remaining
//         ? remaining
//         : bits_to_consume;

//     result = (*stream->pos >> (remaining - range_1)) & ((1 << range_1) - 1);
//     stream->bit_pos += range_1;

//     if (bits_to_consume < remaining) {
//         return result;
//     }

//     ++stream->pos;
//     stream->bit_pos = 0;
//     bits_to_consume -= range_1;

//     uint32 full_bytes = bits_to_consume / 8;
//     if (full_bytes > 0) {
//         for (int i = 0; i < full_bytes; ++i) {
//             result = (result << 8) | *stream->pos;

//             ++stream->pos;
//         }
//     }

//     uint32 range_2 = bits_to_consume - full_bytes * 8;
//     if (range_2 == 0) {
//         return result;
//     }

//     stream->bit_pos += range_2;

//     return (result << range_2) | ((*stream->pos >> (8 - range_2)) & ((1 << range_2) - 1));
// }

// inline
// uint32 bits_consume_32(BitWalk* stream, uint32 bits_to_consume)
// {
//     uint32 result;

//     uint32 remaining = 8 - stream->bit_pos;
//     uint32 range_1 = bits_to_consume >= remaining
//         ? remaining
//         : bits_to_consume;

//     result = (*stream->pos >> (remaining - range_1)) & ((1 << range_1) - 1);
//     stream->bit_pos += range_1;

//     if (bits_to_consume < remaining) {
//         return result;
//     }

//     ++stream->pos;
//     stream->bit_pos = 0;
//     bits_to_consume -= range_1;

//     uint32 full_bytes = bits_to_consume / 8;
//     if (full_bytes > 0) {
//         for (int i = 0; i < full_bytes; ++i) {
//             result = (result << 8) | *stream->pos;

//             ++stream->pos;
//         }
//     }

//     uint32 range_2 = bits_to_consume - full_bytes * 8;
//     if (range_2 == 0) {
//         return result;
//     }

//     stream->bit_pos += range_2;

//     return (result << range_2) | ((*stream->pos >> (8 - range_2)) & ((1 << range_2) - 1));
// }

// inline
// uint64 bits_consume_64(BitWalk* stream, uint32 bits_to_consume)
// {
//     uint64 result;

//     uint32 remaining = 8 - stream->bit_pos;
//     uint32 range_1 = bits_to_consume >= remaining
//         ? remaining
//         : bits_to_consume;

//     result = (*stream->pos >> (remaining - range_1)) & ((1 << range_1) - 1);
//     stream->bit_pos += range_1;

//     if (bits_to_consume < remaining) {
//         return result;
//     }

//     ++stream->pos;
//     stream->bit_pos = 0;
//     bits_to_consume -= range_1;

//     uint32 full_bytes = bits_to_consume / 8;
//     if (full_bytes > 0) {
//         for (int i = 0; i < full_bytes; ++i) {
//             result = (result << 8) | *stream->pos;

//             ++stream->pos;
//         }
//     }

//     uint32 range_2 = bits_to_consume - full_bytes * 8;
//     if (range_2 == 0) {
//         return result;
//     }

//     stream->bit_pos += range_2;

//     return (result << range_2) | ((*stream->pos >> (8 - range_2)) & ((1 << range_2) - 1));
// }

// uint8 bits_peek_8(BitWalk* stream, uint32 bits_to_consume) {
//     byte* pos = stream->pos;
//     byte bit_pos = stream->bit_pos;

//     uint8 bits = bits_consume_8(stream, bits_to_consume);

//     stream->pos = pos;
//     stream->bit_pos = bit_pos;

//     return bits;
// }

// uint16 bits_peek_16(BitWalk* stream, uint32 bits_to_consume) {
//     byte* pos = stream->pos;
//     byte bit_pos = stream->bit_pos;

//     uint16 bits = bits_consume_16(stream, bits_to_consume);

//     stream->pos = pos;
//     stream->bit_pos = bit_pos;

//     return bits;
// }

// uint32 bits_peek_32(BitWalk* stream, uint32 bits_to_consume) {
//     byte* pos = stream->pos;
//     byte bit_pos = stream->bit_pos;

//     uint32 bits = bits_consume_32(stream, bits_to_consume);

//     stream->pos = pos;
//     stream->bit_pos = bit_pos;

//     return bits;
// }

// uint64 bits_peek_64(BitWalk* stream, uint32 bits_to_consume) {
//     byte* pos = stream->pos;
//     byte bit_pos = stream->bit_pos;

//     uint64 bits = bits_consume_64(stream, bits_to_consume);

//     stream->pos = pos;
//     stream->bit_pos = bit_pos;

//     return bits;
// }

inline
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

inline
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

inline
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

inline
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

inline
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

inline
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
    if (use_abm) UNLIKELY {
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

FORCE_INLINE
byte bits_count(uint16 data) NO_EXCEPT
{
    return BIT_COUNT_LOOKUP_TABLE[data & 0xFF]
        + BIT_COUNT_LOOKUP_TABLE[(data >> 8) & 0xFF];
}

FORCE_INLINE
byte bits_count(uint8 data) NO_EXCEPT
{
    return BIT_COUNT_LOOKUP_TABLE[data];
}

#if _WIN32 || __LITTLE_ENDIAN__
    #define SWAP_ENDIAN_LITTLE_SIMD(val, result, size, steps) ((void)0)
    #define SWAP_ENDIAN_BIG_SIMD(val, result, size, steps) endian_swap((val), (result), (size), (steps))
#else
    #define SWAP_ENDIAN_LITTLE_SIMD(val, result, size, steps) endian_swap((val), (result), (size), (steps))
    #define SWAP_ENDIAN_BIG_SIMD(val, result, size, steps) ((void)0)
#endif

#endif
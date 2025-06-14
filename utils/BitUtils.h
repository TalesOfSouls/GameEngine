/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_UTILS_BIT_H
#define COMS_UTILS_BIT_H

//#include <intrin.h>
#include "../stdlib/Types.h"
#include "../architecture/Intrinsics.h"

// @todo Replace many of these functions with intrinsic functions
//      This file can remain but the callers should get adjusted
//      Obviously we would have to check at runtime if ABM is supported

// Left to right (big endian)
// "bits" refers to the bits of the data type (e.g. 8, 16, 32, 64)
#define IS_BIT_SET_L2R(num, pos, bits) ((bool) ((num) & (1 << ((bits - 1) - (pos)))))
#define BIT_SET_L2R(num, pos, bits) ((num) | (1U << ((bits - 1) - (pos))))
#define BIT_UNSET_L2R(num, pos, bits) ((num) & ~(1U << ((bits - 1) - (pos))))
#define BIT_FLIP_L2R(num, pos, bits) ((num) ^ (1U << ((bits - 1) - (pos))))
#define BIT_SET_TO_L2R(num, pos, x, bits) (((num) & ~((uint32_t)(1) << ((bits) - 1 - (pos)))) | (((uint32_t)(x) & 1U) << ((bits) - 1 - (pos))))
#define BITS_GET_8_L2R(num, pos, to_read) (((num) >> (8 - (pos) - (to_read))) & ((1U << (to_read)) - 1))
#define BITS_GET_16_L2R(num, pos, to_read) (((num) >> (16 - (pos) - (to_read))) & ((1U << (to_read)) - 1))
#define BITS_GET_32_L2R(num, pos, to_read) (((num) >> (32 - (pos) - (to_read))) & ((1U << (to_read)) - 1))
#define BITS_GET_64_L2R(num, pos, to_read) (((num) >> (64 - (pos) - (to_read))) & ((1ULL << (to_read)) - 1))

// Merges an array of bytes as an int value (16bit, 32bit, 64bit)
// Depending on the endianness of the system you could simply cast the array
#define BYTES_MERGE_2_L2R(arr) (((arr)[0] << 8) | (arr)[1])
#define BYTES_MERGE_4_L2R(arr) (((arr)[0] << 24) | ((arr)[1] << 16) | ((arr)[2] << 8) | (arr)[3])
#define BYTES_MERGE_8_L2R(arr) (((uint64_t)(arr)[0] << 56) | ((uint64_t)(arr)[1] << 48) | ((uint64_t)(arr)[2] << 40) | ((uint64_t)(arr)[3] << 32) | ((uint64_t)(arr)[4] << 24) | ((uint64_t)(arr)[5] << 16) | ((uint64_t)(arr)[6] << 8)  | ((uint64_t)(arr)[7]))

// Right to left (little endian)
#define IS_BIT_SET_R2L(num, pos) ((bool) ((num) & (1 << (pos))))
#define IS_BIT_SET_64_R2L(num, pos) ((bool) ((num) & (1ULL << (pos))))
#define BIT_SET_R2L(num, pos) ((num) | ((uint32) 1 << (pos)))
#define BIT_UNSET_R2L(num, pos) ((num) & ~((uint32) 1 << (pos)))
#define BIT_FLIP_R2L(num, pos) ((num) ^ ((uint32) 1 << (pos)))
#define BIT_SET_TO_R2L(num, pos, x) (((num) & ~((uint32_t)(1) << (pos))) | ((uint32_t)(x) << (pos)))
// @performance Try to use this version over the L2R version for performance reasons
#define BITS_GET_8_R2L(num, pos, to_read) (((num) >> (pos)) & ((1U << (to_read)) - 1))
#define BITS_GET_16_R2L(num, pos, to_read) (((num) >> (pos)) & ((1U << (to_read)) - 1))
#define BITS_GET_32_R2L(num, pos, to_read) (((num) >> (pos)) & ((1U << (to_read)) - 1))
#define BITS_GET_64_R2L(num, pos, to_read) (((num) >> (pos)) & ((1ULL << (to_read)) - 1))

// Merges an array of bytes as an int value (16bit, 32bit, 64bit)
// Depending on the endianness of the system you could simply cast the array
#define BYTES_MERGE_2_R2L(arr) (((arr)[1] << 8) | (arr)[0])
#define BYTES_MERGE_4_R2L(arr) (((arr)[3] << 24) | ((arr)[2] << 16) | ((arr)[1] << 8) | (arr)[0])
#define BYTES_MERGE_8_R2L(arr) (((uint64_t)(arr)[7] << 56) | ((uint64_t)(arr)[6] << 48) | ((uint64_t)(arr)[5] << 40) | ((uint64_t)(arr)[4] << 32) | ((uint64_t)(arr)[3] << 24) | ((uint64_t)(arr)[2] << 16) | ((uint64_t)(arr)[1] << 8)  | ((uint64_t)(arr)[0]))

struct BitWalk {
    byte* pos;
    uint32 bit_pos;
};

inline
void bits_walk(BitWalk* stream, uint32 bits_to_walk) noexcept
{
    stream->bit_pos += bits_to_walk;
    stream->pos += stream->bit_pos / 8;
    stream->bit_pos %= 8;
}

inline
void bits_flush(BitWalk* stream) noexcept
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

static
inline int32 find_first_set_bit(int32 value) noexcept {
    if (value == 0) {
        return 0;
    }

    #if __GNUC__ || __clang__
        return __builtin_ffs(value);
    #elif _MSC_VER
        unsigned long index;
        if (_BitScanForward(&index, value)) {
            return (int32) index + 1;
        } else {
            return 0;
        }
    #else
        int32 index = 1;
        while (value) {
            if (value & 1) {
                return index;
            }
            value >>= 1;
            index++;
        }
        return 0;
    #endif
}

inline
uint32 bits_reverse(uint32 data, uint32 count) noexcept
{
    uint32 reversed = 0;
    for (uint32 i = 0; i <= (count / 2); ++i) {
        uint32 inv = count - i - 1;
        reversed |= ((data >> i) & 0x1) << inv;
        reversed |= ((data >> inv) & 0x1) << i;
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

inline
byte bits_count(uint64 data, bool use_abm = false) noexcept {
    if (use_abm) {
        return (byte) intrin_bits_count_64(data);
    } else {
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

inline
byte bits_count(uint32 data, bool use_abm = false) noexcept {
    if (use_abm) {
        return intrin_bits_count_32(data);
    } else {
        return BIT_COUNT_LOOKUP_TABLE[data & 0xFF]
            + BIT_COUNT_LOOKUP_TABLE[(data >> 8) & 0xFF]
            + BIT_COUNT_LOOKUP_TABLE[(data >> 16) & 0xFF]
            + BIT_COUNT_LOOKUP_TABLE[(data >> 24) & 0xFF];
    }
}

inline
byte bits_count(uint16 data) noexcept {
    return BIT_COUNT_LOOKUP_TABLE[data & 0xFF]
        + BIT_COUNT_LOOKUP_TABLE[(data >> 8) & 0xFF];
}

inline
byte bits_count(uint8 data) noexcept {
    return BIT_COUNT_LOOKUP_TABLE[data];
}

#endif
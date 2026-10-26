/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_ENCODING_HEX_H
#define COMS_ENCODING_HEX_H

#include "../stdlib/Stdlib.h"

/**
 * Lowercase hexadecimal alphabet.
 *
 * Used for hex strings inside text protocols, configuration files and logs.
 * Binary protocols should prefer the raw bytes to avoid the 2x memory overhead.
 */
CONSTEXPR const char HEX_CHARS_LOWER[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'a', 'b', 'c', 'd', 'e', 'f',
};

CONSTEXPR const char HEX_CHARS_UPPER[16] = {
    '0', '1', '2', '3', '4', '5', '6', '7',
    '8', '9', 'A', 'B', 'C', 'D', 'E', 'F',
};

/**
 * Convert a single hex character to its numeric value
 *
 * @param char c Character to convert
 *
 * @return int32 0-15 for a valid hex character, -1 otherwise
 */
FORCE_INLINE
int32 hex_char_to_int(char c) NO_EXCEPT
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }

    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }

    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }

    return -1;
}

/**
 * Check if a string only contains hex characters
 *
 * @param const char* src    String to check
 * @param size_t      length Amount of characters to check
 *
 * @return bool True if all characters are hex characters
 */
inline
bool hex_is_valid(const char* const __restrict src, size_t length) NO_EXCEPT
{
    if (!src || (length & 1)) {
        return false;
    }

    for (size_t i = 0; i < length; ++i) {
        if (hex_char_to_int(src[i]) < 0) {
            return false;
        }
    }

    return true;
}

/**
 * Encode binary data as lowercase hex string
 *
 * The destination buffer must be at least 2 * length + 1 bytes large
 * (the last byte is the null terminator).
 *
 * @param const byte* src    Binary data
 * @param size_t      length Amount of bytes to encode
 * @param char*       dst    Destination buffer
 *
 * @return int32 Amount of characters written (excluding the null terminator)
 */
inline
int32 hex_encode(
    const byte* const __restrict src,
    size_t length,
    char* const __restrict dst
) NO_EXCEPT
{
    ASSERT_TRUE(src);
    ASSERT_TRUE(dst);

    for (size_t i = 0; i < length; ++i) {
        dst[i * 2] = HEX_CHARS_LOWER[(src[i] >> 4) & 0x0F];
        dst[i * 2 + 1] = HEX_CHARS_LOWER[src[i] & 0x0F];
    }

    dst[length * 2] = '\0';

    return (int32) (length * 2);
}

/**
 * Encode binary data as uppercase hex string
 *
 * @param const byte* src    Binary data
 * @param size_t      length Amount of bytes to encode
 * @param char*       dst    Destination buffer
 *
 * @return int32 Amount of characters written (excluding the null terminator)
 */
inline
int32 hex_encode_upper(
    const byte* const __restrict src,
    size_t length,
    char* const __restrict dst
) NO_EXCEPT
{
    ASSERT_TRUE(src);
    ASSERT_TRUE(dst);

    for (size_t i = 0; i < length; ++i) {
        dst[i * 2] = HEX_CHARS_UPPER[(src[i] >> 4) & 0x0F];
        dst[i * 2 + 1] = HEX_CHARS_UPPER[src[i] & 0x0F];
    }

    dst[length * 2] = '\0';

    return (int32) (length * 2);
}

/**
 * Decode a hex string to binary data
 *
 * The destination buffer must be at least length / 2 bytes large.
 * The function does not write a null terminator since the result is binary.
 *
 * @param const char* src    Hex string
 * @param size_t      length Amount of characters to decode (must be even)
 * @param byte*       dst    Destination buffer
 *
 * @return int32 Amount of bytes written or -1 on invalid input
 */
inline
int32 hex_decode(
    const char* const __restrict src,
    size_t length,
    byte* const __restrict dst
) NO_EXCEPT
{
    ASSERT_TRUE(src);
    ASSERT_TRUE(dst);

    if (length & 1) {
        return -1;
    }

    for (size_t i = 0; i < length; i += 2) {
        const int32 high = hex_char_to_int(src[i]);
        const int32 low = hex_char_to_int(src[i + 1]);

        if (high < 0 || low < 0) {
            return -1;
        }

        dst[i / 2] = (byte) ((high << 4) | low);
    }

    return (int32) (length / 2);
}

#endif

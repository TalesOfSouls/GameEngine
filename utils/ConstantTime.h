/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_UTILS_CONSTANT_TIME_H
#define COMS_UTILS_CONSTANT_TIME_H

#include "../stdlib/Stdlib.h"

/**
 * Constant time helpers
 *
 * Comparing secrets (password hashes, signatures, tokens) with a normal memcmp
 * leaks the position of the first differing byte through the execution time.
 * All functions in here have a data independent execution time which makes them
 * safe to use for secret comparisons.
 *
 * The compiler is prevented from turning these functions into library calls by
 * using volatile accesses and explicit asm barriers where available.
 */

/**
 * Compare two byte buffers in constant time
 *
 * @param const void* a      First buffer
 * @param const void* b      Second buffer
 * @param size_t      length Amount of bytes to compare
 *
 * @return bool True if both buffers are equal
 */
inline
bool ct_memcmp(const void* const __restrict a, const void* const __restrict b, size_t length) NO_EXCEPT
{
    const volatile byte* const va = (const volatile byte *) a;
    const volatile byte* const vb = (const volatile byte *) b;

    byte diff = 0;

    for (size_t i = 0; i < length; ++i) {
        diff |= (byte) (va[i] ^ vb[i]);
    }

    return diff == 0;
}

/**
 * Compare two strings in constant time
 *
 * WARNING: The comparison is only constant time for strings of equal length.
 *          For secrets always compare fixed size binary data (e.g. signatures).
 *
 * @param const char* a         First string
 * @param size_t      a_length  Length of the first string
 * @param const char* b         Second string
 * @param size_t      b_length  Length of the second string
 *
 * @return bool True if both strings have the same length and content
 */
inline
bool ct_strcmp(
    const char* const __restrict a, size_t a_length,
    const char* const __restrict b, size_t b_length
) NO_EXCEPT
{
    if (a_length != b_length) {
        return false;
    }

    return ct_memcmp(a, b, a_length);
}

/**
 * Overwrite memory in a way the compiler cannot remove
 *
 * Used for passwords, keys and other secrets that should not linger in memory.
 *
 * @param void*  data   Memory to clear
 * @param size_t length Amount of bytes to clear
 */
inline
void ct_memzero(void* const data, size_t length) NO_EXCEPT
{
    volatile byte* const p = (volatile byte *) data;

    for (size_t i = 0; i < length; ++i) {
        p[i] = 0;
    }

    #if defined(__GNUC__) || defined(__clang__)
        __asm__ __volatile__("" : : "r"(data) : "memory");
    #endif
}

/**
 * Select a value without a data dependent branch
 *
 * @param uint32 mask Mask which is either all bits set (0xFFFFFFFF) or zero
 * @param uint32 a    Value returned if the mask is set
 * @param uint32 b    Value returned if the mask is not set
 *
 * @return uint32 Selected value
 */
FORCE_INLINE
uint32 ct_select(uint32 mask, uint32 a, uint32 b) NO_EXCEPT
{
    return (a & mask) | (b & ~mask);
}

/**
 * Convert a boolean into a selection mask (true = all bits set)
 *
 * @param bool value Boolean to convert
 *
 * @return uint32 Selection mask
 */
FORCE_INLINE
uint32 ct_mask(bool value) NO_EXCEPT
{
    return (uint32) -(int32) value;
}

#endif

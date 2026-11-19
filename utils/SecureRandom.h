/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_UTILS_SECURE_RANDOM_H
#define COMS_UTILS_SECURE_RANDOM_H

#include "../stdlib/Stdlib.h"

/**
 * Cryptographically secure random numbers
 *
 * The fast PRNG in RandomUtils is perfectly fine for particles or loot but it
 * MUST NOT be used for session ids, nonces or secrets. Those values have to be
 * unpredictable, otherwise an attacker could guess a session id and steal a
 * session.
 *
 * The platform files implement secure_random_bytes()
 */
#if _WIN32
    #include "../platform/win32/SecureRandom.h"
#elif __linux__
    #include "../platform/linux/SecureRandom.h"
#endif

/**
 * Generate a secure 64 bit value
 *
 * @param uint64* value Output value
 *
 * @return bool True on success
 */
inline
bool secure_random_u64(uint64* const value) NO_EXCEPT
{
    return secure_random_bytes(value, sizeof(*value));
}

/**
 * Generate a secure 32 bit value
 *
 * @param uint32* value Output value
 *
 * @return bool True on success
 */
inline
bool secure_random_u32(uint32* const value) NO_EXCEPT
{
    return secure_random_bytes(value, sizeof(*value));
}

#endif

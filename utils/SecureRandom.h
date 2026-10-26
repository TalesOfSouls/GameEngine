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
 * Linux:   getrandom() with a /dev/urandom fallback
 * Windows: BCryptGenRandom (link against bcrypt)
 */

#if __linux__
    #include <sys/random.h>
    #include <fcntl.h>
    #include <unistd.h>
    #include <errno.h>
#elif _WIN32
    #include <bcrypt.h>
#endif

/**
 * Fill a buffer with secure random bytes
 *
 * @param void*  data   Buffer
 * @param size_t length Amount of bytes
 *
 * @return bool True if the buffer was filled
 */
inline
bool secure_random_bytes(void* const data, size_t length) NO_EXCEPT
{
    if (!data || !length) {
        return false;
    }

#if __linux__
    byte* position = (byte *) data;
    size_t remaining = length;

    while (remaining) {
        const ssize_t received = getrandom(position, remaining, 0);

        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }

            break;
        }

        position += received;
        remaining -= (size_t) received;
    }

    if (!remaining) {
        return true;
    }

    // Fallback for kernels without getrandom()
    const int32 fd = open("/dev/urandom", O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return false;
    }

    position = (byte *) data;
    remaining = length;

    while (remaining) {
        const ssize_t received = read(fd, position, remaining);

        if (received <= 0) {
            if (received < 0 && errno == EINTR) {
                continue;
            }

            close(fd);

            return false;
        }

        position += received;
        remaining -= (size_t) received;
    }

    close(fd);

    return true;
#elif _WIN32
    return BCryptGenRandom(
        NULL,
        (PUCHAR) data,
        (ULONG) length,
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    ) == 0;
#else
    // Unknown platform: fail closed so no weak randomness is ever used
    return false;
#endif
}

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

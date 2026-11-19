/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_SECURE_RANDOM_H
#define COMS_PLATFORM_WIN32_SECURE_RANDOM_H

#include <windows.h>
#include <bcrypt.h>
#include "../../stdlib/Stdlib.h"

/**
 * Fill a buffer with secure random bytes
 * Uses BCryptGenRandom (link against bcrypt)
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

    return BCryptGenRandom(
        NULL,
        (PUCHAR) data,
        (ULONG) length,
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    ) == 0;
}

#endif

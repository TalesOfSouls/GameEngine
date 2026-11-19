/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_LINUX_SECURE_RANDOM_H
#define COMS_PLATFORM_LINUX_SECURE_RANDOM_H

#include "../../stdlib/Stdlib.h"
#include <sys/random.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

/**
 * Fill a buffer with secure random bytes
 * Uses getrandom() with a /dev/urandom fallback
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
}

#endif

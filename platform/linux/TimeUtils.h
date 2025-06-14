/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_LINUX_TIME_UTILS_H
#define COMS_PLATFORM_LINUX_TIME_UTILS_H

#include <time.h>
#include <sys/time.h>
#include "../../stdlib/Types.h"

uint64 system_time() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return (uint64_t) ts.tv_sec * 1000000ULL + (uint64_t) ts.tv_nsec / 1000ULL;
}

// Used as initializer for 64bit random number generators instead of time()
inline
uint64 time_index() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64) ts.tv_sec * 1000000ULL + (uint64) (ts.tv_nsec / 1000);
}

uint64 time_mu() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64) ts.tv_sec * 1000000ULL + (uint64) (ts.tv_nsec / 1000);
}

#endif
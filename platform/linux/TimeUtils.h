/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_LINUX_TIME_UTILS_H
#define COMS_PLATFORM_LINUX_TIME_UTILS_H

#include <time.h>
#include <ctime>
#include <sys/time.h>
#include <unistd.h>
#include "../../stdlib/Stdlib.h"
#include "../../log/PerformanceProfiler.h"

inline
void usleep(uint64 microseconds) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_SLEEP, (char *) NULL, PROFILE_FLAG_ADD_HISTORY);

    struct timespec start, now;
    clock_gettime(CLOCK_MONOTONIC, &start);
    uint64 target_ns = usec * 1000ULL;

    do {
        clock_gettime(CLOCK_MONOTONIC, &now);
        uint64 elapsed = (now.tv_sec - start.tv_sec) * 1000000000ULL
            + (now.tv_nsec - start.tv_nsec);

        if (elapsed >= target_ns) {
            break;
        }
    } while (true);
}

inline
uint64 system_time() NO_EXCEPT
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    struct tm local_tm;
    localtime_r(&ts.tv_sec, &local_tm);

    time_t local_epoch = mktime(&local_tm);

    return (uint64_t) local_epoch * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

inline
uint64 system_time_utc() NO_EXCEPT
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return (uint64) ts.tv_sec * 1000000ULL + (uint64) ts.tv_nsec / 1000ULL;
}

// Used as initializer for 64bit random number generators instead of time()
inline
uint64 time_index() NO_EXCEPT
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64) ts.tv_sec * 1000000ULL + (uint64) (ts.tv_nsec / 1000);
}

inline
uint64 time_mu() NO_EXCEPT
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);

    return (uint64) ts.tv_sec * 1000000ULL + (uint64) (ts.tv_nsec / 1000);
}

/**
 * Seconds since the unix epoch (UTC)
 *
 * Used for protocol timestamps and token expiry. The name matches the win32
 * implementation so the same code can be used on both platforms.
 *
 * @return uint64 Seconds since 1970-01-01 00:00:00 UTC
 */
inline
uint64 unix_epoch_s() NO_EXCEPT
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return (uint64) ts.tv_sec;
}

/**
 * Microseconds since the unix epoch (UTC)
 *
 * @return uint64 Microseconds since 1970-01-01 00:00:00 UTC
 */
inline
uint64 unix_epoch_us() NO_EXCEPT
{
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return (uint64) ts.tv_sec * 1000000ULL + (uint64) (ts.tv_nsec / 1000);
}

#endif
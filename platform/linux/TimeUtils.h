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
#include <unistd.h>
#include "../../stdlib/Types.h"
#include "../../log/PerformanceProfiler.h"

void usleep(uint64 microseconds)
{
    PROFILE(PROFILE_SLEEP, NULL, PROFILE_FLAG_ADD_HISTORY);

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

uint64 system_time() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    return (uint64) ts.tv_sec * 1000000ULL + (uint64) ts.tv_nsec / 1000ULL;
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
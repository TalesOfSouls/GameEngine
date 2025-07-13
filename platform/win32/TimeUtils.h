/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_TIME_UTILS_H
#define COMS_PLATFORM_WIN32_TIME_UTILS_H

#include <stdio.h>
#include <windows.h>
#include <time.h>
#include "../../stdlib/Types.h"

void usleep(uint64 microseconds)
{
    LARGE_INTEGER frequency, start, end;
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&start);

    long long target = start.QuadPart + (microseconds * frequency.QuadPart) / 1000000;

    do {
        QueryPerformanceCounter(&end);
    } while (end.QuadPart < target);
}

inline
uint64 system_time()
{
    SYSTEMTIME systemTime;
    FILETIME fileTime;
    ULARGE_INTEGER largeInt;

    GetLocalTime(&systemTime);
    SystemTimeToFileTime(&systemTime, &fileTime);

    // Convert FILETIME to a 64-bit integer
    largeInt.LowPart = fileTime.dwLowDateTime;
    largeInt.HighPart = fileTime.dwHighDateTime;

    return ((uint64) (largeInt.QuadPart / 10000000ULL)) - ((uint64) 11644473600ULL);
}

// Used as initializer for 64bit random number generators instead of time()
inline
uint64 time_index() {
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    return counter.QuadPart;
}

// doesn't return clock time, only to return time since program start
// -> can be used for profiling
inline
uint64 time_mu()
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    static LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    ASSERT_SIMPLE(counter.QuadPart != frequency.QuadPart);
    ASSERT_SIMPLE(counter.QuadPart != 1);

    return (counter.QuadPart * 1000000) / frequency.QuadPart;
}

inline
uint64 unix_epoch_s()
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    ULARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;

    return (uint64) (li.QuadPart - 116444736000000000ULL) / 10000000ULL;
}

inline
DWORD timespec_to_ms(const timespec* abstime)
{
    if (abstime == NULL) {
        return INFINITE;
    }

    uint64 seconds_since_epoch = unix_epoch_s();
    DWORD t = (DWORD) (((abstime->tv_sec - seconds_since_epoch) * 1000) + (abstime->tv_nsec / 1000000));

    return t < 0 ? 1 : t;
}

#endif
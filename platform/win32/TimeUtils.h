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

#include <windows.h>
#include "../../stdlib/Stdlib.h"
#include "../../log/PerformanceProfiler.h"

thread_local static LARGE_INTEGER _performance_frequency = []{
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    return f;
}();

inline
void usleep(uint64 microseconds) NO_EXCEPT
{
    PROFILE(PROFILE_SLEEP, NULL, PROFILE_FLAG_ADD_HISTORY);

    LARGE_INTEGER start, end;
    QueryPerformanceCounter(&start);

    const long long target = start.QuadPart
        + (microseconds * _performance_frequency.QuadPart)
        / 1000000ULL;

    do {
        QueryPerformanceCounter(&end);
    } while (end.QuadPart < target);
}

inline
uint64 system_time() NO_EXCEPT
{
    SYSTEMTIME system_time;
    FILETIME file_time;
    ULARGE_INTEGER li;

    GetLocalTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);

    // Convert FILETIME to a 64-bit integer
    li.LowPart = file_time.dwLowDateTime;
    li.HighPart = file_time.dwHighDateTime;

    return ((uint64) (li.QuadPart / 10000000ULL)) - ((uint64) 11644473600ULL);
}

inline
uint64 system_time_utc() NO_EXCEPT
{
    SYSTEMTIME system_time;
    FILETIME file_time;
    ULARGE_INTEGER li;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);

    // Convert FILETIME to a 64-bit integer
    li.LowPart = file_time.dwLowDateTime;
    li.HighPart = file_time.dwHighDateTime;

    return ((uint64) (li.QuadPart / 10000000ULL)) - ((uint64) 11644473600ULL);
}

// Used as initializer for 64bit random number generators instead of time()
FORCE_INLINE
uint64 time_index() NO_EXCEPT
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    return counter.QuadPart;
}

inline
uint64 time_mu() NO_EXCEPT
{
    LARGE_INTEGER counter;
    QueryPerformanceCounter(&counter);

    ASSERT_TRUE(counter.QuadPart != _performance_frequency.QuadPart);
    ASSERT_TRUE(counter.QuadPart != 1);

    return (counter.QuadPart / _performance_frequency.QuadPart) * 1000000ULL
        + (counter.QuadPart % _performance_frequency.QuadPart) * 1000000ULL
        / _performance_frequency.QuadPart;
}

inline
uint64 unix_epoch_s() NO_EXCEPT
{
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    ULARGE_INTEGER li;
    li.LowPart = ft.dwLowDateTime;
    li.HighPart = ft.dwHighDateTime;

    return (uint64) (li.QuadPart - 116444736000000000ULL) / 10000000ULL;
}

inline
DWORD timespec_to_ms(const timespec* const abstime) NO_EXCEPT
{
    if (abstime == NULL) {
        return INFINITE;
    }

    const uint64 seconds_since_epoch = unix_epoch_s();
    const DWORD t = (DWORD) (((abstime->tv_sec - seconds_since_epoch) * 1000)
        + (abstime->tv_nsec / 1000000)
    );

    return t < 0 ? 1 : t;
}

#endif
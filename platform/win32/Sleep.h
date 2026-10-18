/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_WIN32_TIME_SLEEP_H
#define COMS_PLATFORM_WIN32_TIME_SLEEP_H

#include "../../stdlib/Stdlib.h"

// WARNING: this is a busy sleep, use thread_usleep for idle sleep
inline
void usleep(uint64 microseconds) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_SLEEP, (char *) NULL, PROFILE_FLAG_ADD_HISTORY);

    LARGE_INTEGER start, end;
    QueryPerformanceCounter(&start);

    const long long target = start.QuadPart
        + (microseconds * _performance_frequency.QuadPart)
        / 1000000ULL;

    do {
        QueryPerformanceCounter(&end);
    } while (end.QuadPart < target);
}

#ifndef CREATE_WAITABLE_TIMER_HIGH_RESOLUTION
    #define CREATE_WAITABLE_TIMER_HIGH_RESOLUTION 0x00000002
#endif

typedef HANDLE (WINAPI *CreateWaitableTimerExW_t)(LPSECURITY_ATTRIBUTES, LPCWSTR, DWORD, DWORD);
typedef LONG (NTAPI  *NtQueryTimerResolution_t)(PULONG, PULONG, PULONG);

static CreateWaitableTimerExW_t _create_timer_ex = NULL;
static NtQueryTimerResolution_t _query_timer_res = NULL;
static volatile LONG _sleep_init = 0;

/**
 * It is insane but "older" Windows versions have very inaccurate and/or only very coarse sleep timers
 * We try to support older versions by setting up a mixed solution that either uses:
 *      1. the modern solution (CreateWaitableTimerExW)
 *      2. a mix of coarse sleep + busy sleep with a loop that checks every iteration
 */
static inline
void sleep_global_init() NO_EXCEPT
{
    // Atomics at home
    if(_sleep_init) {
        return;
    }

    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    HMODULE nt = GetModuleHandleW(L"ntdll.dll");

    // @legacy Always available on newer systems (WIN11)
    _create_timer_ex = (CreateWaitableTimerExW_t) GetProcAddress(k32, "CreateWaitableTimerExW");
    _query_timer_res = (NtQueryTimerResolution_t) GetProcAddress(nt, "NtQueryTimerResolution");

    // Atomics at home
    InterlockedExchange(&_sleep_init, 1);
}

// Current system timer resolution in microseconds (changes when any process calls timeBeginPeriod)
static inline
uint64 current_timer_resolution_us() NO_EXCEPT
{
    if (_query_timer_res) {
        ULONG min_res, max_res, cur_res; // 100ns units
        if (_query_timer_res(&min_res, &max_res, &cur_res) >= 0) {
            return (uint64) cur_res / 10;
        }
    }

    // default 15.625 ms
    return 15625;
}

// Converts us -> QPC ticks without overflow
static inline
LONGLONG us_to_qpc_ticks(uint64 us) NO_EXCEPT
{
    const LONGLONG f = _performance_frequency.QuadPart;
    return (LONGLONG) (us / 1000000) * f + (LONGLONG) (us % 1000000) * f / 1000000;
}

static inline
void hybrid_usleep(uint64 us) NO_EXCEPT
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);

    if (_performance_frequency.QuadPart == 0) {
        Sleep((DWORD) ((us + 999) / 1000));
        return;
    }

    const LONGLONG end = now.QuadPart + us_to_qpc_ticks(us);

    // One scheduler tick + 1ms safety, because Sleep(1) wakes on the next tick
    // We can't make this static since the resolution could change any time, it's a system global
    // Yes, that's right :(
    const LONGLONG spin_ticks = us_to_qpc_ticks(current_timer_resolution_us() + 1000);

    // Approximate solution
    while (end - now.QuadPart > spin_ticks) {
        Sleep(1);
        QueryPerformanceCounter(&now);
    }

    // Fine tune the sleep
    while (now.QuadPart < end) {
        YieldProcessor();
        QueryPerformanceCounter(&now);
    }
}

/**
 * It is insane but "older" Windows versions have very inaccurate and/or only very coarse sleep timers
 * We try to support older versions by setting up a mixed solution that either uses:
 *      1. the modern solution (CreateWaitableTimerExW)
 *      2. a mix of coarse sleep + busy sleep with a loop that checks every iteration
 */
void thread_usleep(uint64 us) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_SLEEP, (char *) NULL, PROFILE_FLAG_ADD_HISTORY);

    static thread_local HANDLE timer = NULL;
    static thread_local bool timer_init = false;

    if (!timer_init) {
        timer_init = true;
        // @legacy
        sleep_global_init();

        // Only the high-resolution timer is useful,
        // a normal waitable timer has Sleep() granularity
        if (_create_timer_ex) {
            // The insane part is, this doesn't even work for all WIN10 versions
            timer = _create_timer_ex(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
        }
    }

    if (timer) {
        LARGE_INTEGER due;
        due.QuadPart = -((LONGLONG) us * 10);

        if (SetWaitableTimer(timer, &due, 0, NULL, NULL, FALSE)) {
            WaitForSingleObject(timer, INFINITE);
            return;
        }
    }

    hybrid_usleep(us);
}

#endif
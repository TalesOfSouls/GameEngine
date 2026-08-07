/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_THREADS_SPINLOCK_STANDALONE_H
#define COMS_THREADS_SPINLOCK_STANDALONE_H

#include "../stdlib/Stdlib.h"

/**
 * WARNING: This implementation is ONLY used by core framework code such as the debugging/logging code.
 * We need this standalone implementation besides the regular spinlock implementation
 * because the regular spinlock implementation also does performance logs.
 * This behaviour would create a circular dependency if we would use the standard implementation
 * in any of our debugging/logging code.
 *
 * Why do we want performance profilling in the standard spinlock implementation?
 *  > Well, we want to profile deadlocks
 */

// We need to make some platform specific defines here to avoid including other header files
// Other header files most likely would also use the log header creating circular dependencies
// As a result we are kinda implementing some things twice
#if _WIN32
    #include <windows.h>

    thread_local static LARGE_INTEGER _standalone_performance_frequency = []{
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        return f;
    }();

    /**
     * Get the system time
     *
     * @return uint64
     */
    static inline HOT_CODE
    uint64 standalone_sys_time() NO_EXCEPT
    {
        SYSTEMTIME sys_time;
        FILETIME file_time;
        ULARGE_INTEGER large_int;

        GetLocalTime(&sys_time);
        SystemTimeToFileTime(&sys_time, &file_time);

        // Convert FILETIME to a 64-bit integer
        large_int.LowPart = file_time.dwLowDateTime;
        large_int.HighPart = file_time.dwHighDateTime;

        return ((uint64) (large_int.QuadPart / 10000000ULL)) - ((uint64) 11644473600ULL);
    }

    typedef volatile long standalone_spinlock32;

    /**
     * Initialize the log specific spinlock
     *
     * @param standalone_spinlock32* lock Spinlock variable
     *
     * @return void
     */
    FORCE_INLINE
    void standalone_spinlock_init(standalone_spinlock32* const lock) NO_EXCEPT
    {
        *lock = 0;
    }

    /**
     * Start the spinlock
     *
     * @param standalone_spinlock32*   lock    Spinlock variable
     * @param int32             delay   Minimum amount of time spend befor rechecking spinlock
     *
     * @return void
     */
    static FORCE_INLINE HOT_CODE
    void standalone_spinlock_start(standalone_spinlock32* const lock, int32 delay = 10) NO_EXCEPT
    {
        while (*lock || InterlockedExchange(lock, 1)) {
            LARGE_INTEGER start, end;
            QueryPerformanceCounter(&start);

            const long long target = start.QuadPart
                + (delay * _standalone_performance_frequency.QuadPart)
                / 1000000ULL;

            do {
                QueryPerformanceCounter(&end);
            } while (end.QuadPart < target);
        }
    }

    /**
     * End/unlock the spinlock
     *
     * @param standalone_spinlock32* lock Spinlock variable
     *
     * @return void
     */
    static FORCE_INLINE HOT_CODE
    void standalone_spinlock_end(standalone_spinlock32* const lock) NO_EXCEPT
    {
        InterlockedExchange(lock, 0);
    }
#elif __linux__
    #include <time.h>
    #include <sys/time.h>
    #include <unistd.h>

    /**
     * Get the system time
     *
     * @return uint64
     */
    static inline HOT_CODE
    uint64 standalone_sys_time() NO_EXCEPT
    {
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);

        return (uint64) ts.tv_sec * 1000000ULL + (uint64) ts.tv_nsec / 1000ULL;
    }

    typedef volatile int32 standalone_spinlock32;

    /**
     * Initialize the log specific spinlock
     *
     * @param standalone_spinlock32* lock Spinlock variable
     *
     * @return void
     */
    FORCE_INLINE
    void standalone_spinlock_init(standalone_spinlock32* const lock) NO_EXCEPT
    {
        *lock = 0;
    }

    /**
     * Start the spinlock
     *
     * @param standalone_spinlock32*   lock    Spinlock variable
     * @param int32             delay   Minimum amount of time spend befor rechecking spinlock
     *
     * @return void
     */
    static FORCE_INLINE HOT_CODE
    void standalone_spinlock_start(standalone_spinlock32* const lock, int32 delay = 10) NO_EXCEPT
    {
        while (__atomic_load_n(lock, __ATOMIC_ACQUIRE)
            || __atomic_exchange_n(lock, 1, __ATOMIC_ACQUIRE)
        ) {
            struct timespec start, now;
            clock_gettime(CLOCK_MONOTONIC, &start);
            const uint64 target_ns = usec * 1000ULL;

            do {
                clock_gettime(CLOCK_MONOTONIC, &now);
                const uint64 elapsed = (now.tv_sec - start.tv_sec) * 1000000000ULL
                    + (now.tv_nsec - start.tv_nsec);

                if (elapsed >= target_ns) {
                    break;
                }
            } while (true);
        }
    }

    /**
     * End/unlock the spinlock
     *
     * @param standalone_spinlock32* lock Spinlock variable
     *
     * @return void
     */
    static FORCE_INLINE HOT_CODE
    void standalone_spinlock_end(standalone_spinlock32* const lock) NO_EXCEPT
{
        __atomic_store_n(lock, 0, __ATOMIC_RELEASE);
    }
#endif

// By using this constructor/destructor pattern you can avoid deadlocks in case of exceptions
// Why? Well because if we go out of scope the destructor is automatically called and the lock is unlocked
struct StandaloneSpinlockGuard {
    standalone_spinlock32* _lock = NULL;

    inline HOT_CODE
    explicit StandaloneSpinlockGuard(standalone_spinlock32* const lock, int32 delay = 10) {
        this->_lock = lock;

        standalone_spinlock_start(this->_lock, delay);
    }

    inline HOT_CODE
    void unlock() {
        if (this->_lock) {
            standalone_spinlock_end(this->_lock);
            this->_lock = NULL;
        }
    }

    inline HOT_CODE
    ~StandaloneSpinlockGuard() {
        this->unlock();
    }
};

#endif
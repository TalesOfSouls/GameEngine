/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_THREADS_THREAD_HELPER_C
#define COMS_THREADS_THREAD_HELPER_C

#include "ThreadDefines.h"

#if _WIN32
    #include "../platform/win32/threading/ThreadHelper.cpp"
#elif __linux__
    #include "../platform/linux/threading/ThreadHelper.cpp"
#endif

// By using this constructor/destructor pattern you can avoid deadlocks in case of exceptions
// Why? Well because if we go out of scope the destructor is automatically called and the lock is unlocked
struct MutexGuard {
    mutex* _mutex = NULL;

    FORCE_INLINE HOT_CODE
    explicit MutexGuard(mutex* const mut) {
        this->_mutex = mut;

        mutex_lock(this->_mutex);
    }

    FORCE_INLINE HOT_CODE
    void unlock() {
        if (this->_mutex) {
            mutex_unlock(this->_mutex);
            this->_mutex = NULL;
        }
    }

    FORCE_INLINE HOT_CODE
    ~MutexGuard() {
        this->unlock();
    }
};

#endif
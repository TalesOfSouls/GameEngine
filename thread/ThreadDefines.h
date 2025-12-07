/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_THREADS_THREAD_DEFINES_H
#define COMS_THREADS_THREAD_DEFINES_H

#include "../stdlib/Types.h"

int32 thread_local _thread_local_id = 0;
int32 thread_local _thread_cpu_id = 0;

#if _WIN32
    #include "../platform/win32/threading/ThreadDefines.h"
#elif __linux__
    #include "../platform/linux/threading/ThreadDefines.h"
#endif

// By using this constructor/destructor pattern you can avoid deadlocks in case of exceptions
// Why? Well because if we go out of scope the destructor is automatically called and the lock is unlocked
struct MutexGuard {
    mutex* _mutex = NULL;

    inline HOT_CODE
    MutexGuard(mutex* const mut) {
        this->_mutex = mut;

        mutex_lock(this->_mutex);
    }

    inline HOT_CODE
    void unlock() {
        if (this->_mutex) {
            mutex_unlock(this->_mutex);
            this->_mutex = NULL;
        }
    }

    inline HOT_CODE
    ~MutexGuard() {
        this->unlock();
    }
};

#endif
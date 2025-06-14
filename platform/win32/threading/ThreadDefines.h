/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_THREADING_THREAD_DEFINES_H
#define COMS_PLATFORM_WIN32_THREADING_THREAD_DEFINES_H

#include "../../../stdlib/Types.h"
#include "../../../thread/Atomic.h"
#include <windows.h>

#define THREAD_RETURN DWORD WINAPI
typedef DWORD (WINAPI *ThreadJobFunc)(void*);
typedef CRITICAL_SECTION mutex;
typedef void mutexattr_t;
typedef void coms_pthread_condattr_t;
typedef void coms_pthread_rwlockattr_t;

struct coms_pthread_t {
    HANDLE h;
    void* stack;
};

typedef CONDITION_VARIABLE mutex_cond;

// Thread local variable Already exists in c++11
// #define thread_local __declspec(thread)

struct coms_pthread_rwlock_t {
    SRWLOCK lock;
    bool exclusive;
};

inline
int32 mutex_init(mutex* mutex, mutexattr_t*)
{
    if (mutex == NULL) {
        return 1;
    }

    InitializeCriticalSection(mutex);

    return 0;
}

inline
int32 mutex_destroy(mutex* mutex)
{
    if (mutex == NULL) {
        return 1;
    }

    DeleteCriticalSection(mutex);

    return 0;
}

inline
int32 mutex_lock(mutex* mutex)
{
    if (mutex == NULL) {
        return 1;
    }

    EnterCriticalSection(mutex);

    return 0;
}

inline
int32 mutex_unlock(mutex* mutex)
{
    if (mutex == NULL) {
        return 1;
    }

    LeaveCriticalSection(mutex);

    return 0;
}

#endif
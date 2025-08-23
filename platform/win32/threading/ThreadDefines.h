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
#define THREAD_RETURN_BODY DWORD
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

// This way we don't have to use sys calls more than once to get the thread id.
int32 thread_local _thread_local_id;

// We use Macros to avoid the function call overhead and exception handling overhead
#define mutex_init(mutex, attr) { \
    ASSERT_TRUE((mutex)); \
    InitializeCriticalSection((mutex)); \
}

#define mutex_destroy(mutex) { \
    ASSERT_TRUE((mutex)); \
    DeleteCriticalSection((mutex)); \
}

#define mutex_lock(mutex) { \
    ASSERT_TRUE((mutex)); \
    EnterCriticalSection((mutex)); \
}

#define mutex_unlock(mutex) { \
    ASSERT_TRUE((mutex)); \
    LeaveCriticalSection((mutex)); \
}

#endif
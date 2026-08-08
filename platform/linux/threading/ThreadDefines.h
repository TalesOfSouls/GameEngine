/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_LINUX_THREADING_THREAD_DEFINES_H
#define COMS_PLATFORM_LINUX_THREADING_THREAD_DEFINES_H

// #include <pthread.h>
// #include <unistd.h>

#include "../../../stdlib/Stdlib.h"
#include "../../../thread/Atomic.h"
#include "../../../log/PerformanceProfiler.h"
#include <linux/futex.h>
#include <sys/syscall.h>

#define THREAD_RETURN int32
typedef THREAD_RETURN (*ThreadJobFunc)(void*);

struct mutex {
    atomic<int32> futex;
};

typedef void mutexattr_t;
typedef void coms_pthread_condattr_t;
typedef void coms_pthread_rwlockattr_t;

struct mutex_cond {
    atomic<int32> futex;
};

struct coms_pthread_rwlock_t {
    atomic<int32> futex;
    bool exclusive{false};
};

struct coms_pthread_t {
    int32 id;
    int h;
    void* stack;
};

inline int futex_wait(atomic<int32>* futex, int32 val)
{
    return syscall(
        SYS_futex,
        (int32 *) (futex),
        FUTEX_WAIT,
        val,
        NULL,
        NULL,
        0
    );
}

inline int futex_wake(atomic<int32>* futex, int n)
{
    return syscall(
        SYS_futex,
        (int32 *) (futex),
        FUTEX_WAKE,
        n,
        NULL,
        NULL,
        0
    );
}

#define mutex_init(a, b) ((void)0)
#define mutex_destroy(a, b) ((void)0)

#define mutex_lock(mtx)                                                     \
do {                                                                        \
    ASSERT_TRUE((mtx));                                                     \
    while ((mtx)->futex.exchange(1, memory_order_acquire) != 0) {      \
        futex_wait(&(mtx)->futex, 1);                                       \
    }                                                                       \
} while (0)

#define mutex_unlock(mtx)                                                   \
do {                                                                        \
    ASSERT_TRUE((mtx));                                                     \
    (mtx)->futex.store(0, memory_order_release);                        \
    futex_wake(&(mtx)->futex, 1);                                           \
} while (0)

#endif
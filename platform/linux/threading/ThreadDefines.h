/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_LINUX_THREADING_THREAD_DEFINES_H
#define COMS_PLATFORM_LINUX_THREADING_THREAD_DEFINES_H

// #include <pthread.h>
// #include <unistd.h>

#include "../../../stdlib/Stdlib.h"
#include "../../../thread/Atomic.h"
#include <linux/futex.h>
#include <sys/syscall.h>

#define THREAD_RETURN int32
typedef THREAD_RETURN (*ThreadJobFunc)(void*);

struct mutex {
    atomic_32 int32 futex;
};

typedef void mutexattr_t;
typedef void coms_pthread_condattr_t;
typedef void coms_pthread_rwlockattr_t;

struct mutex_cond {
    atomic_32 int32 futex;
} ;

struct coms_pthread_rwlock_t {
    atomic_32 int32 futex;
    bool exclusive;
};

struct coms_pthread_t {
    int h;
    void* stack;
};

#define futex_wait(futex, val) syscall(SYS_futex, futex, FUTEX_WAIT, val, NULL, NULL, 0)
#define futex_wake(futex, n) syscall(SYS_futex, futex, FUTEX_WAKE, n, NULL, NULL, 0)

#define mutex_init(a, b) ((void) 0)
#define mutex_init(a, b, c) ((void) 0)
#define mutex_destroy(a, b) ((void) 0)

#define mutex_lock(mutex) { \
    ASSERT_TRUE((mutex)); \
    while (atomic_fetch_set_acquire(&(mutex)->futex, 1) != 0) { \
        futex_wait(&(mutex)->futex, 1); \
    } \
}

#define mutex_unlock(mutex) { \
    ASSERT_TRUE((mutex)); \
    atomic_set_release(&(mutex)->futex, 0); \
    futex_wake(&(mutex)->futex, 1); \
}

#endif
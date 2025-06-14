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

#include "../../../stdlib/Types.h"
#include "../../../thread/Atomic.h"
#include <linux/futex.h>
#include <sys/syscall.h>

#define THREAD_RETURN int32
typedef THREAD_RETURN (*ThreadJobFunc)(void*);

struct mutex {
    alignas(4) atomic_32 int32 futex;
};

typedef void mutexattr_t;
typedef void coms_pthread_condattr_t;
typedef void coms_pthread_rwlockattr_t;

struct mutex_cond {
    alignas(4) atomic_32 int32 futex;
} ;

struct coms_pthread_rwlock_t {
    alignas(4) atomic_32 int32 futex;
    bool exclusive;
};

struct coms_pthread_t {
    int h;
    void* stack;
};

FORCE_INLINE
int32 futex_wait(volatile int32* futex, int32 val) {
    return syscall(SYS_futex, futex, FUTEX_WAIT, val, NULL, NULL, 0);
}

FORCE_INLINE
int32 futex_wake(volatile int32* futex, int32 n) {
    return syscall(SYS_futex, futex, FUTEX_WAKE, n, NULL, NULL, 0);
}

FORCE_INLINE
int32 mutex_init(mutex* mutex, mutexattr_t*) {
    return mutex == NULL ? 1 : 0;
}

FORCE_INLINE
int32 mutex_destroy(mutex* mutex) {
    return mutex == NULL ? 1 : 0;
}

inline
int32 mutex_lock(mutex* mutex) {
    if (mutex == NULL) {
        return 1;
    }

    while (atomic_fetch_set_acquire(&mutex->futex, 1) != 0) {
        futex_wait(&mutex->futex, 1);
    }

    return 0;
}

inline
int32 mutex_unlock(mutex* mutex) {
    if (mutex == NULL) {
        return 1;
    }

    atomic_set_release(&mutex->futex, 0);
    futex_wake(&mutex->futex, 1);

    return 0;
}

#endif
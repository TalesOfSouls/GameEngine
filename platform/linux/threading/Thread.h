/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_LINUX_THREADING_THREAD_H
#define COMS_PLATFORM_LINUX_THREADING_THREAD_H

#include <sched.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <linux/futex.h>
#include <sys/syscall.h>

#include "../../../stdlib/Types.h"
#include "../../../compiler/CompilerUtils.h"
#include "../Allocator.h"
#include "ThreadDefines.h"
#include "Atomic.h"
#include "../../../log/PerformanceProfiler.h"

inline
int32 coms_pthread_create(coms_pthread_t* thread, void*, ThreadJobFunc start_routine, void* arg) NO_EXCEPT
{
    // @question Do we want to pin threads automatically to p cores based on a utilization score?
    ASSERT_TRUE(thread);
    ASSERT_TRUE(start_routine);

    const uint64 stack_size = 1 * MEGABYTE;
    thread->stack = platform_alloc_aligned(stack_size, 64);

    int32 flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD | CLONE_SYSVSEM;

    // The + stack_size is required since the stack is used "downwards"
    thread->h = clone((int32 (*)(void*))start_routine, (void *) ((uintptr_t) thread->stack + stack_size), flags, arg);
    if (thread->h == -1) {
        LOG_1("Thread creation failed");
        return -1;
    }

    int32 thread_id = (int32) thread->h;
    THREAD_LOG_CREATE(thread_id);

    return thread_id;
}

FORCE_INLINE
int32 coms_pthread_join(coms_pthread_t thread, void** retval) NO_EXCEPT
{
    THREAD_LOG_DELETE((int32) thread.h);

    int32 res = syscall(SYS_waitid, P_PID, thread, retval, WEXITED, NULL) == -1
        ? 1
        : 0;

    platform_aligned_free((void **) &thread.stack);

    return res;
}

FORCE_INLINE
int32 coms_pthread_detach([[maybe_unused]] coms_pthread_t thread) NO_EXCEPT
{
    // In Linux, threads are automatically detached when they exit.
    return 0;
}

FORCE_INLINE
int32 coms_pthread_cond_init(mutex_cond* cond, coms_pthread_condattr_t*) NO_EXCEPT
{
    ASSERT_TRUE(cond);

    cond->futex = 0;

    return 0;
}

FORCE_INLINE
int32 coms_pthread_cond_destroy(mutex_cond* cond) NO_EXCEPT
{
    return cond == NULL ? 1 : 0;
}

inline
int32 mutex_condimedwait(mutex_cond* cond, mutex* mutex, const struct timespec*) NO_EXCEPT
{
    ASSERT_TRUE(cond);
    ASSERT_TRUE(mutex);

    int32 oldval = atomic_get_acquire(&cond->futex);
    mutex_unlock(mutex);
    futex_wait(&cond->futex, oldval);
    mutex_lock(mutex);

    return 0;
}

FORCE_INLINE
int32 coms_pthread_cond_wait(mutex_cond* cond, mutex* mutex) NO_EXCEPT
{
    return mutex_condimedwait(cond, mutex, NULL);
}

FORCE_INLINE
int32 coms_pthread_cond_signal(mutex_cond* cond) NO_EXCEPT
{
    ASSERT_TRUE(cond);

    atomic_increment_release(&cond->futex);
    futex_wake(&cond->futex, 1);

    return 0;
}

FORCE_INLINE
int32 coms_pthread_cond_broadcast(mutex_cond* cond) NO_EXCEPT
{
    ASSERT_TRUE(cond);

    atomic_increment_release(&cond->futex);
    futex_wake(&cond->futex, INT32_MAX);

    return 0;
}

FORCE_INLINE
int32 coms_pthread_rwlock_init(coms_pthread_rwlock_t* rwlock, const coms_pthread_rwlockattr_t*) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);

    rwlock->futex = 0;
    rwlock->exclusive = false;

    return 0;
}

FORCE_INLINE
int32 coms_pthread_rwlock_destroy(coms_pthread_rwlock_t* rwlock) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);

    return 0;
}

inline
int32 coms_pthread_rwlock_rdlock(coms_pthread_rwlock_t* rwlock) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);

    while (true) {
        int32 val = atomic_get_acquire(&rwlock->futex);
        if (val >= 0 && __atomic_compare_exchange_n(&rwlock->futex, &val, val + 1, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
            break;
        }
        futex_wait(&rwlock->futex, val);
    }

    return 0;
}

inline
int32 coms_pthread_rwlock_tryrdlock(coms_pthread_rwlock_t* rwlock) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);

    int32 val = atomic_get_acquire(&rwlock->futex);
    if (val >= 0 && __atomic_compare_exchange_n(&rwlock->futex, &val, val + 1, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
        return 0;
    }

    return 1;
}

inline
int32 coms_pthread_rwlock_wrlock(coms_pthread_rwlock_t* rwlock) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);

    while (true) {
        int32 val = atomic_get_acquire(&rwlock->futex);
        if (val == 0 && __atomic_compare_exchange_n(&rwlock->futex, &val, -1, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
            rwlock->exclusive = true;
            break;
        }

        futex_wait(&rwlock->futex, val);
    }

    return 0;
}

inline
int32 coms_pthread_rwlock_trywrlock(coms_pthread_rwlock_t* rwlock) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);

    int32 val = atomic_get_acquire(&rwlock->futex);
    if (val == 0 && __atomic_compare_exchange_n(&rwlock->futex, &val, -1, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
        rwlock->exclusive = true;
        return 0;
    }

    return 1;
}

inline
int32 coms_pthread_rwlock_unlock(coms_pthread_rwlock_t* rwlock) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);

    if (rwlock->exclusive) {
        rwlock->exclusive = false;
        atomic_set_release(&rwlock->futex, 0);
        futex_wake(&rwlock->futex, 1);
    } else {
        int32 val = atomic_decrement_release(&rwlock->futex);
        if (val == 0) {
            futex_wake(&rwlock->futex, 1);
        }
    }

    return 0;
}

FORCE_INLINE
uint32 pcthread_get_num_procs() NO_EXCEPT
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}

#define coms_pthread_exit(a) { return (a); }

// WARNING: Rather use _thread_local_id variable to avoid the syscall
FORCE_INLINE
int32 thread_current_id()
{
    return (int32) syscall(SYS_gettid);
}

#endif
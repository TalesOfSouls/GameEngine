/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_PLATFORM_LINUX_THREADING_THREAD_H
#define COMS_PLATFORM_LINUX_THREADING_THREAD_H

#include <sched.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sched.h>

#include "../../../stdlib/Stdlib.h"
#include "../../../compiler/CompilerUtils.h"
#include "../Allocator.h"
#include "ThreadDefines.h"
#include "Atomic.h"
#include "../../../log/PerformanceProfiler.h"

inline
void coms_thread_affinity_set(coms_pthread_t* thread, int64 mask) NO_EXCEPT
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(mask, &cpuset);

    sched_setaffinity(0, sizeof(cpuset), &cpuset);
}

inline
int32 coms_pthread_create(coms_pthread_t* __restrict thread, void*, ThreadJobFunc start_routine, void* __restrict arg) NO_EXCEPT
{
    // @question Do we want to pin threads automatically to p cores based on a utilization score?
    ASSERT_TRUE(thread);
    ASSERT_TRUE(start_routine);

    const uint64 stack_size = 1 * MEGABYTE;
    thread->stack = platform_alloc_aligned(stack_size, stack_size, ASSUMED_CACHE_LINE_SIZE);

    int32 flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD | CLONE_SYSVSEM;

    // The + stack_size is required since the stack is used "downwards"
    thread->h = clone((int32 (*)(void*))start_routine, (void *) ((uintptr_t) thread->stack + stack_size), flags, arg);
    if (thread->h == -1) {
        LOG_1("Thread creation failed");
        return -1;
    }

    thread->id = (int32) thread->h;
    THREAD_LOG_CREATE(thread->id);

    return thread->id;
}

FORCE_INLINE
int32 coms_pthread_join(coms_pthread_t thread, void** retval) NO_EXCEPT
{
    THREAD_LOG_DELETE((int32) thread.h);

    int32 res = syscall(SYS_waitid, P_PID, thread, retval, WEXITED, NULL) == -1
        ? 1
        : 0;

    platform_aligned_free((void **) &thread.stack);
    THREAD_LOG_DELETE(thread.id);

    return res;
}

FORCE_INLINE
int32 coms_pthread_detach(MAYBE_UNUSED coms_pthread_t thread) NO_EXCEPT
{
    // In Linux, threads are automatically detached when they exit.
    return 0;
}

FORCE_INLINE
int32 coms_pthread_cond_init(mutex_cond* __restrict cond, coms_pthread_condattr_t*) NO_EXCEPT
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
int32 mutex_cond_timedwait(
    mutex_cond* __restrict cond,
    mutex* __restrict mtx,
    const struct timespec*
) NO_EXCEPT
{
    ASSERT_TRUE(cond);
    ASSERT_TRUE(mtx);

    const int32 oldval = cond->futex.load(memory_order_acquire);

    mutex_unlock(mtx);
    futex_wait(&cond->futex, oldval);
    mutex_lock(mtx);

    return 0;
}

FORCE_INLINE
int32 coms_pthread_cond_wait(
    mutex_cond* __restrict cond,
    mutex* __restrict mtx
) NO_EXCEPT
{
    return mutex_cond_timedwait(cond, mtx, NULL);
}

FORCE_INLINE
int32 coms_pthread_cond_signal(mutex_cond* cond) NO_EXCEPT
{
    ASSERT_TRUE(cond);

    cond->futex.fetch_add(1, memory_order_release);
    futex_wake(&cond->futex, 1);

    return 0;
}

FORCE_INLINE
int32 coms_pthread_cond_broadcast(mutex_cond* cond) NO_EXCEPT
{
    ASSERT_TRUE(cond);

    cond->futex.fetch_add(1, memory_order_release);
    futex_wake(&cond->futex, INT32_MAX);

    return 0;
}

FORCE_INLINE
int32 coms_pthread_rwlock_init(
    coms_pthread_rwlock_t* __restrict rwlock,
    const coms_pthread_rwlockattr_t*
) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);

    rwlock->futex.store(0, memory_order_relaxed);
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
        int32 val = rwlock->futex.load(memory_order_acquire);

        if (val >= 0) {
            int32 expected = val;
            if (rwlock->futex.compare_exchange_weak(
                    expected,
                    val + 1,
                    memory_order_acq_rel,
                    memory_order_acquire)) {
                break;
            }
        }

        futex_wait(&rwlock->futex, val);
    }

    return 0;
}

inline
int32 coms_pthread_rwlock_tryrdlock(coms_pthread_rwlock_t* rwlock) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);

    int32 val = rwlock->futex.load(memory_order_acquire);

    if (val >= 0) {
        int32 expected = val;
        if (rwlock->futex.compare_exchange_strong(
                expected,
                val + 1,
                memory_order_acq_rel,
                memory_order_acquire)) {
            return 0;
        }
    }

    return 1;
}

inline
int32 coms_pthread_rwlock_wrlock(coms_pthread_rwlock_t* rwlock) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);

    while (true) {
        int32 val = rwlock->futex.load(memory_order_acquire);

        if (val == 0) {
            int32 expected = 0;
            if (rwlock->futex.compare_exchange_weak(
                    expected,
                    -1,
                    memory_order_acq_rel,
                    memory_order_acquire)) {
                rwlock->exclusive = true;
                break;
            }
        }

        futex_wait(&rwlock->futex, val);
    }

    return 0;
}

inline
int32 coms_pthread_rwlock_trywrlock(coms_pthread_rwlock_t* rwlock) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);

    int32 expected = 0;

    if (rwlock->futex.compare_exchange_strong(
            expected,
            -1,
            memory_order_acq_rel,
            memory_order_acquire
        )
    ) {
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
        rwlock->futex.store(0, memory_order_release);
        futex_wake(&rwlock->futex, 1);
    } else {
        const int32 previous = rwlock->futex.fetch_sub(1, memory_order_release);
        if (previous == 1) {
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

#if (defined(DEBUG) && DEBUG) || (defined(INTERNAL) && INTERNAL)
    // This information is usually only needed in debug and internal builds
    #define THREAD_CURRENT_ID(a) a = thread_current_id()
    #define THREAD_CPU_ID(a) a = thread_cpu_id()
#else
    #define THREAD_CURRENT_ID(a) ((void) 0)
    #define THREAD_CPU_ID(a) ((void) 0)
#endif

#endif
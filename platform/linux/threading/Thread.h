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
#include <pthread.h>
#include <linux/futex.h>
#include <sys/syscall.h>
#include <sys/resource.h>

#include "../../../stdlib/Stdlib.h"
#include "../../../compiler/CompilerUtils.h"
#include "../Allocator.h"
#include "ThreadDefines.h"
#include "Atomic.h"
#include "../../../log/PerformanceProfiler.h"

// Only in the signature of coms_pthreads_running() (same signature as win32, the memory isn't needed on Linux)
struct BufferMemory;

// mask = bit per cpu (same as win32 SetThreadAffinityMask)
inline
void coms_thread_affinity_set(coms_pthread_t* const thread, int64 mask) NO_EXCEPT
{
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);

    for (int32 cpu = 0; cpu < 64; ++cpu) {
        if ((uint64) mask & (1ULL << cpu)) {
            CPU_SET(cpu, &cpuset);
        }
    }

    pthread_setaffinity_np(thread->h, sizeof(cpuset), &cpuset);
}

/**
 * Lowers the priority of the calling thread (background work, e.g. the thread pool workers)
 *
 * When all cores are busy the interactive threads (update, render) are scheduled first.
 * Linux: the nice value is per thread (no privileges required to increase it)
 *
 * @return void
 */
inline
void coms_thread_priority_background() NO_EXCEPT
{
    setpriority(PRIO_PROCESS, (id_t) syscall(SYS_gettid), 5);
}

// Stack size of the threads (same as the win32 default)
#define COMS_THREAD_STACK_SIZE (1 * MEGABYTE)

// The last owner (thread or handle) frees the state
static inline
void coms_thread_state_release(ComsThreadState* state) NO_EXCEPT
{
    if (state->refs.fetch_sub(1, memory_order_acq_rel) == 1) {
        platform_aligned_free((void **) &state);
    }
}

// pthread entry point: the routines have the cOMS signature (THREAD_RETURN routine(void*))
static
void* coms_pthread_start(void* data) NO_EXCEPT
{
    ComsThreadState* const state = (ComsThreadState *) data;

    // coms_pthread_create() waits for the id
    state->tid.store((int32) syscall(SYS_gettid), memory_order_release);
    futex_wake(&state->tid, 1);

    const THREAD_RETURN result = state->routine(state->arg);

    state->running.store(0, memory_order_release);
    coms_thread_state_release(state);

    return (void *) (intptr_t) result;
}

/**
 * Creates a thread
 *
 * The thread is created with pthreads, a raw clone() thread isn't a valid thread for glibc
 * (thread local storage, errno, malloc arenas, ... are only set up by pthread_create)
 *
 * @return int32 OS thread id (same as thread_current_id() inside of the thread), -1 on failure
 */
inline
int32 coms_pthread_create(coms_pthread_t* __restrict thread, void*, ThreadJobFunc start_routine, void* __restrict arg) NO_EXCEPT
{
    // @question Do we want to pin threads automatically to p cores based on a utilization score?
    ASSERT_TRUE(thread);
    ASSERT_TRUE(start_routine);

    thread->id = -1;
    thread->state = NULL;

    ComsThreadState* const state = (ComsThreadState *) platform_alloc_aligned(
        sizeof(ComsThreadState), sizeof(ComsThreadState), ASSUMED_CACHE_LINE_SIZE
    );
    if (!state) {
        LOG_1("Thread creation failed");
        return -1;
    }

    state->routine = start_routine;
    state->arg = arg;
    state->tid.store(0, memory_order_relaxed);
    state->running.store(1, memory_order_relaxed);
    state->refs.store(2, memory_order_relaxed);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, COMS_THREAD_STACK_SIZE);

    const int32 error = pthread_create(&thread->h, &attr, coms_pthread_start, state);
    pthread_attr_destroy(&attr);

    if (error) {
        ComsThreadState* unused = state;
        platform_aligned_free((void **) &unused);
        LOG_1("Thread creation failed");

        return -1;
    }

    // The OS thread id is only known inside of the thread (the thread keeps running, the state is still owned by the handle)
    int32 tid;
    while (!(tid = state->tid.load(memory_order_acquire))) {
        futex_wait(&state->tid, 0);
    }

    thread->state = state;
    thread->id = tid;
    THREAD_LOG_CREATE(thread->id);

    return thread->id;
}

FORCE_INLINE
int32 coms_pthread_join(coms_pthread_t& thread, void** retval) NO_EXCEPT
{
    void* result = NULL;
    const int32 error = pthread_join(thread.h, &result);

    if (retval) {
        *retval = result;
    }

    if (thread.state) {
        coms_thread_state_release(thread.state);
        thread.state = NULL;
    }

    THREAD_LOG_DELETE(thread.id);

    return error ? 1 : 0;
}

FORCE_INLINE
int32 coms_pthread_detach(coms_pthread_t& thread) NO_EXCEPT
{
    const int32 error = pthread_detach(thread.h);

    // The thread frees the state when it ends
    if (thread.state) {
        coms_thread_state_release(thread.state);
        thread.state = NULL;
    }

    return error ? 1 : 0;
}

FORCE_INLINE
bool coms_pthread_running(const coms_pthread_t& thread) NO_EXCEPT
{
    return thread.state && thread.state->running.load(memory_order_acquire);
}

// All threads are still running (same as win32)
FORCE_INLINE
bool coms_pthreads_running(const coms_pthread_t* threads, int count, BufferMemory*) NO_EXCEPT
{
    for (int i = 0; i < count; ++i) {
        if (!coms_pthread_running(threads[i])) {
            return false;
        }
    }

    return true;
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
    const coms_pthread_rwlockattr_t* __restrict
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

// CPU the calling thread currently runs on (same as win32 GetCurrentProcessorNumber)
FORCE_INLINE
int32 thread_cpu_id()
{
    return (int32) sched_getcpu();
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
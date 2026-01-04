/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_PLATFORM_WIN32_THREADING_THREAD_H
#define COMS_PLATFORM_WIN32_THREADING_THREAD_H

#include "../../../stdlib/Stdlib.h"
#include "../TimeUtils.h"
#include "ThreadDefines.h"
#include <windows.h>
#include "../../../log/PerformanceProfiler.h"

inline
int32 coms_pthread_create(coms_pthread_t* __restrict thread, void*, ThreadJobFunc start_routine, void* __restrict arg) NO_EXCEPT
{
    // @question Do we want to pin threads automatically to p cores based on a utilization score?
    ASSERT_TRUE(thread);
    ASSERT_TRUE(start_routine);

    thread->h = CreateThread(NULL, 0, start_routine, arg, 0, NULL);
    if (thread->h == NULL) {
        LOG_1("Thread creation failed");
        return -1;
    }

    int32 thread_id = (int32) GetThreadId(thread->h);
    THREAD_LOG_CREATE(thread_id);

    return thread_id;
}

// @bug when we close a thread we need to cleanup create_thread_profile_history(thread_id);

FORCE_INLINE
int32 coms_pthread_join(coms_pthread_t thread, void**) NO_EXCEPT
{
    THREAD_LOG_DELETE((int32) GetThreadId(thread.h));

    WaitForSingleObject(thread.h, INFINITE);
    CloseHandle(thread.h);

    return 0;
}

FORCE_INLINE
int32 coms_pthread_detach(coms_pthread_t thread) NO_EXCEPT
{
    CloseHandle(thread.h);

    return 0;
}

// WARNING: We don't support windows events since they are much slower than conditional variables/mutexes
FORCE_INLINE
int32 coms_pthread_cond_init(mutex_cond* __restrict cond, coms_pthread_condattr_t*) NO_EXCEPT
{
    ASSERT_TRUE(cond);
    InitializeConditionVariable(cond);

    return 0;
}

FORCE_INLINE
int32 coms_pthread_cond_destroy(mutex_cond*) NO_EXCEPT
{
    /* Windows does not have a destroy for conditionals */
    return 0;
}

// @question Can't we turn timespec in a typedef of uint64? I would like to avoid the time.h class
FORCE_INLINE
int32 mutex_condimedwait(mutex_cond* __restrict cond, mutex* mutex, const timespec* __restrict abstime) NO_EXCEPT
{
    ASSERT_TRUE(cond);
    ASSERT_TRUE(mutex);

    if (!SleepConditionVariableCS(cond, mutex, timespec_to_ms(abstime))) {
        return 1;
    }

    return 0;
}

FORCE_INLINE
int32 coms_pthread_cond_wait(mutex_cond* __restrict cond, mutex* __restrict mutex) NO_EXCEPT
{
    ASSERT_TRUE(cond);
    ASSERT_TRUE(mutex);

    return mutex_condimedwait(cond, mutex, NULL);
}

FORCE_INLINE
int32 coms_pthread_cond_signal(mutex_cond* cond) NO_EXCEPT
{
    ASSERT_TRUE(cond);
    WakeConditionVariable(cond);

    return 0;
}

FORCE_INLINE
int32 coms_pthread_cond_broadcast(mutex_cond* cond) NO_EXCEPT
{
    ASSERT_TRUE(cond);
    WakeAllConditionVariable(cond);

    return 0;
}

FORCE_INLINE
int32 coms_pthread_rwlock_init(coms_pthread_rwlock_t* __restrict rwlock, const coms_pthread_rwlockattr_t*) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);

    InitializeSRWLock(&rwlock->lock);
    rwlock->exclusive = false;

    return 0;
}

FORCE_INLINE
int32 coms_pthread_rwlock_destroy(coms_pthread_rwlock_t*) NO_EXCEPT
{
    return 0;
}

FORCE_INLINE
int32 coms_pthread_rwlock_rdlock(coms_pthread_rwlock_t* rwlock) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);
    AcquireSRWLockShared(&rwlock->lock);

    return 0;
}

FORCE_INLINE
int32 coms_pthread_rwlock_tryrdlock(coms_pthread_rwlock_t* rwlock) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);
    return !TryAcquireSRWLockShared(&rwlock->lock);
}

FORCE_INLINE
int32 coms_pthread_rwlock_wrlock(coms_pthread_rwlock_t* rwlock) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);

    AcquireSRWLockExclusive(&rwlock->lock);
    rwlock->exclusive = true;

    return 0;
}

FORCE_INLINE
int32 coms_pthread_rwlock_trywrlock(coms_pthread_rwlock_t* rwlock) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);

    BOOLEAN ret = TryAcquireSRWLockExclusive(&rwlock->lock);
    if (ret) {
        rwlock->exclusive = true;
    }

    return ret;
}

FORCE_INLINE
int32 coms_pthread_rwlock_unlock(coms_pthread_rwlock_t* rwlock) NO_EXCEPT
{
    ASSERT_TRUE(rwlock);

    if (rwlock->exclusive) {
        rwlock->exclusive = false;
        ReleaseSRWLockExclusive(&rwlock->lock);
    } else {
        ReleaseSRWLockShared(&rwlock->lock);
    }

    return 0;
}

FORCE_INLINE
uint32 pcthread_get_num_procs() NO_EXCEPT
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    return sysinfo.dwNumberOfProcessors;
}

#define coms_pthread_exit(a) { return (a); }

FORCE_INLINE
int32 thread_current_id()
{
    return (int32) GetCurrentThreadId();
}

FORCE_INLINE
int32 thread_cpu_id()
{
    return (int32) GetCurrentProcessorNumber();
}

#endif
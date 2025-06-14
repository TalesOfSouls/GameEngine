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

#include "../../../stdlib/Types.h"
#include "../TimeUtils.h"
#include "ThreadDefines.h"
#include <windows.h>

inline
int32 coms_pthread_create(coms_pthread_t* thread, void*, ThreadJobFunc start_routine, void* arg)
{
    if (thread == NULL || start_routine == NULL) {
        return 1;
    }

    *thread = CreateThread(NULL, 0, start_routine, arg, 0, NULL);
    if (*thread == NULL) {
        return 1;
    }

    return 0;
}

inline
int32 coms_pthread_join(coms_pthread_t thread, void**)
{
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);

    return 0;
}

inline
int32 coms_pthread_detach(coms_pthread_t thread)
{
    CloseHandle(thread);

    return 0;
}

// WARNING: We don't support windows events since they are much slower than conditional variables/mutexes
inline
int32 coms_pthread_cond_init(mutex_cond* cond, coms_pthread_condattr_t*)
{
    if (cond == NULL) {
        return 1;
    }

    InitializeConditionVariable(cond);

    return 0;
}

inline
int32 coms_pthread_cond_destroy(mutex_cond*)
{
    /* Windows does not have a destroy for conditionals */
    return 0;
}

// @question Can't we turn timespec in a typedef of uint64? I would like to avoid the time.h class
inline
int32 mutex_condimedwait(mutex_cond* cond, mutex* mutex, const timespec* abstime)
{
    if (cond == NULL || mutex == NULL) {
        return 1;
    }

    if (!SleepConditionVariableCS(cond, mutex, timespec_to_ms(abstime))) {
        return 1;
    }

    return 0;
}

inline
int32 coms_pthread_cond_wait(mutex_cond* cond, mutex* mutex)
{
    if (cond == NULL || mutex == NULL) {
        return 1;
    }

    return mutex_condimedwait(cond, mutex, NULL);
}

inline
int32 coms_pthread_cond_signal(mutex_cond* cond)
{
    if (cond == NULL) {
        return 1;
    }

    WakeConditionVariable(cond);

    return 0;
}

inline
int32 coms_pthread_cond_broadcast(mutex_cond* cond)
{
    if (cond == NULL) {
        return 1;
    }

    WakeAllConditionVariable(cond);

    return 0;
}

inline
int32 coms_pthread_rwlock_init(coms_pthread_rwlock_t* rwlock, const coms_pthread_rwlockattr_t*)
{
    if (rwlock == NULL) {
        return 1;
    }

    InitializeSRWLock(&rwlock->lock);
    rwlock->exclusive = false;

    return 0;
}

inline
int32 coms_pthread_rwlock_destroy(coms_pthread_rwlock_t*)
{
    return 0;
}

inline
int32 coms_pthread_rwlock_rdlock(coms_pthread_rwlock_t* rwlock)
{
    if (rwlock == NULL) {
        return 1;
    }

    AcquireSRWLockShared(&rwlock->lock);

    return 0;
}

inline
int32 coms_pthread_rwlock_tryrdlock(coms_pthread_rwlock_t* rwlock)
{
    if (rwlock == NULL) {
        return 1;
    }

    return !TryAcquireSRWLockShared(&rwlock->lock);
}

inline
int32 coms_pthread_rwlock_wrlock(coms_pthread_rwlock_t* rwlock)
{
    if (rwlock == NULL) {
        return 1;
    }

    AcquireSRWLockExclusive(&rwlock->lock);
    rwlock->exclusive = true;

    return 0;
}

inline
int32 coms_pthread_rwlock_trywrlock(coms_pthread_rwlock_t  *rwlock)
{
    if (rwlock == NULL) {
        return 1;
    }

    BOOLEAN ret = TryAcquireSRWLockExclusive(&rwlock->lock);
    if (ret) {
        rwlock->exclusive = true;
    }

    return ret;
}

inline
int32 coms_pthread_rwlock_unlock(coms_pthread_rwlock_t* rwlock)
{
    if (rwlock == NULL) {
        return 1;
    }

    if (rwlock->exclusive) {
        rwlock->exclusive = false;
        ReleaseSRWLockExclusive(&rwlock->lock);
    } else {
        ReleaseSRWLockShared(&rwlock->lock);
    }

    return 0;
}

inline
uint32 pcthread_get_num_procs()
{
    SYSTEM_INFO sysinfo;
    GetSystemInfo(&sysinfo);

    return sysinfo.dwNumberOfProcessors;
}

#define coms_pthread_exit(a) { return (a); }

#endif
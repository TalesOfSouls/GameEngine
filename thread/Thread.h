/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_THREADS_THREAD_H
#define COMS_THREADS_THREAD_H

#include <stdio.h>
#include "../stdlib/Types.h"
#include "../compiler/CompilerUtils.h"
#include "../log/Log.h"
#include "../log/Stats.h"
#include "Atomic.h"

#if _WIN32
    #include "../platform/win32/threading/Thread.h"
#elif __linux__
    #include "../platform/linux/threading/Thread.h"
#endif

#include "ThreadJob.h"

FORCE_INLINE
int32 thread_create(Worker* worker, ThreadJobFunc routine, void* arg) NO_EXCEPT
{
    LOG_1("[INFO] Thread starting");
    STATS_INCREMENT(DEBUG_COUNTER_THREAD);

    return coms_pthread_create(&worker->thread, NULL, routine, arg);
}

FORCE_INLINE
void thread_stop(Worker* worker) NO_EXCEPT
{
    atomic_set_release(&worker->state, 0);
    coms_pthread_join(worker->thread, NULL);

    LOG_1("[INFO] Thread ended");
    STATS_DECREMENT(DEBUG_COUNTER_THREAD);
}

#if DEBUG || INTERNAL
    // This information is usually only needed in debug and internal builds
    #define THREAD_CURRENT_ID(a) a = thread_current_id()
    #define THREAD_CPU_ID(a) a = thread_cpu_id()
#else
    #define THREAD_CURRENT_ID(a) ((void) 0)
    #define THREAD_CPU_ID(a) ((void) 0)
#endif

#endif
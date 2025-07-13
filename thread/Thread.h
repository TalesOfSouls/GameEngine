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
#include "../log/Log.h"
#include "../log/Stats.h"
#include "Atomic.h"

#if _WIN32
    #include "../platform/win32/threading/Thread.h"
#elif __linux__
    #include "../platform/linux/threading/Thread.h"
#endif

#include "ThreadJob.h"
#include "ThreadPool.h"

void thread_create(Worker* worker, ThreadJobFunc routine, void* arg)
{
    LOG_1("[INFO] Thread starting");

    coms_pthread_create(&worker->thread, NULL, routine, arg);

    LOG_INCREMENT(DEBUG_COUNTER_THREAD);
    LOG_2("[INFO] %d threads running", {{LOG_DATA_INT64, (void *) &_stats_counter[DEBUG_COUNTER_THREAD]}});
}

void thread_stop(Worker* worker)
{
    atomic_set_release(&worker->state, 0);
    coms_pthread_join(worker->thread, NULL);

    LOG_1("[INFO] Thread ended");
    LOG_DECREMENT(DEBUG_COUNTER_THREAD);
    LOG_2("[INFO] %d threads running", {{LOG_DATA_INT64, (void *) &_stats_counter[DEBUG_COUNTER_THREAD]}});
}

#endif
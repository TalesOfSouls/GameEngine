/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_THREADS_THREAD_POOL_H
#define COMS_THREADS_THREAD_POOL_H

#include "../stdlib/Stdlib.h"
#include "../memory/PersistentQueueT.h"
#include "../memory/BufferMemory.h"
#include "../log/DebugMemory.h"
#include "../log/PerformanceProfiler.h"
#include "../log/DebugContainer.h"
#include "Thread.h"
#include "Atomic.h"
#include "ThreadJob.h"
#include "../utils/RandomUtils.h"

enum ThreadPoolState : int16 {
    THREAD_POOL_STATE_CANCELING = -1,
    THREAD_POOL_STATE_COMPLETED = 0,
    THREAD_POOL_STATE_WAITING = 1,
    THREAD_POOL_STATE_RUNNING = 2,

    // The job itself may pause for whatever reason (e.g. wait for elements in a threaded queue)
    THREAD_POOL_STATE_PAUSED = 3,
};

struct ThreadPool {
    // This is not a threaded queue since we want to handle the mutex in here, not in the queue for finer control
    PersistentQueueT<PoolWorker> work_queue;

    // @performance Could it make more sense to use a spinlock for the thread pool?
    // Is probably overkill if we cannot really utilize it a lot -> we would have a lot of spinning
    mutex work_mutex;
    mutex_cond work_cond;
    mutex_cond working_cond;

    // By design the working_cnt is <= thread_cnt
    atomic_32 int32 working_cnt;
    atomic_32 int32 thread_cnt;

    // This is where we store the handles IFF we are using
    // none-detached threads
    coms_pthread_t* thread_handles;

    // @question what is the difference between thread_cnt and size?
    int32 size;

    // @question Why are we using atomics if we are using mutex at the same time?
    //          I think because the mutex is actually intended for the queue.
    ThreadPoolState state;
    bool is_detached;
    atomic_32 uint32 id_counter;

    DebugContainer* debug_container;
};

#endif
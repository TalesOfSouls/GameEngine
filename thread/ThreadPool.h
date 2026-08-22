/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_THREADS_THREAD_POOL_H
#define COMS_THREADS_THREAD_POOL_H

#include "../stdlib/Stdlib.h"
#include "../memory/MPMCWorkTrackingQueueT.h"
#include "../memory/ThrdChunkMemory.h"
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
    // @performance Which variables need padding to avoid false sharing?
    atomic<ThreadPoolState> state;
    // By design the working_cnt is <= thread_cnt
    atomic<int32> working_cnt;
    atomic<int32> thread_cnt;
    atomic<uint32> id_counter;

    // How many worker threads are currently parked/asleep
    // Producers only take work_mutex/signal work_cond when this is > 0, so
    // enqueueing work under load (workers already busy, none sleeping) never
    // uses the mutex at all
    atomic<int32> sleeping_cnt;

    MPMCWorkTrackingQueueT<PoolWorker> work_queue;

    sem work_sem;

    // This is where we store the handles IFF we are using
    // none-detached threads
    coms_pthread_t* thread_handles;

    // thread_cnt should be the same as size UNLESS a thread shut down for whatever reason
    // We use the size to check the "healthyness" of the pool and may spin up new worker threads
    // if size != thread_cnt
    int32 size;

    bool is_detached;

    DebugContainer* debug_container;

    // From this memory we distribute memory to all the worker threads
    // The distributed memory depends on the requirements of the respective job
    // This means we redistribute memory on a job basis
    ThrdChunkMemory thrd_mem;
};

#endif
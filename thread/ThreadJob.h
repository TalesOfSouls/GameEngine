/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_THREADS_JOB_H
#define COMS_THREADS_JOB_H

#include <stdio.h>
#include <stdlib.h>

#include "../stdlib/Stdlib.h"
#include "../memory/ThreadedRingMemory.h"
#include "../thread/ThreadDefines.h"

typedef void (*ThreadPoolJobFunc)(void*);

enum PoolWorkerState : int32 {
    POOL_WORKER_STATE_CANCEL = -1,
    POOL_WORKER_STATE_COMPLETED = 0,
    POOL_WORKER_STATE_WAITING = 1,
    POOL_WORKER_STATE_RUNNING = 2
};

/**
 * Worker for the thread pool
 */
struct PoolWorker {
    // @performance We could reduce the size of id and state down to u16
    //              However, that wouldn't change the size of the struct due to alignment
    //              Maybe this will become useful later if we add more members to the struct
    atomic_32 uint32 id;
    atomic_32 PoolWorkerState state;

    ThreadPoolJobFunc func;

    // Callback for when the job completes
    ThreadPoolJobFunc callback;

    // If we have different arg data you must use a wrapper struct that can hold the other data
    // The queue allows to store fixed data larger than PoolWorker by providing the the element size
    // This means you could use sizeof(PoolWorker) + 128, make arg point to job + 1
    // thread_pool_add_work() automatically adds sizeof(PoolWorker) + 128 to the queue
    void* arg;

    // This can be used either to describe the actual size if arg is a string/byte array,
    // or we can use it to describe the array length if arg is an array
    int32 arg_size;

    // Pointer to memory to be used by the thread worker
    void* mem;
};

/**
 * Worker for a normal thread
 */
struct Worker {
    atomic_32 int32 state;
    coms_pthread_t thread;
    void* arg;
};

#endif
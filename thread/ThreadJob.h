/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_THREADS_JOB_H
#define COMS_THREADS_JOB_H

#include "../stdlib/Stdlib.h"
#include "../thread/ThreadDefines.h"
#include "../thread/Spinlock.h"

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
    //              Due to alignments this wouldn't have any effect currently
    atomic<uint32> id;
    atomic<PoolWorkerState> state;

    // After running the task it is automatically removed from the thread queue memory
    // If this is false the user MUST do this manually by calling the _release() function
    bool automatic_release;

    // This can be used either to describe the actual size if arg is a string/byte array,
    // or we can use it to describe the array length if arg is an array
    // How to interpret arg_size is handled in the function itself
    int32 arg_size;

    void* arg;

    ThreadPoolJobFunc func;

    // Callback for when the job completes
    ThreadPoolJobFunc callback;

    // Pointer to memory to be used by the thread worker
    size_t mem_size;
    byte* mem;
};

/**
 * Worker for a normal thread
 */
struct ThreadWorker {
    atomic<int32> state;
    coms_pthread_t thread;
    void* arg;
};

struct ThreadParameter {
    ThreadWorker worker;
    ThreadJobFunc routine;
    void* arg;
};

#endif
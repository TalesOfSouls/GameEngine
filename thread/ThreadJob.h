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

enum PoolWorkerState : int8 {
    POOL_WORKER_STATE_CANCEL = -1,
    POOL_WORKER_STATE_COMPLETED = 0,
    POOL_WORKER_STATE_WAITING = 1,
    POOL_WORKER_STATE_RUNNING = 2
};

/**
 * Worker for the thread pool
 */
struct PoolWorker {
    uint64 id;

    // This is up here instead of down where void* mem is defined to reduce the alignment overhead
    uint32 mem_size;

    // This can be used either to describe the actual size if arg is a string/byte array,
    // or we can use it to describe the array length if arg is an array
    // How to interpret arg_size is handled in the function itself
    int32 arg_size;
    void* arg;

    ThreadPoolJobFunc func;

    // Callback for when the job completes
    ThreadPoolJobFunc callback;

    // Completion target for the callback (e.g. a result queue or a simple int array etc.)
    // @optional
    void* sink;

    // External state to be updated
    // @optional
    atomic<PoolWorkerState>* state_external;

    // Pointer to memory to be used by the thread worker
    void* mem;
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
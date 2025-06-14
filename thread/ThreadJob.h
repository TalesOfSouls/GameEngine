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

#include "../stdlib/Types.h"
#include "../memory/ThreadedRingMemory.h"
#include "../thread/ThreadDefines.h"

typedef void (*ThreadPoolJobFunc)(void*);

enum PoolWorkerState : int32 {
    POOL_WORKER_STATE_CANCEL = -1,
    POOL_WORKER_STATE_COMPLETED = 0,
    POOL_WORKER_STATE_WAITING = 1,
    POOL_WORKER_STATE_RUNNING = 2
};

// @performance Could we reduce the size of PoolWorker by reducing atomic_32 to atomic_16?
//  I don't think so because ThreadPoolJobFunc should be 8 bytes
struct PoolWorker {
    alignas(4) atomic_32 int32 id;
    alignas(4) atomic_32 PoolWorkerState state;
    ThreadPoolJobFunc func;
    ThreadPoolJobFunc callback;
};

struct Worker {
    alignas(4) atomic_32 int32 state;
    coms_pthread_t thread;
    void* arg;
};

#endif
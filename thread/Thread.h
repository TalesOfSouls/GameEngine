/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_THREADS_THREAD_H
#define COMS_THREADS_THREAD_H

#include "../stdlib/Stdlib.h"
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
int32 thread_create(ThreadWorker* worker, ThreadJobFunc routine, void* arg) NO_EXCEPT
{
    LOG_1("[INFO] Thread starting");
    STATS_INCREMENT_DEBUG(DEBUG_COUNTER_THREAD);

    return coms_pthread_create(&worker->thread, NULL, routine, arg);
}

FORCE_INLINE
void thread_stop(ThreadWorker* worker) NO_EXCEPT
{
    worker->state.store(0);
    coms_pthread_join(worker->thread, NULL);

    LOG_1("[INFO] Thread ended");
    STATS_DECREMENT_DEBUG(DEBUG_COUNTER_THREAD);
}

#define TB_INDEX_MASK 0x3u
#define TB_NEW_FLAG 0x4u

struct TripleBufferIndex {
    alignas(ASSUMED_CACHE_LINE_SIZE) atomic<uint32> middle;
    alignas(ASSUMED_CACHE_LINE_SIZE) uint32 write;
    alignas(ASSUMED_CACHE_LINE_SIZE) uint32 read;
};

void triple_buffer_init(TripleBufferIndex* const buffer) NO_EXCEPT
{
    buffer->write = 0;
    buffer->middle.store(1);
    buffer->read = 2;
}

void triple_buffer_publish(TripleBufferIndex* const buffer) NO_EXCEPT
{
    uint32 old = buffer->middle.exchange(
        buffer->write | TB_NEW_FLAG, memory_order_acq_rel
    );

    buffer->write = old & TB_INDEX_MASK;
}

// This returns either a new index if one is available or the previous one
uint32 triple_buffer_acquire(TripleBufferIndex* const buffer) NO_EXCEPT
{
    if (buffer->middle.load(memory_order_relaxed) & TB_NEW_FLAG) {
        uint32 old = buffer->middle.exchange(
            buffer->read, memory_order_acq_rel
        );
        buffer->read = old & TB_INDEX_MASK;
    }

    return buffer->read;
}

// is a new index available?
FORCE_INLINE
bool triple_buffer_available(const TripleBufferIndex* const buffer) NO_EXCEPT
{
    return buffer->middle.load(memory_order_relaxed) & TB_NEW_FLAG;
}

#if (defined(DEBUG) && DEBUG) || (defined(INTERNAL) && INTERNAL)
    // This information is usually only needed in debug and internal builds
    #define THREAD_CURRENT_ID(a) a = thread_current_id()
    #define THREAD_CPU_ID(a) a = thread_cpu_id()
#else
    #define THREAD_CURRENT_ID(a) ((void) 0)
    #define THREAD_CPU_ID(a) ((void) 0)
#endif

#endif
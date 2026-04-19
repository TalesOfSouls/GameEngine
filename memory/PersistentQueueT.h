/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_QUEUET_PERSISTENT_H
#define COMS_MEMORY_QUEUET_PERSISTENT_H

#include "../stdlib/Stdlib.h"
#include "../utils/BitUtils.h"
#include "../log/Log.h"
#include "../log/Stats.h"
#include "../log/PerformanceProfiler.h"
#include "../log/DebugMemory.h"
#include "../thread/ThreadDefines.h"
#include "../system/Allocator.h"
#include "MemoryArena.h"
#include "BufferMemory.h"
#include "ChunkMemory.h"
#include "../thread/Thread.h"
#include "../thread/Semaphore.h"

/**
 * This storage system is a combination of a Queue and ChunkMemory
 * We sometimes need to hold onto elements in a queue
 * For that case we need two flags
 *      1. describe which slot has free memory
 *      2. which slot already got dequeued/completed but remains in the queue
 */
template <typename T>
struct PersistentQueueT {
    T* memory;
    atomic_32 uint32 head;
    atomic_32 uint32 tail;

    uint32 capacity;

    // free describes which locations are used and which are free
    atomic_ptr uint_max* free;

    // completed describes which elements are already dequeued but are kept in the queue indefinitely
    atomic_ptr uint_max* completed;

    // We support both conditional locking and semaphore locking
    // These values are not initialized and not used unless you use the queue
    mutex lock;
    mutex_cond cond;

    sem empty;
    sem full;
};

#endif
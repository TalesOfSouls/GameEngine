/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_QUEUE_T_H
#define COMS_MEMORY_QUEUE_T_H

#include "../stdlib/Stdlib.h"
#include "../system/Allocator.h"
#include "BufferMemory.h"
#include "MemoryArena.h"
#include "../thread/Thread.h"
#include "../thread/Semaphore.h"

/**
 * This queue implementation can be used single threaded or multi threaded
 * The programmer is responsible for calling the appropriate functions.
 * This also goes for SPSC vs. SPMC, MPMC, ...
 * This of course puts more mental load on the programmer but makes this queue very powerful
 */
template <typename T>
struct QueueT {
    T* memory;
    atomic_ptr T* head;
    atomic_ptr T* tail;

    int capacity;

    // We support both conditional locking and semaphore locking
    // These values are not initialized and not used unless you use the queue
    mutex mtx;
    mutex_cond cond;

    sem empty;
    sem full;
};

#endif
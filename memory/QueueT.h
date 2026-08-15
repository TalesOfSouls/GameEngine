/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_QUEUE_T_H
#define COMS_MEMORY_QUEUE_T_H

#include "../stdlib/Stdlib.h"
#include "../thread/ThreadDefines.h"
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
    T* head;
    T* tail;

    int capacity;

    // We support both conditional locking and semaphore locking
    // These values are not initialized and not used unless you use the queue
    alignas(ASSUMED_CACHE_LINE_SIZE) mutex mtx;
    mutex_cond cond;

    sem empty;
    sem full;

    // We can't compile the actual required size at compile time because compilers/windows suck!
    char _pad[ASSUMED_CACHE_LINE_SIZE];
};

#endif
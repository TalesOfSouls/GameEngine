/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_MPSCQUEUE_T_H
#define COMS_MEMORY_MPSCQUEUE_T_H

#include "../stdlib/Stdlib.h"
#include "../thread/Atomic.h"

template <typename T>
struct MPSCQueueT {
    T* memory;
    int capacity;

    // one flag per slot: 0 = not ready, 1 = ready
    alignas(ASSUMED_CACHE_LINE_SIZE) atomic<byte>* slot_ready;

    // Contended across all producer threads.
    alignas(ASSUMED_CACHE_LINE_SIZE) atomic<T*> head;

    // Written only by the consumer thread; read atomically by producers
    // to know whether a slot is free.
    alignas(ASSUMED_CACHE_LINE_SIZE) atomic<T*> tail;
    T* head_cache;
    char _pad[ASSUMED_CACHE_LINE_SIZE - sizeof(atomic<T*>) - sizeof(T*)];
};

#endif
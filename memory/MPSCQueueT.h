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
    alignas(ASSUMED_CACHE_LINE_SIZE) atomic<T*> head;
    alignas(ASSUMED_CACHE_LINE_SIZE) T* tail;
    int capacity;

    // Construction tracking - one per slot
    // 0 = empty/writing, 1 = ready
    atomic<uint32>* slot_ready;
};

#endif
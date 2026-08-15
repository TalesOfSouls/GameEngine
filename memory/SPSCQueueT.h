/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_SPSCQUEUE_T_H
#define COMS_MEMORY_SPSCQUEUE_T_H

#include "../stdlib/Stdlib.h"
#include "../thread/Atomic.h"

template <typename T>
struct SPSCQueueT {
    T* memory;
    int capacity;

    alignas(ASSUMED_CACHE_LINE_SIZE) atomic<T*> head;
    T* tail_cache;

    alignas(ASSUMED_CACHE_LINE_SIZE) atomic<T*> tail;
    T* head_cache;
    char _pad[ASSUMED_CACHE_LINE_SIZE - sizeof(atomic<T*>) - sizeof(T*)];
};

#endif
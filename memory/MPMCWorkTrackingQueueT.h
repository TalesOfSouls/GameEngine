/**
 * Sometimes we need a queue where the element remains in the queue even after
 * dequeuing it. This is helpful to avoid copying or allocating new memory.
 *
 * Examples for such situations are thread pools, task schedulers, etc.
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_MPMC_WORK_TRACKING_QUEUE_H
#define COMS_MEMORY_MPMC_WORK_TRACKING_QUEUE_H

#include "../stdlib/Stdlib.h"
#include "../thread/Atomic.h"

// @todo Implement a function that checks if a slot has been handled but is simply not deleted yet
template<typename T>
struct Slot {
    // This is at the beginning to improve loading performance
    // If we put it after memory, we may have to load additional cache lines
    atomic<size_t> turn;
    T memory;
};

template<typename T>
struct MPMCWorkTrackingQueueT {
    Slot<T>* memory;
    int capacity;
    size_t capacity_mask;

    alignas(ASSUMED_CACHE_LINE_SIZE) atomic<size_t> head;
    alignas(ASSUMED_CACHE_LINE_SIZE) atomic<size_t> tail;
};

#endif
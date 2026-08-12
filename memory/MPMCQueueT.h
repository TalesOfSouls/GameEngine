/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_MPMCQUEUE_T_H
#define COMS_MEMORY_MPMCQUEUE_T_H

#include "../stdlib/Stdlib.h"
#include "../thread/Atomic.h"

template <typename T>
struct MPMCQueueT {
    T* memory;
    int capacity;

    // Sequence numbers for each slot - one extra bit indicates if slot contains data
    // Sequence scheme: seq % (2*capacity) < capacity means slot is available for writing
    //                  seq % (2*capacity) >= capacity means slot has data for reading
    atomic<uint32>* sequences;

    alignas(ASSUMED_CACHE_LINE_SIZE) atomic<uint32> enqueue_pos;
    alignas(ASSUMED_CACHE_LINE_SIZE) atomic<uint32> dequeue_pos;
};

#endif
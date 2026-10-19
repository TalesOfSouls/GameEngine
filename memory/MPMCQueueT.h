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

    // WARNING: Must be power of 2
    int capacity;
    int mask;

    // The sequence number helps us to handle multiple producers/consumers and flag
    // when a slot is empty/used. For the position pos that maps to slot i:
    //      sequence[i] == pos       slot is empty, a producer may claim it
    //      sequence[i] == pos + 1   slot is full, a consumer may claim it
    // Dequeueing sets it to pos + capacity (= empty for the next lap)
    atomic<size_t>* sequence;

    // Contended across all producer threads
    alignas(ASSUMED_CACHE_LINE_SIZE) atomic<size_t> head;

    // Contended across all consumer threads
    alignas(ASSUMED_CACHE_LINE_SIZE) atomic<size_t> tail;
    char _pad[ASSUMED_CACHE_LINE_SIZE - sizeof(atomic<size_t>)];
};

#endif
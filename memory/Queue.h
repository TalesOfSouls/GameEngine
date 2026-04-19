/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_QUEUE_H
#define COMS_MEMORY_QUEUE_H

#include "../stdlib/Stdlib.h"
#include "../utils/Utils.h"
#include "../thread/Atomic.h"
#include "RingMemory.cpp"
#include "BufferMemory.h"

// WARNING: Structure needs to be the same as RingMemory
struct Queue {
    byte* memory;
    byte* end;

    atomic_ptr byte* head;

    // This variable is usually only used by single producer/consumer code mostly found in threads.
    // One thread inserts elements -> updates head
    // The other thread reads elements -> updates tail
    // This code itself doesn't change this variable
    atomic_ptr byte* tail;

    size_t size;
    uint32 alignment;

    // The ring memory ends here
    uint32 element_size;

    // We support both conditional locking and semaphore locking
    // These values are not initialized and not used unless you use the queue
    mutex mtx;
    mutex_cond cond;

    sem empty;
    sem full;
};

typedef void* (*QueueFunction)(void* data);

// General purpose event for event queues
struct QueueEvent {
    int16 type;
    QueueFunction callback;
    byte data[256];
};

#endif
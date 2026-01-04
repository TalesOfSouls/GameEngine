/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MEMORY_QUEUE_H
#define COMS_MEMORY_QUEUE_H

#include "../stdlib/Stdlib.h"
#include "../utils/Utils.h"
#include "RingMemory.h"

// WARNING: Structure needs to be the same as RingMemory
struct Queue {
    byte* memory;
    byte* end;

    byte* head;

    // This variable is usually only used by single producer/consumer code mostly found in threads.
    // One thread inserts elements -> updates head
    // The other thread reads elements -> updates tail
    // This code itself doesn't change this variable
    byte* tail;

    uint64 size;
    uint32 alignment;

    // The ring memory ends here
    uint32 element_size;
};

typedef void* (*QueueFunction)(void* data);

// General purpose event for event queues
struct QueueEvent {
    int16 type;
    QueueFunction callback;
    byte data[256];
};

FORCE_INLINE
void queue_alloc(Queue* const queue, uint64 element_count, uint32 element_size, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    element_size = align_up(element_size, alignment);

    ring_alloc((RingMemory *) queue, element_count * element_size, alignment);
    queue->element_size = element_size;
}

FORCE_INLINE
void queue_init(Queue* const queue, BufferMemory* const buf, uint64 element_count, uint32 element_size, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    element_size = align_up(element_size, alignment);

    ring_init((RingMemory *) queue, buf, element_count * element_size, alignment);
    queue->element_size = element_size;
}

FORCE_INLINE
void queue_init(Queue* const queue, byte* buf, uint64 element_count, uint32 element_size, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    element_size = align_up(element_size, alignment);

    ring_init((RingMemory *) queue, buf, element_count * element_size, alignment);
    queue->element_size = element_size;
}

FORCE_INLINE
void queue_free(Queue* const queue) NO_EXCEPT
{
    ring_free((RingMemory *) queue);
}

FORCE_INLINE
bool queue_is_empty(const Queue* const queue) NO_EXCEPT
{
    return queue->head == queue->tail;
}

FORCE_INLINE
void queue_set_empty(Queue* const queue) NO_EXCEPT
{
    queue->head = queue->tail;
}

FORCE_INLINE
bool queue_is_full(Queue* const queue) NO_EXCEPT
{
    return !ring_commit_safe((RingMemory *) queue, queue->element_size, queue->alignment);
}

inline
void queue_enqueue_unique(Queue* const queue, const byte* data) NO_EXCEPT
{
    ASSERT_TRUE((uint64_t) data % 4 == 0);

    byte* tail = queue->tail;
    while (tail != queue->tail) {
        ASSERT_TRUE((uint64_t) tail % 4 == 0);

        // @performance we could probably make this faster since we don't need to compare the entire range
        if (is_equal(tail, data, queue->element_size) == 0) {
            return;
        }

        ring_move_pointer((RingMemory *) queue, &tail, queue->element_size, queue->alignment);
    }

    if (!ring_commit_safe((RingMemory *) queue, queue->element_size, queue->alignment)) {
        return;
    }

    byte* mem = ring_get_memory((RingMemory *) queue, queue->element_size, queue->alignment);
    memcpy(mem, data, queue->element_size);
}

inline
byte* queue_enqueue(Queue* const queue, const byte* data) NO_EXCEPT
{
    byte* mem = ring_get_memory_nomove((RingMemory *) queue, queue->element_size, queue->alignment);
    memcpy(mem, data, queue->element_size);
    ring_move_pointer((RingMemory *) queue, &queue->head, queue->element_size, queue->alignment);

    return mem;
}

inline
byte* queue_enqueue_safe(Queue* const queue, const byte* data) NO_EXCEPT
{
    if(queue_is_full(queue)) {
        return NULL;
    }

    byte* mem = ring_get_memory_nomove((RingMemory *) queue, queue->element_size, queue->alignment);
    memcpy(mem, data, queue->element_size);
    ring_move_pointer((RingMemory *) queue, &queue->head, queue->element_size, queue->alignment);

    return mem;
}

// WARNING: Only useful for single producer single consumer
inline
byte* queue_enqueue_wait_atomic(Queue* const queue, const byte* data) NO_EXCEPT
{
    while (!ring_commit_safe_atomic((RingMemory *) queue, queue->alignment)) {}

    byte* mem = ring_get_memory_nomove((RingMemory *) queue, queue->element_size, queue->alignment);
    memcpy(mem, data, queue->element_size);
    ring_move_pointer((RingMemory *) queue, &queue->head, queue->element_size, queue->alignment);

    return mem;
}

// WARNING: Only useful for single producer single consumer
inline
byte* queue_enqueue_safe_atomic(Queue* const queue, const byte* data) NO_EXCEPT
{
    if (!ring_commit_safe_atomic((RingMemory *) queue, queue->alignment)) {
        return NULL;
    }

    byte* mem = ring_get_memory_nomove((RingMemory *) queue, queue->element_size, queue->alignment);
    memcpy(mem, data, queue->element_size);
    ring_move_pointer((RingMemory *) queue, &queue->head, queue->element_size, queue->alignment);

    return mem;
}

FORCE_INLINE
byte* queue_enqueue_start(Queue* const queue) NO_EXCEPT
{
    return ring_get_memory_nomove((RingMemory *) queue, queue->element_size, queue->alignment);
}

FORCE_INLINE
void queue_enqueue_end(Queue* const queue) NO_EXCEPT
{
    ring_move_pointer((RingMemory *) queue, &queue->head, queue->element_size, queue->alignment);
}

inline
bool queue_dequeue(Queue* const queue, byte* data) NO_EXCEPT
{
    if (queue->head == queue->tail) {
        return false;
    }

    if (queue->element_size == 4) {
        *((int32 *) data) = *((int32 *) queue->tail);
    } else {
        memcpy(data, queue->tail, queue->element_size);
    }

    ring_move_pointer((RingMemory *) queue, &queue->tail, queue->element_size, queue->alignment);

    return true;
}

// WARNING: Only useful for single producer single consumer
inline
bool queue_dequeue_atomic(Queue* const queue, byte* data) NO_EXCEPT
{
    if ((uint64) atomic_get_acquire_release((void **) &queue->head) == (uint64) queue->tail) {
        return false;
    }

    if (queue->element_size == 4) {
        *((int32 *) data) = *((int32 *) queue->tail);
    } else {
        memcpy(data, queue->tail, queue->element_size);
    }

    ring_move_pointer((RingMemory *) queue, &queue->tail, queue->element_size, queue->alignment);

    return true;
}

inline
byte* queue_dequeue_keep(Queue* const queue) NO_EXCEPT
{
    if (queue->head == queue->tail) {
        return NULL;
    }

    byte* data = queue->tail;
    ring_move_pointer((RingMemory *) queue, &queue->tail, queue->element_size, queue->alignment);

    return data;
}

FORCE_INLINE
byte* queue_dequeue_start(const Queue* const queue) NO_EXCEPT
{
    return queue->tail;
}

FORCE_INLINE
void queue_dequeue_end(Queue* const queue) NO_EXCEPT
{
    ring_move_pointer((RingMemory *) queue, &queue->tail, queue->element_size, queue->alignment);
}

#endif
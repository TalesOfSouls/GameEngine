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

#include "../stdlib/Types.h"
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

inline
void queue_alloc(Queue* queue, uint64 element_count, uint32 element_size, uint32 alignment = 64)
{
    element_size = ROUND_TO_NEAREST(element_size, alignment);

    ring_alloc((RingMemory *) queue, element_count * element_size, alignment);
    queue->element_size = element_size;
}

inline
void queue_init(Queue* queue, BufferMemory* buf, uint64 element_count, uint32 element_size, uint32 alignment = 64)
{
    element_size = ROUND_TO_NEAREST(element_size, alignment);

    ring_init((RingMemory *) queue, buf, element_count * element_size, alignment);
    queue->element_size = element_size;
}

inline
void queue_init(Queue* queue, byte* buf, uint64 element_count, uint32 element_size, uint32 alignment = 64)
{
    element_size = ROUND_TO_NEAREST(element_size, alignment);

    ring_init((RingMemory *) queue, buf, element_count * element_size, alignment);
    queue->element_size = element_size;
}

inline
void queue_free(Queue* queue) noexcept
{
    ring_free((RingMemory *) queue);
}

inline
bool queue_is_empty(const Queue* queue) noexcept {
    return queue->head == queue->tail;
}

inline
void queue_set_empty(Queue* queue) noexcept {
    queue->head = queue->tail;
}

inline
bool queue_is_full(Queue* queue) noexcept {
    return !ring_commit_safe((RingMemory *) queue, queue->element_size, queue->alignment);
}

inline
void queue_enqueue_unique(Queue* queue, const byte* data) noexcept
{
    ASSERT_SIMPLE((uint64_t) data % 4 == 0);

    byte* tail = queue->tail;
    while (tail != queue->tail) {
        ASSERT_SIMPLE((uint64_t) tail % 4 == 0);

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
byte* queue_enqueue(Queue* queue, const byte* data) noexcept
{
    byte* mem = ring_get_memory_nomove((RingMemory *) queue, queue->element_size, queue->alignment);
    memcpy(mem, data, queue->element_size);
    ring_move_pointer((RingMemory *) queue, &queue->head, queue->element_size, queue->alignment);

    return mem;
}

inline
byte* queue_enqueue_safe(Queue* queue, const byte* data) noexcept
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
byte* queue_enqueue_wait_atomic(Queue* queue, const byte* data) noexcept
{
    while (!ring_commit_safe_atomic((RingMemory *) queue, queue->alignment)) {}

    byte* mem = ring_get_memory_nomove((RingMemory *) queue, queue->element_size, queue->alignment);
    memcpy(mem, data, queue->element_size);
    ring_move_pointer((RingMemory *) queue, &queue->head, queue->element_size, queue->alignment);

    return mem;
}

// WARNING: Only useful for single producer single consumer
inline
byte* queue_enqueue_safe_atomic(Queue* queue, const byte* data) noexcept
{
    if (!ring_commit_safe_atomic((RingMemory *) queue, queue->alignment)) {
        return NULL;
    }

    byte* mem = ring_get_memory_nomove((RingMemory *) queue, queue->element_size, queue->alignment);
    memcpy(mem, data, queue->element_size);
    ring_move_pointer((RingMemory *) queue, &queue->head, queue->element_size, queue->alignment);

    return mem;
}

inline
byte* queue_enqueue_start(Queue* queue) noexcept
{
    return ring_get_memory_nomove((RingMemory *) queue, queue->element_size, queue->alignment);
}

inline
void queue_enqueue_end(Queue* queue) noexcept
{
    ring_move_pointer((RingMemory *) queue, &queue->head, queue->element_size, queue->alignment);
}

inline
bool queue_dequeue(Queue* queue, byte* data) noexcept
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
bool queue_dequeue_atomic(Queue* queue, byte* data) noexcept
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
byte* queue_dequeue_keep(Queue* queue) noexcept
{
    if (queue->head == queue->tail) {
        return NULL;
    }

    byte* data = queue->tail;
    ring_move_pointer((RingMemory *) queue, &queue->tail, queue->element_size, queue->alignment);

    return data;
}

inline
byte* queue_dequeue_start(const Queue* queue) noexcept
{
    return queue->tail;
}

inline
void queue_dequeue_end(Queue* queue) noexcept
{
    ring_move_pointer((RingMemory *) queue, &queue->tail, queue->element_size, queue->alignment);
}

#endif
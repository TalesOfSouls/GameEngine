/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_MPMC_WORK_TRACKING_QUEUE_C
#define COMS_MEMORY_MPMC_WORK_TRACKING_QUEUE_C

#include "MPMCWorkTrackingQueueT.h"

// Get next power of 2
FORCE_INLINE CONSTEXPR
uint32 queue_next_pow2(uint32 n) NO_EXCEPT
{
    if (n <= 1) {
        return 1;
    }

    --n;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;

    return n + 1;
}

FORCE_INLINE CONSTEXPR
size_t queue_size(size_t element_size, int32 capacity) NO_EXCEPT
{
    return element_size * queue_next_pow2((uint32) capacity);
}

template<typename T>
void queue_init(MPMCWorkTrackingQueueT<T>* queue, void* buf, int32 capacity, int32 alignment = alignof(size_t)) NO_EXCEPT
{
    queue->capacity = queue_next_pow2((uint32) capacity);
    queue->capacity_mask = queue->capacity - 1;
    queue->memory = (Slot<T> *) align_up((uintptr_t) buf, (size_t) alignment);

    // Initial turn == index means writable/cycle 0
    for (int i = 0; i < queue->capacity; ++i) {
        queue->memory[i].turn.store(i);
    }

    queue->head.store(0);
    queue->tail.store(0);
}

template<typename T>
void queue_alloc(MPMCWorkTrackingQueueT<T>* queue, int32 capacity, int32 alignment) NO_EXCEPT
{
    const size_t size = queue_size(sizeof(T), capacity);
    byte* buf = (byte *) platform_alloc_aligned(size, size, alignment);
    queue_init(queue, buf, capacity, alignment);
}

template<typename T>
T* queue_enqueue_start(MPMCWorkTrackingQueueT<T>* queue) NO_EXCEPT
{
    uint64 pos = queue->head.load();

    for (uint32 spin = 0;; ++spin) {
        Slot<T>* const slot = &queue->memory[pos & queue->capacity_mask];
        const uint64 turn = slot->turn.load();
        const int64 diff = (int64) turn - (int64) pos;

        if (diff == 0) {
            if (queue->head.compare_exchange_strong(pos, pos + 1)) {
                return &slot->memory;
            }

            // CAS failed, pos was refreshed to the current value by compare_exchange_strong,
            // loop again immediately
        } else if (diff < 0) {
            // The slot is still in use by an element nobody has released yet
            // -> queue is full
            return NULL;
        } else {
            pos = queue->head.load();
        }

        if (spin > 16) {
            YieldProcessor();
        }
    }
}

template<typename T>
void queue_enqueue_end(T* element) NO_EXCEPT
{
    Slot<T>* const slot = (Slot<T> *) ((byte *) element - offsetof(Slot<T>, memory));

    // Turn still holds "pos" (nothing has touched it since queue_enqueue_start() reserved the slot)
    // -> publish by moving it to pos + 1 (= it is ready).
    ++slot->turn;
}

template<typename T>
T* queue_enqueue(MPMCWorkTrackingQueueT<T>* queue, const T* element) NO_EXCEPT
{
    T* const slot = queue_enqueue_start(queue);
    if (!slot) {
        return NULL;
    }

    memcpy(slot, element, sizeof(T));
    queue_enqueue_end(queue, slot);

    return slot;
}

template<typename T>
T* queue_enqueue(MPMCWorkTrackingQueueT<T>* queue, const T& element) NO_EXCEPT
{
    T* const slot = queue_enqueue_start(queue);
    if (!slot) {
        return NULL;
    }

    memcpy(slot, element, sizeof(T));
    queue_enqueue_end(queue, slot);

    return slot;
}

// Dequeue a queue element but keep it in memory
template<typename T>
T* queue_dequeue_keep(MPMCWorkTrackingQueueT<T>* queue) NO_EXCEPT
{
    uint64 pos = queue->tail.load();

    for (uint32 spin = 0;; ++spin) {
        Slot<T>* const slot = &queue->memory[pos & queue->capacity_mask];
        const uint64 turn = slot->turn.load();
        const int64 diff = (int64) turn - (int64) (pos + 1);

        if (diff == 0) {
            if (queue->tail.compare_exchange_strong(pos, pos + 1)) {
                // Ownership of this slot is exclusively ours
                // The turn is only modified on release
                return &slot->memory;
            }
        } else if (diff < 0) {
            // Nothing published at this slot yet
            // -> empty (from this consumer's point of view).
            return NULL;
        } else {
            pos = queue->tail.load();
        }

        if (spin > 16) {
            YieldProcessor();
        }
    }
}

// Release an already dequeued element
template<typename T>
void queue_dequeue_release(MPMCWorkTrackingQueueT<T>* queue, const T* element) NO_EXCEPT
{
    Slot<T>* const slot = (Slot<T> *) ((byte *) element - offsetof(Slot<T>, memory));

    const uint64 pos_plus_one = slot->turn.load();
    slot->turn.store(pos_plus_one + queue->capacity - 1);
}

template<typename T>
bool queue_is_empty(const MPMCWorkTrackingQueueT<T>* queue) NO_EXCEPT
{
    const uint64 pos = queue->tail.load();
    const Slot<T>* const slot = &queue->memory[pos & queue->capacity_mask];
    const uint64 turn = slot->turn.load();

    return ((int64) turn - (int64) (pos + 1)) < 0;
}

#endif
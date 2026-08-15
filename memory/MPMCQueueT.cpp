/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_MPMCQUEUE_T_C
#define COMS_MEMORY_MPMCQUEUE_T_C

#include "MPMCQueueT.h"
#include "BufferMemory.cpp"
#include "MemoryArena.h"

template <typename T>
FORCE_INLINE
void queue_init(MPMCQueueT<T>* const queue, byte* buf, int capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ASSERT_TRUE((capacity & (capacity - 1)) == 0);

    queue->capacity = capacity;
    queue->mask = capacity - 1;
    queue->memory = (T *) align_up((uintptr_t) buf, alignment);

    queue->sequence = (atomic<size_t> *) align_up(
        queue->memory + queue->capacity * sizeof(T),
        ASSUMED_CACHE_LINE_SIZE
    );

    for (int i = 0; i < capacity; ++i) {
        queue->sequence[i].store((size_t) i, memory_order_relaxed);
    }

    queue->head.store(0, memory_order_relaxed);
    queue->tail.store(0, memory_order_relaxed);
}

template <typename T>
FORCE_INLINE
void queue_alloc(MPMCQueueT<T>* const queue, int capacity, int max_capacity, int alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_QUEUE_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE((capacity & (capacity - 1)) == 0);
    ASSERT_TRUE((max_capacity & (max_capacity - 1)) == 0);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating MPMCQueueT");

    byte* buf = (byte *) platform_alloc_aligned(
        capacity * sizeof(T)
            + ASSUMED_CACHE_LINE_SIZE
            + capacity * sizeof(atomic<size_t>),
        max_capacity * sizeof(T)
            + ASSUMED_CACHE_LINE_SIZE
            + max_capacity * sizeof(atomic<size_t>),
        alignment
    );

    queue_init(queue, buf, capacity, alignment);
}

template <typename T>
FORCE_INLINE
void queue_alloc(
    MPMCQueueT<T>* const queue,
    MemoryArena* const mem,
    int capacity, int max_capacity,
    uint32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_QUEUE_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE((capacity & (capacity - 1)) == 0);
    ASSERT_TRUE((max_capacity & (max_capacity - 1)) == 0);

    MemoryArena* const arena = mem_arena_add(
        mem,
        capacity * sizeof(T)
            + ASSUMED_CACHE_LINE_SIZE
            + capacity * sizeof(atomic<size_t>),
        max_capacity * sizeof(T)
            + ASSUMED_CACHE_LINE_SIZE
            + max_capacity * sizeof(atomic<size_t>),
        alignment
    );
    queue_init(queue, arena->memory, capacity, alignment);
}

template <typename T>
FORCE_INLINE
void queue_init(MPMCQueueT<T>* const queue, BufferMemory* const buf, int capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ASSERT_TRUE((capacity & (capacity - 1)) == 0);

    byte* buffer = memory_get(
        buf,
        capacity * sizeof(T)
            + ASSUMED_CACHE_LINE_SIZE
            + capacity * sizeof(atomic<size_t>),
        alignment
    );
    queue_init(queue, buffer, capacity, alignment);
}

template <typename T>
FORCE_INLINE
void queue_free(MPMCQueueT<T>* const queue) NO_EXCEPT
{
    platform_aligned_free((void **) &queue->memory);
}

template <typename T>
FORCE_INLINE
void queue_free(MPMCQueueT<T>* const queue, MemoryArena* const mem) NO_EXCEPT
{
    mem_arena_remove(mem, queue->memory);
}

template <typename T>
FORCE_INLINE
bool queue_is_empty(const MPMCQueueT<T>* const queue) NO_EXCEPT
{
    return queue->tail.load(memory_order_acquire)
        == queue->head.load(memory_order_acquire);
}

// Best-effort only, see queue_is_empty().
template <typename T>
FORCE_INLINE
bool queue_is_full(const MPMCQueueT<T>* const queue) NO_EXCEPT
{
    return (queue->head.load(memory_order_acquire)
        - queue->tail.load(memory_order_acquire)) >= (size_t) queue->capacity;
}

template <typename T>
inline
T* queue_enqueue_start(MPMCQueueT<T>* const queue) NO_EXCEPT
{
    size_t pos = queue->head.load(memory_order_relaxed);

    while (true) {
        const int index = (int) (pos & queue->mask);
        atomic<size_t>* const seq = &queue->sequence[index];
        const size_t seq_val = seq->load(memory_order_acquire);
        const intptr_t diff = (intptr_t) seq_val - (intptr_t) pos;

        if (diff == 0) {
            if (queue->head.compare_exchange_weak(
                pos, pos + 1,
                memory_order_relaxed,
                memory_order_relaxed
            )) {
                T* const slot = queue->memory + index;
                DEBUG_MEMORY_WRITE((uintptr_t) slot, sizeof(T));

                return slot;
            }
        } else if (diff < 0) {
            // Queue is full
            return NULL;
        } else {
            pos = queue->head.load(memory_order_relaxed);
        }
    }
}

template <typename T>
inline
T* queue_enqueue_start_wait(MPMCQueueT<T>* const queue) NO_EXCEPT
{
    T* slot;
    while ((slot = queue_enqueue_start(queue)) == NULL) {
        YieldProcessor();
    }

    return slot;
}

// WARNING: only call with a slot returned by queue_enqueue_start()/
// queue_enqueue_start_wait(), and only once per slot.
template <typename T>
FORCE_INLINE
void queue_enqueue_end(MPMCQueueT<T>* const queue, T* const slot) NO_EXCEPT
{
    const int index = (int) (slot - queue->memory);
    atomic<size_t>* const seq = &queue->sequence[index];

    // The slot is exclusively ours between queue_enqueue_start() and here,
    // so the sequence number is still the position we claimed it at.
    const size_t pos = seq->load(memory_order_relaxed);
    seq->store(pos + 1, memory_order_release);
}

template <typename T>
inline
bool queue_enqueue(MPMCQueueT<T>* const __restrict queue, const T* __restrict data) NO_EXCEPT
{
    T* const slot = queue_enqueue_start(queue);
    if (!slot) {
        return false;
    }

    *slot = *data;
    queue_enqueue_end(queue, slot);

    return true;
}

template <typename T>
inline
bool queue_enqueue(MPMCQueueT<T>* const __restrict queue, const T& data) NO_EXCEPT
{
    T* const slot = queue_enqueue_start(queue);
    if (!slot) {
        return false;
    }

    *slot = data;
    queue_enqueue_end(queue, slot);

    return true;
}

template <typename T>
inline
void queue_enqueue_wait(MPMCQueueT<T>* const __restrict queue, const T* __restrict data) NO_EXCEPT
{
    T* const slot = queue_enqueue_start_wait(queue);

    *slot = *data;
    queue_enqueue_end(queue, slot);
}

template <typename T>
inline
void queue_enqueue_wait(MPMCQueueT<T>* const __restrict queue, const T& data) NO_EXCEPT
{
    T* const slot = queue_enqueue_start_wait(queue);

    *slot = data;
    queue_enqueue_end(queue, slot);
}

template <typename T>
inline
T* queue_dequeue_start(MPMCQueueT<T>* const queue) NO_EXCEPT
{
    size_t pos = queue->tail.load(memory_order_relaxed);

    while (true) {
        const int index = (int) (pos & queue->mask);
        atomic<size_t>* const seq = &queue->sequence[index];
        const size_t seq_val = seq->load(memory_order_acquire);
        const intptr_t diff = (intptr_t) seq_val - (intptr_t) (pos + 1);

        if (diff == 0) {
            if (queue->tail.compare_exchange_weak(
                pos, pos + 1,
                memory_order_relaxed,
                memory_order_relaxed
            )) {
                return queue->memory + index;
            }
        } else if (diff < 0) {
            // Queue is empty
            return NULL;
        } else {
            pos = queue->tail.load(memory_order_relaxed);
        }
    }
}

template <typename T>
inline
T* queue_dequeue_start_wait(MPMCQueueT<T>* const queue) NO_EXCEPT
{
    T* slot;
    while ((slot = queue_dequeue_start(queue)) == NULL) {
        YieldProcessor();
    }

    return slot;
}

// WARNING: only call with a slot returned by queue_dequeue_start()/queue_dequeue_start_wait(), and only once per slot
template <typename T>
FORCE_INLINE
void queue_dequeue_end(MPMCQueueT<T>* const queue, T* const slot) NO_EXCEPT
{
    const int index = (int) (slot - queue->memory);
    atomic<size_t>* const seq = &queue->sequence[index];

    // Still exclusively ours, so the sequence number is still pos + 1;
    // bump it a full lap ahead so the slot becomes claimable by a producer
    // again once head wraps back around to it.
    const size_t seq_val = seq->load(memory_order_relaxed);
    seq->store(seq_val - 1 + (size_t) queue->capacity, memory_order_release);
}

template <typename T>
inline
bool queue_dequeue(MPMCQueueT<T>* const __restrict queue, T* __restrict data) NO_EXCEPT
{
    T* const slot = queue_dequeue_start(queue);
    if (!slot) {
        return false;
    }

    DEBUG_MEMORY_DELETE((uintptr_t) slot, sizeof(T));
    *data = *slot;

    queue_dequeue_end(queue, slot);

    return true;
}

template <typename T>
inline
void queue_dequeue_wait(MPMCQueueT<T>* const __restrict queue, T* __restrict data) NO_EXCEPT
{
    T* const slot = queue_dequeue_start_wait(queue);

    DEBUG_MEMORY_DELETE((uintptr_t) slot, sizeof(T));
    *data = *slot;

    queue_dequeue_end(queue, slot);
}

#endif
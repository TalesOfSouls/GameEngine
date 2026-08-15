/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_MPSCQUEUE_T_C
#define COMS_MEMORY_MPSCQUEUE_T_C

#include "MPSCQueueT.h"
#include "BufferMemory.cpp"
#include "MemoryArena.h"

template <typename T>
FORCE_INLINE
void queue_init(MPSCQueueT<T>* const queue, byte* buf, int capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    queue->capacity = capacity;
    queue->memory = (T *) align_up((uintptr_t) buf, alignment);

    queue->slot_ready = (atomic<byte> *) align_up(
        queue->memory + queue->capacity * sizeof(T),
        ASSUMED_CACHE_LINE_SIZE
    );

    for (int i = 0; i < capacity; ++i) {
        queue->slot_ready[i].store(0, memory_order_relaxed);
    }

    queue->head.store(queue->memory, memory_order_relaxed);
    queue->tail.store(queue->memory, memory_order_relaxed);
    queue->head_cache = queue->memory;
}

template <typename T>
FORCE_INLINE
void queue_alloc(MPSCQueueT<T>* const queue, int capacity, int max_capacity, int alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_QUEUE_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating MPSCQueueT");

    byte* buffer = (byte *) platform_alloc_aligned(
        capacity * sizeof(T)
            + ASSUMED_CACHE_LINE_SIZE
            + capacity * sizeof(atomic<byte>),
        max_capacity * sizeof(T)
            + ASSUMED_CACHE_LINE_SIZE
            + max_capacity * sizeof(atomic<byte>),
        alignment
    );

    queue_init(queue, buffer, capacity, alignment);
}

template <typename T>
FORCE_INLINE
void queue_alloc(
    MPSCQueueT<T>* const queue,
    MemoryArena* const mem,
    int capacity, int max_capacity,
    uint32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_QUEUE_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);

    MemoryArena* const arena = mem_arena_add(
        mem,
        capacity * sizeof(T)
            + ASSUMED_CACHE_LINE_SIZE
            + capacity * sizeof(atomic<byte>),
        max_capacity * sizeof(T)
            + ASSUMED_CACHE_LINE_SIZE
            + max_capacity * sizeof(atomic<byte>),
        alignment
    );
    queue_init(queue, arena->memory, capacity, alignment);
}

template <typename T>
FORCE_INLINE
void queue_init(MPSCQueueT<T>* const queue, BufferMemory* const buf, int capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    queue->capacity = capacity;
    queue->memory = (T *) memory_get(
        buf,
        queue->capacity * sizeof(T)
            + ASSUMED_CACHE_LINE_SIZE
            + queue->capacity * sizeof(atomic<byte>),
        alignment
    );
    queue->slot_ready = (atomic<byte> *) align_up(
        queue->memory + queue->capacity * sizeof(T),
        ASSUMED_CACHE_LINE_SIZE
    );

    for (int i = 0; i < capacity; ++i) {
        queue->slot_ready[i].store(0, memory_order_relaxed);
    }

    queue->head.store(queue->memory, memory_order_relaxed);
    queue->tail.store(queue->memory, memory_order_relaxed);
    queue->head_cache = queue->memory;
}

template <typename T>
FORCE_INLINE
void queue_free(MPSCQueueT<T>* const queue) NO_EXCEPT
{
    platform_aligned_free((void **) &queue->memory);
}

template <typename T>
FORCE_INLINE
void queue_free(MPSCQueueT<T>* const queue, MemoryArena* const mem) NO_EXCEPT
{
    mem_arena_remove(mem, queue->memory);
}

template <typename T>
FORCE_INLINE
bool queue_is_empty(const MPSCQueueT<T>* const queue) NO_EXCEPT
{
    return queue->tail.load(memory_order_acquire) == queue->head.load(memory_order_acquire);
}

template <typename T>
static inline
bool queue_has_space(const MPSCQueueT<T>* const queue) NO_EXCEPT
{
    T* const head = queue->head.load(memory_order_relaxed);
    T* next_head = head + 1;
    if (next_head >= queue->memory + queue->capacity) {
        next_head = queue->memory;
    }

    return next_head != queue->tail.load(memory_order_acquire);
}

template <typename T>
FORCE_INLINE
bool queue_is_full(const MPSCQueueT<T>* const queue) NO_EXCEPT
{
    return !queue_has_space(queue);
}

template <typename T>
inline
T* queue_reserve_slot(MPSCQueueT<T>* const queue) NO_EXCEPT
{
    T* head = queue->head.load(memory_order_relaxed);
    T* next_head;

    while (true) {
        next_head = head + 1;
        if (next_head >= queue->memory + queue->capacity) {
            next_head = queue->memory;
        }

        if (next_head == queue->tail.load(memory_order_acquire)) {
            // Queue is full
            return NULL;
        }

        if (queue->head.compare_exchange_weak(
            head, next_head,
            memory_order_relaxed,
            memory_order_relaxed
        )) {
            return head;
        }
    }
}

template <typename T>
inline
T* queue_reserve_slot_wait(MPSCQueueT<T>* const queue) NO_EXCEPT
{
    T* slot;
    while ((slot = queue_reserve_slot(queue)) == NULL) {
        YieldProcessor();
    }

    return slot;
}

template <typename T>
inline
T* queue_enqueue(MPSCQueueT<T>* const __restrict queue, const T* __restrict data) NO_EXCEPT
{
    T* const slot = queue_reserve_slot(queue);
    if (!slot) {
        return NULL;
    }

    *slot = *data;
    DEBUG_MEMORY_WRITE((uintptr_t) slot, sizeof(T));

    const int index = (int) (slot - queue->memory);

    // Publish data to the consumer
    queue->slot_ready[index].store(1, memory_order_release);

    return slot;
}

template <typename T>
inline
T* queue_enqueue(MPSCQueueT<T>* const __restrict queue, const T& data) NO_EXCEPT
{
    T* const slot = queue_reserve_slot(queue);
    if (!slot) {
        return NULL;
    }

    *slot = data;
    DEBUG_MEMORY_WRITE((uintptr_t) slot, sizeof(T));

    const int index = (int) (slot - queue->memory);
    queue->slot_ready[index].store(1, memory_order_release);

    return slot;
}

template <typename T>
inline
T* queue_enqueue_wait(MPSCQueueT<T>* const __restrict queue, const T* __restrict data) NO_EXCEPT
{
    T* const slot = queue_reserve_slot_wait(queue);

    *slot = *data;
    DEBUG_MEMORY_WRITE((uintptr_t) slot, sizeof(T));

    const int index = (int) (slot - queue->memory);
    queue->slot_ready[index].store(1, memory_order_release);

    return slot;
}

template <typename T>
inline
T* queue_enqueue_wait(MPSCQueueT<T>* const __restrict queue, const T& data) NO_EXCEPT
{
    T* const slot = queue_reserve_slot_wait(queue);

    *slot = data;
    DEBUG_MEMORY_WRITE((uintptr_t) slot, sizeof(T));

    const int index = (int) (slot - queue->memory);
    queue->slot_ready[index].store(1, memory_order_release);

    return slot;
}

template <typename T>
FORCE_INLINE
T* queue_enqueue_start(MPSCQueueT<T>* const queue) NO_EXCEPT
{
    T* const slot = queue_reserve_slot(queue);
    if (slot) {
        DEBUG_MEMORY_WRITE((uintptr_t) slot, sizeof(T));
    }

    return slot;
}

template <typename T>
FORCE_INLINE
void queue_enqueue_end(MPSCQueueT<T>* const queue, T* const slot) NO_EXCEPT
{
    const int index = (int) (slot - queue->memory);
    queue->slot_ready[index].store(1, memory_order_release);
}

// WARNING: Consumer only!
template <typename T>
FORCE_INLINE
T* queue_dequeue_keep(MPSCQueueT<T>* const queue) NO_EXCEPT
{
    T* const tail = queue->tail.load(memory_order_relaxed);

    if (tail == queue->head_cache) {
        queue->head_cache = queue->head.load(memory_order_acquire);
        if (tail == queue->head_cache) {
            // nothing claimed
            return NULL;
        }
    }

    const int index = (int) (tail - queue->memory);
    if (queue->slot_ready[index].load(memory_order_acquire) == 0) {
        // claimed but not yet published
        return NULL;
    }

    return tail;
}

// WARNING: Consumer only! Caller must have already confirmed availability via queue_dequeue_keep()
template <typename T>
FORCE_INLINE
T* queue_dequeue_start(const MPSCQueueT<T>* const queue) NO_EXCEPT
{
    return queue_dequeue_keep(queue);
}

// WARNING: Consumer only!
template <typename T>
FORCE_INLINE
void queue_dequeue_end(MPSCQueueT<T>* const queue) NO_EXCEPT
{
    T* const tail = queue->tail.load(memory_order_relaxed);
    const int index = (int) (tail - queue->memory);

    DEBUG_MEMORY_DELETE((uintptr_t) tail, sizeof(T));
    queue->slot_ready[index].store(0, memory_order_relaxed);

    T* next_tail = tail + 1;
    if (next_tail >= queue->memory + queue->capacity) {
        next_tail = queue->memory;
    }

    queue->tail.store(next_tail, memory_order_release);
}

// WARNING: Consumer only!
template <typename T>
inline
bool queue_dequeue(MPSCQueueT<T>* const __restrict queue, T* __restrict data) NO_EXCEPT
{
    T* const slot = queue_dequeue_keep(queue);
    if (!slot) {
        return false;
    }

    DEBUG_MEMORY_DELETE((uintptr_t) slot, sizeof(T));
    *data = *slot;

    const int index = (int) (slot - queue->memory);
    queue->slot_ready[index].store(0, memory_order_relaxed);

    T* next_tail = slot + 1;
    if (next_tail >= queue->memory + queue->capacity) {
        next_tail = queue->memory;
    }

    queue->tail.store(next_tail, memory_order_release);

    return true;
}

#endif

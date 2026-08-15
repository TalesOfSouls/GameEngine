/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_SPSCQUEUE_T_C
#define COMS_MEMORY_SPSCQUEUE_T_C

#include "SPSCQueueT.h"
#include "BufferMemory.cpp"
#include "MemoryArena.h"

template <typename T>
FORCE_INLINE
void queue_init(SPSCQueueT<T>* const queue, byte* buf, int capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    queue->capacity = capacity;
    queue->memory = (T *) align_up((uintptr_t) buf, alignment);

    queue->head.store(queue->memory, memory_order_relaxed);
    queue->tail.store(queue->memory, memory_order_relaxed);
    queue->tail_cache = queue->memory;
    queue->head_cache = queue->memory;
}

template <typename T>
FORCE_INLINE
void queue_alloc(SPSCQueueT<T>* const queue, int capacity, int max_capacity, int alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_QUEUE_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating SPSCQueueT");

    byte* buffer = (byte *) platform_alloc_aligned(
        capacity * sizeof(T),
        max_capacity * sizeof(T),
        alignment
    );

    queue_init(queue, buffer, capacity, alignment);
}

template <typename T>
FORCE_INLINE
void queue_alloc(
    SPSCQueueT<T>* const queue,
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
        sizeof(T) * capacity,
        sizeof(T) * max_capacity,
        alignment
    );
    queue_init(queue, arena->memory, capacity, alignment);
}

template <typename T>
FORCE_INLINE
void queue_init(SPSCQueueT<T>* const queue, BufferMemory* const buf, int capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    byte* buffer = memory_get(buf, sizeof(T) * capacity, alignment);
    queue_init(queue, buffer, capacity, alignment);
}

template <typename T>
FORCE_INLINE
void queue_free(SPSCQueueT<T>* const queue) NO_EXCEPT
{
    platform_aligned_free((void **) &queue->memory);
}

template <typename T>
FORCE_INLINE
void queue_free(SPSCQueueT<T>* const queue, MemoryArena* const mem) NO_EXCEPT
{
    mem_arena_remove(mem, queue->memory);
}

template <typename T>
FORCE_INLINE
bool queue_is_empty(const SPSCQueueT<T>* const queue) NO_EXCEPT
{
    return queue->tail.load(memory_order_acquire) == queue->head.load(memory_order_acquire);
}

// WARNING: Producer only!
template <typename T>
static inline
bool queue_has_space(SPSCQueueT<T>* const queue) NO_EXCEPT
{
    T* const head = queue->head.load(memory_order_relaxed);
    T* next_head = head + 1;
    if (next_head >= queue->memory + queue->capacity) {
        next_head = queue->memory;
    }

    if (next_head != queue->tail_cache) {
        return true;
    }

    // Cache miss, refresh from the real tail
    queue->tail_cache = queue->tail.load(memory_order_acquire);

    return next_head != queue->tail_cache;
}

// WARNING: Producer only!
template <typename T>
FORCE_INLINE
bool queue_is_full(SPSCQueueT<T>* const queue) NO_EXCEPT
{
    return !queue_has_space(queue);
}

template <typename T>
inline
T* queue_enqueue_internal(SPSCQueueT<T>* const __restrict queue, const T* __restrict data) NO_EXCEPT
{
    T* const mem = queue->head.load(memory_order_relaxed);
    *mem = *data;

    DEBUG_MEMORY_WRITE((uintptr_t) mem, sizeof(T));

    T* next_head = mem + 1;
    if (next_head >= queue->memory + queue->capacity) {
        next_head = queue->memory;
    }

    // Publishes both the new head AND the data written above to the consumer in one synchronization point
    queue->head.store(next_head, memory_order_release);

    return mem;
}

template <typename T>
inline
T* queue_enqueue_internal(SPSCQueueT<T>* const __restrict queue, const T& data) NO_EXCEPT
{
    T* const mem = queue->head.load(memory_order_relaxed);
    *mem = data;

    DEBUG_MEMORY_WRITE((uintptr_t) mem, sizeof(T));

    T* next_head = mem + 1;
    if (next_head >= queue->memory + queue->capacity) {
        next_head = queue->memory;
    }

    queue->head.store(next_head, memory_order_release);

    return mem;
}

template <typename T>
inline
T* queue_enqueue(SPSCQueueT<T>* const __restrict queue, const T* __restrict data) NO_EXCEPT
{
    if(!queue_has_space(queue)) {
        return NULL;
    }

    return queue_enqueue_internal(queue, data);
}

template <typename T>
inline
T* queue_enqueue(SPSCQueueT<T>* const __restrict queue, const T& data) NO_EXCEPT
{
    if(!queue_has_space(queue)) {
        return NULL;
    }

    return queue_enqueue_internal(queue, data);
}

template <typename T>
FORCE_INLINE
T* queue_enqueue_start(SPSCQueueT<T>* const queue) NO_EXCEPT
{
    if(!queue_has_space(queue)) {
        return NULL;
    }

    return queue->head.load(memory_order_relaxed);
}

template <typename T>
FORCE_INLINE
void queue_enqueue_end(SPSCQueueT<T>* const queue) NO_EXCEPT
{
    T* const head = queue->head.load(memory_order_relaxed);
    T* next_head = head + 1;
    if (next_head >= queue->memory + queue->capacity) {
        next_head = queue->memory;
    }

    queue->head.store(next_head, memory_order_release);
}

// WARNING: Consumer only!
template <typename T>
inline
bool queue_dequeue(SPSCQueueT<T>* const __restrict queue, T* __restrict data) NO_EXCEPT
{
    T* const tail = queue->tail.load(memory_order_relaxed);

    if (tail == queue->head_cache) {
        queue->head_cache = queue->head.load(memory_order_acquire);
        if (tail == queue->head_cache) {
            return false;
        }
    }

    DEBUG_MEMORY_DELETE((uintptr_t) tail, sizeof(T));

    *data = *tail;

    T* next_tail = tail + 1;
    if (next_tail >= queue->memory + queue->capacity) {
        next_tail = queue->memory;
    }

    queue->tail.store(next_tail, memory_order_release);

    return true;
}

// WARNING: Consumer only!
template <typename T>
FORCE_INLINE
T* queue_dequeue_keep(SPSCQueueT<T>* const queue) NO_EXCEPT
{
    T* const tail = queue->tail.load(memory_order_relaxed);

    if (tail == queue->head_cache) {
        queue->head_cache = queue->head.load(memory_order_acquire);
        if (tail == queue->head_cache) {
            return NULL;
        }
    }

    return tail;
}

// WARNING: Consumer only! Caller must have already established non-emptiness via
// queue_dequeue_keep() (or equivalent) this does not itself check.
template <typename T>
FORCE_INLINE
T* queue_dequeue_start(const SPSCQueueT<T>* const queue) NO_EXCEPT
{
    return queue->tail.load(memory_order_relaxed);
}

// WARNING: Consumer only!
template <typename T>
FORCE_INLINE
void queue_dequeue_end(SPSCQueueT<T>* const queue) NO_EXCEPT
{
    T* const tail = queue->tail.load(memory_order_relaxed);
    DEBUG_MEMORY_DELETE((uintptr_t) tail, sizeof(T));

    T* next_tail = tail + 1;
    if (next_tail >= queue->memory + queue->capacity) {
        next_tail = queue->memory;
    }

    queue->tail.store(next_tail, memory_order_release);
}

#endif
/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_QUEUE_C
#define COMS_MEMORY_QUEUE_C

#include "../stdlib/Stdlib.h"
#include "../utils/Utils.h"
#include "../thread/Atomic.h"
#include "RingMemory.cpp"
#include "BufferMemory.cpp"
#include "Queue.h"

/**
 * Ring buffer queue with elements of a fixed size
 *      head = write position (enqueue)
 *      tail = read position (dequeue)
 *      head == tail -> empty
 * One slot always stays empty to distinguish full from empty
 *
 * The memory is managed by the RingMemory functions (alloc/init/free), the element positions are managed here:
 * queue->end is the end of the last complete slot (the ring size may not be a multiple of the element size)
 * and head/tail always point to a valid slot (they wrap immediately when they reach queue->end)
 */

FORCE_INLINE
uint64 queue_capacity(const Queue* const queue) NO_EXCEPT
{
    return (uint64) (queue->end - queue->memory) / queue->element_size;
}

// Called after the ring memory was initialized
FORCE_INLINE
void queue_slots_init(Queue* const queue, uint32 element_size, uint32 alignment) NO_EXCEPT
{
    queue->element_size = element_size;
    queue->alignment = alignment;
    queue->end = queue->memory + (queue->size / element_size) * element_size;
    ASSERT_TRUE(queue->end > queue->memory);
}

// Moves a slot pointer to the next slot
FORCE_INLINE
void queue_advance(const Queue* const queue, byte** const pos) NO_EXCEPT
{
    *pos += queue->element_size;
    if (*pos >= queue->end) UNLIKELY {
        *pos = queue->memory;
    }
}

FORCE_INLINE
void queue_alloc(
    Queue* const queue,
    uint64 element_count, uint64 max_count,
    uint32 element_size,
    uint32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    ASSERT_TRUE(max_count >= element_count);
    element_size = ALIGN_UP(element_size, alignment);

    // +1 since one slot always stays empty (@see queue_has_space)
    ring_alloc((RingMemory *) queue, (element_count + 1) * element_size, (max_count + 1) * element_size, start_alignment);
    queue_slots_init(queue, element_size, alignment);
}

FORCE_INLINE
void queue_alloc(
    Queue* const queue,
    MemoryArena* const mem,
    uint64 element_count, uint64 max_count,
    uint32 element_size,
    uint32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    ASSERT_TRUE(max_count >= element_count);
    element_size = ALIGN_UP(element_size, alignment);

    // +1 since one slot always stays empty (@see queue_has_space)
    ring_alloc((RingMemory *) queue, mem, (element_count + 1) * element_size, (max_count + 1) * element_size, start_alignment);
    queue_slots_init(queue, element_size, alignment);
}

FORCE_INLINE
void queue_init(
    Queue* const queue,
    BufferMemory* const buf,
    uint64 element_count,
    uint32 element_size,
    uint32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    element_size = ALIGN_UP(element_size, alignment);

    // +1 since one slot always stays empty (@see queue_has_space)
    ring_init((RingMemory *) queue, buf, (element_count + 1) * element_size, start_alignment);
    queue_slots_init(queue, element_size, alignment);
}

FORCE_INLINE
void queue_init(
    Queue* const queue,
    byte* buf,
    uint64 element_count,
    uint32 element_size,
    uint32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    element_size = ALIGN_UP(element_size, alignment);

    // WARNING: buf is sized by the caller, one slot always stays empty -> holds element_count - 1 elements
    ring_init((RingMemory *) queue, buf, element_count * element_size, start_alignment);
    queue_slots_init(queue, element_size, alignment);
}

static inline
void thrd_queue_locks_init(Queue* queue, uint32 element_count) NO_EXCEPT
{
    mutex_init(&queue->mtx, NULL);
    coms_pthread_cond_init(&queue->cond, NULL);

    // One slot always stays empty
    coms_sem_init(&queue->empty, (int32) OMS_MIN((uint64) element_count, queue_capacity(queue) - 1));
    coms_sem_init(&queue->full, 0);
}

inline
void thrd_queue_alloc(
    Queue* queue,
    uint32 element_count,
    uint32 max_count,
    uint32 element_size,
    uint32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    queue_alloc(queue, element_count, max_count, element_size, alignment, start_alignment);
    thrd_queue_locks_init(queue, element_count);
}

inline
void thrd_queue_alloc(
    Queue* queue,
    MemoryArena* const mem,
    uint32 element_count,
    uint32 max_count,
    uint32 element_size,
    uint32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    queue_alloc(queue, mem, element_count, max_count, element_size, alignment, start_alignment);
    thrd_queue_locks_init(queue, element_count);
}

inline
void thrd_queue_init(
    Queue* queue,
    BufferMemory* const buf,
    uint32 element_count,
    uint32 element_size,
    uint32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    queue_init(queue, buf, element_count, element_size, alignment, start_alignment);
    thrd_queue_locks_init(queue, element_count);
}

inline
void thrd_queue_init(
    Queue* queue,
    byte* buf,
    uint32 element_count,
    uint32 element_size,
    uint32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    queue_init(queue, buf, element_count, element_size, alignment, start_alignment);
    thrd_queue_locks_init(queue, element_count);
}

FORCE_INLINE
void queue_free(Queue* const queue) NO_EXCEPT
{
    ring_free((RingMemory *) queue);
}

static inline
void thrd_queue_locks_free(Queue* const queue) NO_EXCEPT
{
    coms_sem_destroy(&queue->empty);
    coms_sem_destroy(&queue->full);
    mutex_destroy(&queue->mtx);
    coms_pthread_cond_destroy(&queue->cond);
}

inline
void thrd_queue_free(Queue* queue)
{
    queue_free(queue);
    thrd_queue_locks_free(queue);
}

FORCE_INLINE
void queue_free(Queue* const queue, MemoryArena* const mem) NO_EXCEPT
{
    ring_free((RingMemory *) queue, mem);
}

inline
void thrd_queue_free(Queue* const queue, MemoryArena* const mem)
{
    queue_free(queue, mem);
    thrd_queue_locks_free(queue);
}

/**
 * Changes the capacity (in elements) of a queue created with queue_alloc() (reserved memory)
 * The unhandled elements are kept (moved to the front of the buffer if necessary)
 *
 * WARNING: The semaphores of the thrd_ functions are not adjusted
 *
 * @return false if the new capacity can't hold the unhandled elements or the memory couldn't be resized
 */
inline
bool memory_resize(Queue* const queue, size_t new_capacity) NO_EXCEPT
{
    const uint32 element_size = queue->element_size;
    const size_t capacity = (size_t) queue_capacity(queue);
    size_t head_index = (size_t) (queue->head - queue->memory) / element_size;
    size_t tail_index = (size_t) (queue->tail - queue->memory) / element_size;

    // Unhandled elements: tail (read) -> head (write)
    const size_t count = head_index >= tail_index
        ? head_index - tail_index
        : capacity - tail_index + head_index;

    // One slot always stays empty
    if (new_capacity <= count) {
        return false;
    }

    if (!count) {
        head_index = 0;
        tail_index = 0;
    } else if (head_index < tail_index || head_index >= new_capacity) {
        // The data wraps around or wouldn't fit into the new size -> move it to the front
        byte* temp = (byte *) platform_alloc_aligned(element_size * count);
        if (!temp) {
            return false;
        }

        if (head_index > tail_index) {
            memcpy(temp, queue->memory + tail_index * element_size, element_size * count);
        } else {
            const size_t first = capacity - tail_index;
            memcpy(temp, queue->memory + tail_index * element_size, element_size * first);
            memcpy(temp + first * element_size, queue->memory, element_size * head_index);
        }

        memcpy(queue->memory, temp, element_size * count);
        platform_aligned_free((void **) &temp);

        tail_index = 0;
        head_index = count;
    }

    // Valid for the old and the new capacity
    queue->head = queue->memory + head_index * element_size;
    queue->tail = queue->memory + tail_index * element_size;

    if (!platform_alloc_aligned_resize(queue->memory, new_capacity * element_size)) {
        return false;
    }

    queue->size = new_capacity * element_size;
    queue->end = queue->memory + queue->size;

    return true;
}

inline
bool thrd_memory_resize(Queue* const queue, size_t new_capacity) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    return memory_resize(queue, new_capacity);
}

FORCE_INLINE
bool queue_is_empty(const Queue* const queue) NO_EXCEPT
{
    return queue->head == queue->tail;
}

FORCE_INLINE
bool thrd_queue_is_empty(Queue* const queue) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    return queue_is_empty(queue);
}

FORCE_INLINE
void queue_set_empty(Queue* const queue) NO_EXCEPT
{
    queue->head = queue->tail;
}

// Checks if one more element can be enqueued (the slot after head must not be the tail)
static inline
bool queue_has_space(const Queue* const queue) NO_EXCEPT
{
    byte* next = queue->head;
    queue_advance(queue, &next);

    return next != queue->tail;
}

FORCE_INLINE
bool queue_is_full(const Queue* const queue) NO_EXCEPT
{
    return !queue_has_space(queue);
}

inline
bool thrd_queue_is_full(Queue* queue) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    return !queue_has_space(queue);
}

// WARNING: Doesn't check if there is space (@see queue_enqueue_safe)
inline
byte* queue_enqueue(Queue* const __restrict queue, const byte* __restrict data) NO_EXCEPT
{
    byte* const mem = queue->head;
    DEBUG_MEMORY_WRITE((uintptr_t) mem, queue->element_size);

    memcpy(mem, data, queue->element_size);
    queue_advance(queue, &queue->head);

    return mem;
}

// Conditional Lock
inline
void thrd_queue_enqueue(Queue* __restrict queue, const byte* __restrict data) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    queue_enqueue(queue, data);

    coms_pthread_cond_signal(&queue->cond);
}

inline
byte* queue_enqueue_safe(Queue* const __restrict queue, const byte* __restrict data) NO_EXCEPT
{
    if (!queue_has_space(queue)) {
        return NULL;
    }

    return queue_enqueue(queue, data);
}

inline
byte* thrd_queue_enqueue_safe(Queue* const queue, const byte* __restrict data) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    if (!queue_has_space(queue)) {
        return NULL;
    }

    return queue_enqueue(queue, data);
}

// Only enqueues the data if it isn't already part of the unhandled elements
inline
void queue_enqueue_unique(Queue* const queue, const byte* data) NO_EXCEPT
{
    byte* tail = queue->tail;
    while (tail != queue->head) {
        if (memcmp(tail, data, queue->element_size) == 0) {
            return;
        }

        queue_advance(queue, &tail);
    }

    if (!queue_has_space(queue)) {
        return;
    }

    queue_enqueue(queue, data);
}

inline
void thrd_queue_enqueue_unique(Queue* __restrict queue, const byte* __restrict data) NO_EXCEPT
{
    ASSERT_TRUE((uint64_t) data % 4 == 0);
    MutexGuard _guard(&queue->mtx);

    queue_enqueue_unique(queue, data);

    coms_pthread_cond_signal(&queue->cond);
}

// @todo Create enqueue_unique_sem
inline
void thrd_queue_enqueue_unique_wait(Queue* __restrict queue, const byte* __restrict data) NO_EXCEPT
{
    ASSERT_TRUE((uint64_t) data % 4 == 0);
    MutexGuard _guard(&queue->mtx);

    // Check all unhandled elements (tail -> head)
    byte* tail = queue->tail;
    while (tail != queue->head) {
        if (memcmp(tail, data, queue->element_size) == 0) {
            // The MutexGuard unlocks the mutex
            return;
        }

        queue_advance(queue, &tail);
    }

    while (!queue_has_space(queue)) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    queue_enqueue(queue, data);

    coms_pthread_cond_signal(&queue->cond);
}

// WARNING: Doesn't check if there is space (@see queue_enqueue_start_safe)
FORCE_INLINE
byte* queue_enqueue_start(Queue* const queue) NO_EXCEPT
{
    DEBUG_MEMORY_WRITE((uintptr_t) queue->head, queue->element_size);
    return queue->head;
}

FORCE_INLINE
byte* queue_enqueue_start_safe(Queue* const queue) NO_EXCEPT
{
    if (!queue_has_space(queue)) {
        return NULL;
    }

    return queue_enqueue_start(queue);
}

inline
void thrd_queue_enqueue_wait(Queue* __restrict queue, const byte* __restrict data) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);

    while (!queue_has_space(queue)) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    queue_enqueue(queue, data);

    coms_pthread_cond_signal(&queue->cond);
}

// Semaphore Lock
inline
void thrd_queue_enqueue_sem_wait(Queue* __restrict queue, const byte* __restrict data) NO_EXCEPT
{
    coms_sem_wait(&queue->empty);
    mutex_lock(&queue->mtx);

    queue_enqueue(queue, data);

    mutex_unlock(&queue->mtx);
    coms_sem_post(&queue->full);
}

// wait in ms, @return false if the timeout was reached
inline
bool thrd_queue_enqueue_semimedwait(Queue* __restrict queue, const byte* __restrict data, uint64 wait) NO_EXCEPT
{
    if (!coms_sem_timedwait(&queue->empty, wait)) {
        return false;
    }

    {
        MutexGuard _guard(&queue->mtx);
        queue_enqueue(queue, data);
    }

    coms_sem_post(&queue->full);

    return true;
}

// Locks the mutex, unlocked by thrd_queue_enqueue_end_wait()
inline
byte* thrd_queue_enqueue_start_wait(Queue* queue) NO_EXCEPT
{
    mutex_lock(&queue->mtx);

    while (!queue_has_space(queue)) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    return queue_enqueue_start(queue);
}

// Locks the mutex, unlocked by thrd_queue_enqueue_end_sem_wait()
FORCE_INLINE
byte* thrd_queue_enqueue_start_sem_wait(Queue* queue) NO_EXCEPT
{
    coms_sem_wait(&queue->empty);
    mutex_lock(&queue->mtx);

    return queue_enqueue_start(queue);
}

FORCE_INLINE
void queue_enqueue_end(Queue* const queue) NO_EXCEPT
{
    queue_advance(queue, &queue->head);
}

FORCE_INLINE
void thrd_queue_enqueue_end_wait(Queue* queue) NO_EXCEPT
{
    queue_enqueue_end(queue);

    coms_pthread_cond_signal(&queue->cond);
    mutex_unlock(&queue->mtx);
}

FORCE_INLINE
void thrd_queue_enqueue_end_sem_wait(Queue* queue) NO_EXCEPT
{
    queue_enqueue_end(queue);

    mutex_unlock(&queue->mtx);
    coms_sem_post(&queue->full);
}

inline
bool queue_dequeue(Queue* const queue, byte* data) NO_EXCEPT
{
    if (queue->head == queue->tail) {
        return false;
    }

    DEBUG_MEMORY_READ((uintptr_t) queue->tail, queue->element_size);

    memcpy(data, queue->tail, queue->element_size);
    queue_advance(queue, &queue->tail);

    return true;
}

inline
bool thrd_queue_dequeue(Queue* __restrict queue, byte* __restrict data) NO_EXCEPT
{
    if (queue_is_empty(queue)) {
        return false;
    }

    // we do this twice because the first one is very fast but may return a false positive
    MutexGuard _guard(&queue->mtx);
    bool result = queue_dequeue(queue, data);

    coms_pthread_cond_signal(&queue->cond);

    return result;
}

FORCE_INLINE
byte* queue_dequeue_keep(const Queue* const queue) NO_EXCEPT
{
    if (queue->head == queue->tail) {
        return NULL;
    }

    return queue->tail;
}

// WARNING: Doesn't check if the queue is empty (@see queue_dequeue_keep)
FORCE_INLINE
byte* queue_dequeue_start(const Queue* const queue) NO_EXCEPT
{
    return queue->tail;
}

// Waits until a dequeue is available
inline
void thrd_queue_dequeue_wait(Queue* __restrict queue, byte* __restrict data) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);

    while (queue_is_empty(queue)) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    queue_dequeue(queue, data);

    coms_pthread_cond_signal(&queue->cond);
}

inline
byte* thrd_queue_dequeue_sem_wait(Queue* __restrict queue, byte* __restrict data) NO_EXCEPT
{
    coms_sem_wait(&queue->full);

    {
        MutexGuard _guard(&queue->mtx);
        queue_dequeue(queue, data);
    }

    coms_sem_post(&queue->empty);

    return data;
}

// wait in ms, @return false if the timeout was reached
inline
bool thrd_queue_dequeue_semimedwait(Queue* __restrict queue, byte* __restrict data, uint64 wait) NO_EXCEPT
{
    if (!coms_sem_timedwait(&queue->full, wait)) {
        return false;
    }

    {
        MutexGuard _guard(&queue->mtx);
        queue_dequeue(queue, data);
    }

    coms_sem_post(&queue->empty);

    return true;
}

// Locks the mutex, unlocked by thrd_queue_dequeue_end_wait()
FORCE_INLINE
byte* thrd_queue_dequeue_start_wait(Queue* queue) NO_EXCEPT
{
    mutex_lock(&queue->mtx);

    while (queue->head == queue->tail) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    return queue_dequeue_start(queue);
}

// Locks the mutex, unlocked by thrd_queue_dequeue_end_sem_wait()
FORCE_INLINE
byte* thrd_queue_dequeue_start_sem_wait(Queue* queue) NO_EXCEPT
{
    coms_sem_wait(&queue->full);
    mutex_lock(&queue->mtx);

    return queue_dequeue_start(queue);
}

FORCE_INLINE
void queue_dequeue_end(Queue* const queue) NO_EXCEPT
{
    queue_advance(queue, &queue->tail);
}

FORCE_INLINE
void thrd_queue_dequeue_end_wait(Queue* queue) NO_EXCEPT
{
    queue_dequeue_end(queue);

    coms_pthread_cond_signal(&queue->cond);
    mutex_unlock(&queue->mtx);
}

FORCE_INLINE
void thrd_queue_dequeue_end_sem_wait(Queue* queue) NO_EXCEPT
{
    queue_dequeue_end(queue);

    mutex_unlock(&queue->mtx);
    coms_sem_post(&queue->empty);
}

#endif

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
#include "RingMemory.cpp"
#include "BufferMemory.h"

// WARNING: Structure needs to be the same as RingMemory
// @performance Are we losing a lot of performance by using atomic_ (= volatile) in single threaded use cases
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

FORCE_INLINE
void queue_alloc(
    Queue* const queue,
    uint64 element_count, uint64 max_count,
    uint32 element_size,
    uint32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ASSERT_TRUE(max_count >= element_count);
    element_size = align_up(element_size, alignment);

    ring_alloc((RingMemory *) queue, element_count * element_size, max_count * element_size, alignment);
    queue->element_size = element_size;
}

FORCE_INLINE
void queue_alloc(
    Queue* const queue,
    MemoryArena* const mem,
    uint64 element_count, uint64 max_count,
    uint32 element_size,
    uint32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ASSERT_TRUE(max_count >= element_count);
    element_size = align_up(element_size, alignment);

    ring_alloc((RingMemory *) queue, mem, element_count * element_size, max_count * element_size, alignment);
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

static inline
void thrd_queue_locks_init(Queue* queue, uint32 element_count) NO_EXCEPT
{
    mutex_init(&queue->mtx, NULL);
    coms_pthread_cond_init(&queue->cond, NULL);

    coms_sem_init(&queue->empty, element_count);
    coms_sem_init(&queue->full, 0);
}

inline
void thrd_queue_alloc(Queue* queue, uint32 element_count, uint32 max_count, uint32 element_size, uint32 alignment = sizeof(size_t))
{
    queue_alloc(queue, element_count, max_count, element_size, alignment);
    thrd_queue_locks_init(queue, element_count);
}

inline
void thrd_queue_alloc(Queue* queue, MemoryArena* mem, uint32 element_count, uint32 max_count, uint32 element_size, uint32 alignment = sizeof(size_t))
{
    queue_alloc(queue, mem, element_count, max_count, element_size, alignment);
    thrd_queue_locks_init(queue, element_count);
}

inline
void thrd_queue_init(Queue* queue, BufferMemory* const buf, uint32 element_count, uint32 element_size, uint32 alignment = sizeof(size_t))
{
    queue_init(queue, buf, element_count, element_size, alignment);
    thrd_queue_locks_init(queue, element_count);
}

inline
void thrd_queue_init(Queue* queue, byte* buf, uint32 element_count, uint32 element_size, uint32 alignment = sizeof(size_t))
{
    queue_init(queue, buf, element_count, element_size, alignment);
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
void queue_free(Queue* const queue, MemoryArena* mem) NO_EXCEPT
{
    ring_free((RingMemory *) queue, mem);
}

inline
void thrd_queue_free(Queue* const queue, MemoryArena* mem)
{
    queue_free(queue);
    thrd_queue_locks_free(queue);
}

FORCE_INLINE
bool queue_is_empty(const Queue* const queue) NO_EXCEPT
{
    return queue->head == queue->tail;
}

FORCE_INLINE
bool queue_is_empty_atomic(const Queue* const queue) NO_EXCEPT
{
    return atomic_get_relaxed(&queue->head) == atomic_get_relaxed(&queue->tail);
}

FORCE_INLINE
bool thrd_queue_is_empty(Queue* queue) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    return queue_is_empty(queue);
}

FORCE_INLINE
void queue_set_empty(Queue* const queue) NO_EXCEPT
{
    queue->head = queue->tail;
}

FORCE_INLINE
void queue_set_empty_atomic(Queue* const queue) NO_EXCEPT
{
    atomic_set_relaxed(&queue->head, queue->tail);
}

static inline
bool queue_has_space(Queue* const queue) NO_EXCEPT
{
    return ring_commit_safe((RingMemory *) queue, queue->element_size, queue->alignment);
}

static inline
bool queue_has_space_atomic(Queue* const queue) NO_EXCEPT
{
    return ring_commit_safe_atomic((RingMemory *) queue, queue->element_size, queue->alignment);
}

FORCE_INLINE
bool queue_is_full(Queue* const queue) NO_EXCEPT
{
    return !queue_has_space(queue);
}

inline
bool thrd_queue_is_full(Queue* queue) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    return !queue_has_space();
}

FORCE_INLINE
bool queue_is_full_atomic(Queue* const queue) NO_EXCEPT
{
    return !queue_has_space_atomic(queue);
}

inline
byte* queue_enqueue(Queue* const __restrict queue, const byte* __restrict data) NO_EXCEPT
{
    byte* mem = ring_get_memory((RingMemory *) queue, queue->element_size, queue->alignment);
    memcpy(mem, data, queue->element_size);

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
byte* queue_enqueue_atomic(Queue* const __restrict queue, const byte* __restrict data) NO_EXCEPT
{
    byte* mem = ring_get_memory_atomic((RingMemory *) queue, queue->element_size, queue->alignment);
    memcpy(mem, data, queue->element_size);

    return mem;
}

inline
byte* queue_enqueue_safe(Queue* const __restrict queue, const byte* __restrict data) NO_EXCEPT
{
    if(queue_is_full(queue)) {
        return NULL;
    }

    return queue_enqueue(queue, data);
}

inline
byte* thrd_queue_enqueue_safe(Queue* const queue, const byte* __restrict data) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    if(!queue_has_space(queue)) {
        return NULL;
    }

    return queue_enqueue(queue, data);
}

inline
void queue_enqueue_unique(Queue* const queue, const byte* data) NO_EXCEPT
{
    byte* tail = queue->tail;
    while (tail != queue->head) {
        if (memcmp(tail, data, queue->element_size) == 0) {
            return;
        }

        ring_move_pointer((RingMemory *) queue, &tail, queue->element_size, queue->alignment);
    }

    if (!queue_has_space((RingMemory *) queue, queue->element_size, queue->alignment)) {
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

// @todo Create enqueue_unique and enqueue_unique_sem
inline
void thrd_queue_enqueue_unique_wait(Queue* __restrict queue, const byte* __restrict data) NO_EXCEPT
{
    ASSERT_TRUE((uint64_t) data % 4 == 0);
    MutexGuard _guard(&queue->mtx);

    byte* tail = queue->tail;
    while (tail != queue->tail) {
        ASSERT_TRUE((uint64_t) tail % 4 == 0);

        // @performance we could probably make this faster since we don't need to compare the entire range
        if (memcmp(tail, data, queue->element_size) == 0) {
            mutex_unlock(&queue->mtx);

            return;
        }

        ring_move_pointer((RingMemory *) queue, &tail, queue->element_size, queue->alignment);
    }

    while (!queue_has_space((RingMemory *) queue)) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    queue_enqueue(queue, data);

    coms_pthread_cond_signal(&queue->cond);
}

FORCE_INLINE
byte* queue_enqueue_start(Queue* const queue) NO_EXCEPT
{
    return ring_get_memory_nomove((RingMemory *) queue, queue->element_size, queue->alignment);
}

FORCE_INLINE
byte* queue_enqueue_start_safe(Queue* const queue) NO_EXCEPT
{
    if(!queue_has_space(queue)) {
        return NULL;
    }

    return ring_get_memory_nomove((RingMemory *) queue, queue->element_size, queue->alignment);
}

inline
void thrd_queue_enqueue_wait(Queue* __restrict queue, const byte* __restrict data) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);

    while (!ring_commit_safe((RingMemory *) queue, queue->element_size, queue->alignment)) {
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

inline
bool thrd_queue_enqueue_semimedwait(Queue* __restrict queue, const byte* __restrict data, uint64 wait) NO_EXCEPT
{
    if (semimedwait(&queue->empty, wait)) {
        return false;
    }

    {
        MutexGuard _guard(&queue->mtx);
        queue_enqueue(queue, data);
    }

    coms_sem_post(&queue->full);

    return true;
}

inline
byte* thrd_queue_enqueue_start_wait(Queue* queue) NO_EXCEPT
{
    mutex_lock(&queue->mtx);

    while (!ring_commit_safe((RingMemory *) queue, queue->element_size, queue->alignment)) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    return queue_enqueue_start(queue);
}

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
    ring_move_pointer((RingMemory *) queue, &queue->head, queue->element_size, queue->alignment);
}

FORCE_INLINE
void thrd_queue_enqueue_end_wait(Queue* queue) NO_EXCEPT
{
    queue_enqueue_end(queue);

    coms_pthread_cond_signal(&queue->cond);
    muted_unlock(&queue->mtx);
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

    memcpy(data, queue->tail, queue->element_size);
    ring_move_pointer((RingMemory *) queue, &queue->tail, queue->element_size, queue->alignment);

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
byte* queue_dequeue_keep(Queue* const queue) NO_EXCEPT
{
    if (queue->head == queue->tail) {
        return NULL;
    }

    return queue->tail;
}

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
}

inline
bool thrd_queue_dequeue_semimedwait(Queue* __restrict queue, byte* __restrict data, uint64 wait) NO_EXCEPT
{
    if (semimedwait(&queue->full, wait)) {
        return false;
    }

    {
        MutexGuard _guard(&queue->mtx);
        queue_dequeue(queue, data);
    }

    coms_sem_post(&queue->empty);

    return true;
}

FORCE_INLINE
byte* thrd_queue_dequeue_start_wait(Queue* queue) NO_EXCEPT
{
    mutex_lock(&queue->mtx);

    while (queue->head == queue->tail) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    return queue_dequeue_start(queue);
}

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
    ring_move_pointer((RingMemory *) queue, &queue->tail, queue->element_size, queue->alignment);
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
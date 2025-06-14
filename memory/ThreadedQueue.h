/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MEMORY_THREADED_QUEUE_H
#define COMS_MEMORY_THREADED_QUEUE_H

// @todo This is a horrible implementation. Please implement a lock free solution
/**
 * WARNING: This implementation is a single-producer, single-consumer (SPSC) implementation
 */

#include "../stdlib/Types.h"
#include "../utils/Utils.h"
#include "RingMemory.h"
#include "../thread/Thread.h"
#include "../thread/Semaphore.h"

struct ThreadedQueue {
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

    // We support both conditional locking and semaphore locking
    // These values are not initialized and not used unless you use the queue
    mutex mtx;
    mutex_cond cond;

    sem empty;
    sem full;
};

inline
void thrd_queue_alloc(ThreadedQueue* queue, uint32 element_count, uint32 element_size, uint32 alignment = 64)
{
    element_size = ROUND_TO_NEAREST(element_size, alignment);

    ring_alloc((RingMemory *) queue, element_count * element_size, alignment);

    queue->element_size = element_size;

    mutex_init(&queue->mtx, NULL);
    coms_pthread_cond_init(&queue->cond, NULL);

    coms_sem_init(&queue->empty, element_count);
    coms_sem_init(&queue->full, 0);
}

inline
void thrd_queue_init(ThreadedQueue* queue, BufferMemory* buf, uint32 element_count, uint32 element_size, uint32 alignment = 64)
{
    element_size = ROUND_TO_NEAREST(element_size, alignment);

    ring_init((RingMemory *) queue, buf, element_count * element_size, alignment);

    queue->element_size = element_size;

    mutex_init(&queue->mtx, NULL);
    coms_pthread_cond_init(&queue->cond, NULL);

    coms_sem_init(&queue->empty, element_count);
    coms_sem_init(&queue->full, 0);
}

inline
void thrd_queue_init(ThreadedQueue* queue, byte* buf, uint32 element_count, uint32 element_size, uint32 alignment = 64)
{
    element_size = ROUND_TO_NEAREST(element_size, alignment);

    ring_init((RingMemory *) queue, buf, element_count * element_size, alignment);

    queue->element_size = element_size;

    mutex_init(&queue->mtx, NULL);
    coms_pthread_cond_init(&queue->cond, NULL);

    coms_sem_init(&queue->empty, element_count);
    coms_sem_init(&queue->full, 0);
}

inline
void thrd_queue_free(ThreadedQueue* queue)
{
    ring_free((RingMemory *) queue);
    coms_sem_destroy(&queue->empty);
    coms_sem_destroy(&queue->full);
    mutex_destroy(&queue->mtx);
    coms_pthread_cond_destroy(&queue->cond);
}

// @todo Create enqueue_unique and enqueue_unique_sem
inline
void thrd_queue_enqueue_unique_wait(ThreadedQueue* queue, const byte* data) noexcept
{
    ASSERT_SIMPLE((uint64_t) data % 4 == 0);
    mutex_lock(&queue->mtx);

    byte* tail = queue->tail;
    while (tail != queue->tail) {
        ASSERT_SIMPLE((uint64_t) tail % 4 == 0);

        // @performance we could probably make this faster since we don't need to compare the entire range
        if (is_equal(tail, data, queue->element_size) == 0) {
            mutex_unlock(&queue->mtx);

            return;
        }

        ring_move_pointer((RingMemory *) queue, &tail, queue->element_size, queue->alignment);
    }

    while (!ring_commit_safe((RingMemory *) queue, queue->element_size, queue->alignment)) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    byte* mem = ring_get_memory((RingMemory *) queue, queue->element_size, queue->alignment);
    memcpy(mem, data, queue->element_size);

    coms_pthread_cond_signal(&queue->cond);
    mutex_unlock(&queue->mtx);
}

inline
void thrd_queue_enqueue_unique(ThreadedQueue* queue, const byte* data) noexcept
{
    ASSERT_SIMPLE((uint64_t) data % 4 == 0);
    mutex_lock(&queue->mtx);

    byte* tail = queue->tail;
    while (tail != queue->tail) {
        ASSERT_SIMPLE((uint64_t) tail % 4 == 0);

        // @performance we could probably make this faster since we don't need to compare the entire range
        if (is_equal(tail, data, queue->element_size) == 0) {
            mutex_unlock(&queue->mtx);

            return;
        }

        ring_move_pointer((RingMemory *) queue, &tail, queue->element_size, queue->alignment);
    }

    if (!ring_commit_safe((RingMemory *) queue, queue->element_size, queue->alignment)) {
        mutex_unlock(&queue->mtx);

        return;
    }

    byte* mem = ring_get_memory((RingMemory *) queue, queue->element_size, queue->alignment);
    memcpy(mem, data, queue->element_size);

    coms_pthread_cond_signal(&queue->cond);
    mutex_unlock(&queue->mtx);
}

// Conditional Lock
inline
void thrd_queue_enqueue(ThreadedQueue* queue, const byte* data) noexcept
{
    mutex_lock(&queue->mtx);

    if (!ring_commit_safe((RingMemory *) queue, queue->element_size, queue->alignment)) {
        mutex_unlock(&queue->mtx);

        return;
    }

    byte* mem = ring_get_memory((RingMemory *) queue, queue->element_size, queue->alignment);
    memcpy(mem, data, queue->element_size);

    coms_pthread_cond_signal(&queue->cond);
    mutex_unlock(&queue->mtx);
}

inline
void thrd_queue_enqueue_wait(ThreadedQueue* queue, const byte* data) noexcept
{
    mutex_lock(&queue->mtx);

    while (!ring_commit_safe((RingMemory *) queue, queue->element_size, queue->alignment)) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    byte* mem = ring_get_memory((RingMemory *) queue, queue->element_size, queue->alignment);
    memcpy(mem, data, queue->element_size);

    coms_pthread_cond_signal(&queue->cond);
    mutex_unlock(&queue->mtx);
}

inline
byte* thrd_queue_enqueue_start_wait(ThreadedQueue* queue) noexcept
{
    mutex_lock(&queue->mtx);

    while (!ring_commit_safe((RingMemory *) queue, queue->element_size, queue->alignment)) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    return ring_get_memory((RingMemory *) queue, queue->element_size, queue->alignment);
}

inline
void thrd_queue_enqueue_end_wait(ThreadedQueue* queue) noexcept
{
    coms_pthread_cond_signal(&queue->cond);
    mutex_unlock(&queue->mtx);
}

inline
bool thrd_queue_dequeue(ThreadedQueue* queue, byte* data) noexcept
{
    if (queue->head == queue->tail) {
        return false;
    }

    // we do this twice because the first one is very fast but may return a false positive
    mutex_lock(&queue->mtx);
    if (queue->head == queue->tail) {
        mutex_unlock(&queue->mtx);

        return false;
    }

    if (queue->element_size == 4) {
        *((int32 *) data) = *((int32 *) queue->tail);
    } else {
        memcpy(data, queue->tail, queue->element_size);
    }
    ring_move_pointer((RingMemory *) queue, &queue->tail, queue->element_size, queue->alignment);

    coms_pthread_cond_signal(&queue->cond);
    mutex_unlock(&queue->mtx);

    return true;
}

inline
bool thrd_queue_empty(ThreadedQueue* queue) noexcept {
    mutex_lock(&queue->mtx);
    bool is_empty = queue->head == queue->tail;
    mutex_unlock(&queue->mtx);

    return is_empty;
}

inline
bool thrd_queue_full(ThreadedQueue* queue) noexcept {
    mutex_lock(&queue->mtx);
    bool is_full = !ring_commit_safe((RingMemory *) queue, queue->element_size, queue->alignment);
    mutex_unlock(&queue->mtx);

    return is_full;
}

// Waits until a dequeue is available
inline
void thrd_queue_dequeue_wait(ThreadedQueue* queue, byte* data) noexcept
{
    mutex_lock(&queue->mtx);

    while (queue->head == queue->tail) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    memcpy(data, queue->tail, queue->element_size);
    ring_move_pointer((RingMemory *) queue, &queue->tail, queue->element_size, queue->alignment);

    coms_pthread_cond_signal(&queue->cond);
    mutex_unlock(&queue->mtx);
}

inline
byte* thrd_queue_dequeue_start_wait(ThreadedQueue* queue) noexcept
{
    mutex_lock(&queue->mtx);

    while (queue->head == queue->tail) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    return queue->tail;
}

inline
void thrd_queue_dequeue_end_wait(ThreadedQueue* queue) noexcept
{
    ring_move_pointer((RingMemory *) queue, &queue->tail, queue->element_size, queue->alignment);

    coms_pthread_cond_signal(&queue->cond);
    mutex_unlock(&queue->mtx);
}

// Semaphore Lock
inline
void thrd_queue_enqueue_sem_wait(ThreadedQueue* queue, const byte* data) noexcept
{
    coms_sem_wait(&queue->empty);
    mutex_lock(&queue->mtx);

    byte* mem = ring_get_memory((RingMemory *) queue, queue->element_size, queue->alignment);
    memcpy(mem, data, queue->element_size);

    mutex_unlock(&queue->mtx);
    coms_sem_post(&queue->full);
}

inline
bool thrd_queue_enqueue_semimedwait(ThreadedQueue* queue, const byte* data, uint64 wait) noexcept
{
    if (semimedwait(&queue->empty, wait)) {
        return false;
    }

    mutex_lock(&queue->mtx);

    byte* mem = ring_get_memory((RingMemory *) queue, queue->element_size, queue->alignment);
    memcpy(mem, data, queue->element_size);

    mutex_unlock(&queue->mtx);
    coms_sem_post(&queue->full);

    return true;
}

inline
byte* thrd_queue_enqueue_start_sem_wait(ThreadedQueue* queue) noexcept
{
    coms_sem_wait(&queue->empty);
    mutex_lock(&queue->mtx);

    return ring_get_memory((RingMemory *) queue, queue->element_size, queue->alignment);
}

inline
void thrd_queue_enqueue_end_sem_wait(ThreadedQueue* queue) noexcept
{
    mutex_unlock(&queue->mtx);
    coms_sem_post(&queue->full);
}

inline
byte* thrd_queue_dequeue_sem_wait(ThreadedQueue* queue, byte* data) noexcept
{
    coms_sem_wait(&queue->full);
    mutex_lock(&queue->mtx);

    memcpy(data, queue->tail, queue->element_size);
    ring_move_pointer((RingMemory *) queue, &queue->tail, queue->element_size, queue->alignment);

    mutex_unlock(&queue->mtx);
    coms_sem_post(&queue->empty);
}

inline
bool thrd_queue_dequeue_semimedwait(ThreadedQueue* queue, byte* data, uint64 wait) noexcept
{
    if (semimedwait(&queue->full, wait)) {
        return false;
    }

    mutex_lock(&queue->mtx);

    memcpy(data, queue->tail, queue->element_size);
    ring_move_pointer((RingMemory *) queue, &queue->tail, queue->element_size, queue->alignment);

    mutex_unlock(&queue->mtx);
    coms_sem_post(&queue->empty);

    return true;
}

inline
byte* thrd_queue_dequeue_start_sem_wait(ThreadedQueue* queue) noexcept
{
    coms_sem_wait(&queue->full);
    mutex_lock(&queue->mtx);

    return queue->tail;
}

inline
void thrd_queue_dequeue_end_sem_wait(ThreadedQueue* queue) noexcept
{
    ring_move_pointer((RingMemory *) queue, &queue->tail, queue->element_size, queue->alignment);

    mutex_unlock(&queue->mtx);
    coms_sem_post(&queue->empty);
}

#endif
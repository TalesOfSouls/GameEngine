/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_QUEUE_T_C
#define COMS_MEMORY_QUEUE_T_C

#include "QueueT.h"

template <typename T>
FORCE_INLINE
void queue_init(QueueT<T>* const queue, byte* buf, int capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    queue->capacity = capacity;
    queue->memory = (T *) align_up((uintptr_t) buf, alignment);
    queue->head = queue->memory;
    queue->tail = queue->memory;
}

template <typename T>
FORCE_INLINE
void queue_alloc(QueueT<T>* const queue, int capacity, int max_capacity, int alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_QUEUE_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating QueueT");

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
    QueueT<T>* const queue,
    MemoryArena* const mem,
    int capacity, int max_capacity,
    uint32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_QUEUE_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
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
void queue_init(QueueT<T>* const queue, BufferMemory* const buf, int capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    byte* buffer = memory_get(buf, sizeof(T) * capacity, alignment);
    queue_init(queue, buffer, capacity, alignment);
}

template <typename T>
static inline
void thrd_queue_locks_init(QueueT<T>* const queue) NO_EXCEPT
{
    mutex_init(&queue->mtx, NULL);
    coms_pthread_cond_init(&queue->cond, NULL);

    coms_sem_init(&queue->empty, queue->capacity);
    coms_sem_init(&queue->full, 0);
}

template <typename T>
inline
void thrd_queue_alloc(QueueT<T>* const queue, uint32 capacity, uint32 max_capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    queue_alloc(queue, capacity, max_capacity, alignment);
    thrd_queue_locks_init(queue);
}

template <typename T>
inline
void thrd_queue_alloc(QueueT<T>* const queue, MemoryArena* const mem, uint32 capacity, uint32 max_capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    queue_alloc(queue, mem, capacity, max_capacity, alignment);
    thrd_queue_locks_init(queue);
}

template <typename T>
inline
void thrd_queue_init(QueueT<T>* const queue, BufferMemory* const buf, uint32 capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    queue_init(queue, buf, capacity, alignment);
    thrd_queue_locks_init(queue);
}

template <typename T>
inline
void thrd_queue_init(QueueT<T>* const queue, byte* buf, uint32 capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    queue_init(queue, buf, capacity, alignment);
    thrd_queue_locks_init(queue);
}

template <typename T>
FORCE_INLINE
void queue_free(QueueT<T>* const queue) NO_EXCEPT
{
    platform_aligned_free((void **) &queue->memory);
}

template <typename T>
static inline
void thrd_queue_locks_free(QueueT<T>* const queue) NO_EXCEPT
{
    coms_sem_destroy(&queue->empty);
    coms_sem_destroy(&queue->full);
    mutex_destroy(&queue->mtx);
    coms_pthread_cond_destroy(&queue->cond);
}

template <typename T>
inline
void thrd_queue_free(QueueT<T>* const queue) NO_EXCEPT
{
    queue_free(queue);
    thrd_queue_locks_free(queue);
}

template <typename T>
FORCE_INLINE
void queue_free(QueueT<T>* const queue, MemoryArena* const mem) NO_EXCEPT
{
    mem_arena_remove(mem, queue->memory);
}

template <typename T>
inline
void thrd_queue_free(QueueT<T>* const queue, MemoryArena* const mem) NO_EXCEPT
{
    queue_free(queue, mem);
    thrd_queue_locks_free(queue);
}

template <typename T>
inline
bool memory_resize(QueueT<T>* const queue, size_t new_capacity) NO_EXCEPT
{
    T* old_memory = queue->memory;
    int old_capacity = queue->capacity;

    size_t head_index = queue->head - old_memory;
    size_t tail_index = queue->tail - old_memory;

    // Number of live/unhandled elements currently in the queue
    const size_t count = (tail_index >= head_index)
        ? (tail_index - head_index)
        : (old_capacity - head_index + tail_index);

    // Refuse a resize that would drop unhandled data
    if (new_capacity < count) {
        return false;
    }

    // If the live data currently wraps around the end of the buffer,
    // linearize it first so realloc doesn't split/corrupt it.
    if (tail_index < head_index && count > 0) {
        T* temp = (T*) platform_alloc_aligned(sizeof(T) * count);
        if (!temp) {
            return false;
        }

        size_t first_chunk = old_capacity - head_index;
        memcpy(temp, old_memory + head_index, sizeof(T) * first_chunk);
        memcpy(temp + first_chunk, old_memory, sizeof(T) * tail_index);

        memcpy(old_memory, temp, sizeof(T) * count);
        platform_free_aligned(temp);

        head_index = 0;
        tail_index = count;
    }

    if (!platform_alloc_aligned_resize(queue->memory, sizeof(T) * new_capacity)) {
        return false;
    }

    queue->capacity = new_capacity;

    // memory may have moved (realloc) and/or been linearized above —
    // re-point head/tail into the new buffer rather than assuming the
    // old raw pointer offsets are still valid
    queue->head = queue->memory + head_index;
    queue->tail = queue->memory + (tail_index % new_capacity);

    return true;
}

template <typename T>
inline
bool thrd_memory_resize(QueueT<T>* const queue, size_t new_capacity) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    return memory_resize(queue, new_capacity);
}

template <typename T>
FORCE_INLINE
bool queue_is_empty(const QueueT<T>* const queue) NO_EXCEPT
{
    return queue->head == queue->tail;
}

template <typename T>
FORCE_INLINE
bool thrd_queue_is_empty(QueueT<T>* const queue) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    return queue_is_empty(queue);
}

template <typename T>
FORCE_INLINE
void queue_set_empty(QueueT<T>* const queue) NO_EXCEPT
{
    queue->head = queue->tail;
}

template <typename T>
static inline
bool queue_has_space(const QueueT<T>* const queue) NO_EXCEPT
{
    return queue->tail - 1 != queue->head;
}

template <typename T>
FORCE_INLINE
bool queue_is_full(const QueueT<T>* const queue) NO_EXCEPT
{
    return !queue_has_space(queue);
}

template <typename T>
inline
bool thrd_queue_is_full(QueueT<T>* const queue) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    return !queue_has_space(queue);
}

template <typename T>
inline
T* queue_enqueue(QueueT<T>* const __restrict queue, const T* __restrict data) NO_EXCEPT
{
    *queue->head = *data;
    T* mem = queue->head;

    DEBUG_MEMORY_WRITE((uintptr_t) mem, sizeof(T));

    OMS_WRAPPED_INC_SE(
        queue->head,
        queue->memory,
        queue->memory + queue->capacity
    );

    return mem;
}

template <typename T>
inline
T* queue_enqueue(QueueT<T>* const __restrict queue, T data) NO_EXCEPT
{
    *queue->head = data;
    T* mem = queue->head;

    DEBUG_MEMORY_WRITE((uintptr_t) mem, sizeof(T));

    OMS_WRAPPED_INC_SE(
        queue->head,
        queue->memory,
        queue->memory + queue->capacity
    );

    return mem;
}

// Conditional Lock
template <typename T>
inline
void thrd_queue_enqueue(QueueT<T>* __restrict queue, const T* __restrict data) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    queue_enqueue(queue, data);

    coms_pthread_cond_signal(&queue->cond);
}

template <typename T>
inline
void thrd_queue_enqueue(QueueT<T>* __restrict queue, T data) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    queue_enqueue(queue, data);

    coms_pthread_cond_signal(&queue->cond);
}

template <typename T>
inline
T* queue_enqueue_safe(QueueT<T>* const __restrict queue, const T* __restrict data) NO_EXCEPT
{
    if(!queue_has_space(queue)) {
        return NULL;
    }

    return queue_enqueue(queue, data);
}

template <typename T>
inline
T* thrd_queue_enqueue_safe(QueueT<T>* const __restrict queue, const T* __restrict data) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    if(!queue_has_space(queue)) {
        return NULL;
    }

    return queue_enqueue(queue, data);
}

template <typename T>
inline
T* queue_enqueue_safe(QueueT<T>* const __restrict queue, T data) NO_EXCEPT
{
    if(!queue_has_space(queue)) {
        return NULL;
    }

    return queue_enqueue(queue, data);
}

template <typename T>
inline
T* thrd_queue_enqueue_safe(QueueT<T>* const __restrict queue, T data) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    if(!queue_has_space(queue)) {
        return NULL;
    }

    return queue_enqueue(queue, data);
}

template <typename T>
inline
void queue_enqueue_unique(QueueT<T>* const __restrict queue, const T* __restrict data) NO_EXCEPT
{
    T* tail = queue->tail;
    while (tail != queue->head) {
        if (memcmp(tail, data, sizeof(T)) == 0) {
            return;
        }

        OMS_WRAPPED_INC_SE(
            tail,
            queue->memory,
            queue->memory + queue->capacity
        );
    }

    // @performance This feels like it is performing some of the cost of the while loop above
    if (!queue_has_space(queue)) {
        return;
    }

    queue_enqueue(queue, data);
}

template <typename T>
inline
void thrd_queue_enqueue_unique(QueueT<T>* __restrict queue, const T* __restrict data) NO_EXCEPT
{
    ASSERT_TRUE((uint64_t) data % 4 == 0);
    MutexGuard _guard(&queue->mtx);

    queue_enqueue_unique(queue, data);

    coms_pthread_cond_signal(&queue->cond);
}

// @todo Create enqueue_unique_sem
template <typename T>
inline
void thrd_queue_enqueue_unique_wait(QueueT<T>* __restrict queue, const T* __restrict data) NO_EXCEPT
{
    ASSERT_TRUE((uint64_t) data % 4 == 0);
    MutexGuard _guard(&queue->mtx);

    T* tail = queue->tail;
    while (tail != queue->tail) {
        ASSERT_TRUE((uint64_t) tail % 4 == 0);

        // @performance we could probably make this faster since we don't need to compare the entire range
        if (memcmp(tail, data, sizeof(T)) == 0) {
            mutex_unlock(&queue->mtx);

            return;
        }

        OMS_WRAPPED_INC_SE(
            tail,
            queue->memory,
            queue->memory + queue->capacity
        );
    }

    while (!queue_enqueue_safe(queue, data)) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    coms_pthread_cond_signal(&queue->cond);
}

template <typename T>
FORCE_INLINE
T* queue_enqueue_start(const QueueT<T>* const queue) NO_EXCEPT
{
    DEBUG_MEMORY_WRITE((uintptr_t) queue->head, sizeof(T));
    return queue->head;
}

template <typename T>
FORCE_INLINE
T* queue_enqueue_start_safe(const QueueT<T>* const queue) NO_EXCEPT
{
    if(!queue_has_space(queue)) {
        return NULL;
    }

    return queue->head;
}

template <typename T>
inline
void thrd_queue_enqueue_wait(QueueT<T>* __restrict queue, const T* __restrict data) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);

    while (!queue_enqueue_safe(queue, data)) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    coms_pthread_cond_signal(&queue->cond);
}

/**
 * Enqueues data into a queue and waits until the enqueue was successful
 */
template <typename T>
inline
void thrd_queue_enqueue_wait(QueueT<T>* __restrict queue, T data) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);

    while (!queue_enqueue_safe(queue, data)) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    coms_pthread_cond_signal(&queue->cond);
}

template <typename T>
inline
void thrd_queue_enqueue_sem_wait(QueueT<T>* __restrict queue, const T* __restrict data) NO_EXCEPT
{
    coms_sem_wait(&queue->empty);
    mutex_lock(&queue->mtx);

    queue_enqueue(queue, data);

    mutex_unlock(&queue->mtx);
    coms_sem_post(&queue->full);
}

template <typename T>
inline
bool thrd_queue_enqueue_semimedwait(QueueT<T>* __restrict queue, const T* __restrict data, uint64 wait) NO_EXCEPT
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

template <typename T>
inline
T* thrd_queue_enqueue_start_wait(QueueT<T>* const queue) NO_EXCEPT
{
    mutex_lock(&queue->mtx);

    while (!queue_has_space(queue)) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    return queue_enqueue_start(queue);
}

template <typename T>
FORCE_INLINE
T* thrd_queue_enqueue_start_sem_wait(QueueT<T>* const queue) NO_EXCEPT
{
    coms_sem_wait(&queue->empty);
    mutex_lock(&queue->mtx);

    DEBUG_MEMORY_WRITE((uintptr_t) queue->head, sizeof(T));

    return queue->head;
}

template <typename T>
FORCE_INLINE
void queue_enqueue_end(QueueT<T>* const queue) NO_EXCEPT
{
    OMS_WRAPPED_INC_SE(
        queue->head,
        queue->memory,
        queue->memory + queue->capacity
    );
}

template <typename T>
FORCE_INLINE
void thrd_queue_enqueue_end_wait(QueueT<T>* const queue) NO_EXCEPT
{
    queue_enqueue_end(queue);
    coms_pthread_cond_signal(&queue->cond);

    mutex_unlock(&queue->mtx);
}

template <typename T>
FORCE_INLINE
void thrd_queue_enqueue_end_sem_wait(QueueT<T>* const queue) NO_EXCEPT
{
    mutex_unlock(&queue->mtx);
    coms_sem_post(&queue->full);
}

template <typename T>
inline
bool queue_dequeue(QueueT<T>* const __restrict queue, T* __restrict data) NO_EXCEPT
{
    if (queue->head == queue->tail) {
        return false;
    }

    DEBUG_MEMORY_DELETE((uintptr_t) queue->tail, sizeof(T));

    *data = *queue->tail;
    OMS_WRAPPED_INC_SE(
        queue->tail,
        queue->memory,
        queue->memory + queue->capacity
    );

    return true;
}

template <typename T>
inline
bool thrd_queue_dequeue(QueueT<T>* __restrict queue, T* __restrict data) NO_EXCEPT
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

// Waits until a dequeue is available
template <typename T>
inline
void thrd_queue_dequeue_wait(QueueT<T>* __restrict queue, T* __restrict data) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);

    while (queue_is_empty(queue)) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    queue_dequeue(queue, data);

    coms_pthread_cond_signal(&queue->cond);
}

template <typename T>
inline
T* thrd_queue_dequeue_sem_wait(QueueT<T>* __restrict queue, T* __restrict data) NO_EXCEPT
{
    coms_sem_wait(&queue->full);

    {
        MutexGuard _guard(&queue->mtx);
        queue_dequeue(queue, data);
    }

    coms_sem_post(&queue->empty);
}

template <typename T>
inline
bool thrd_queue_dequeue_semimedwait(QueueT<T>* __restrict queue, T* __restrict data, uint64 wait) NO_EXCEPT
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

template <typename T>
FORCE_INLINE
T* queue_dequeue_keep(const QueueT<T>* const queue) NO_EXCEPT
{
    if (queue_is_empty(queue)) {
        return NULL;
    }

    return queue->tail;
}

template <typename T>
FORCE_INLINE
T* queue_dequeue_start(const QueueT<T>* const queue) NO_EXCEPT
{
    return queue->tail;
}

template <typename T>
FORCE_INLINE
T* thrd_queue_dequeue_start_wait(QueueT<T>* const queue) NO_EXCEPT
{
    mutex_lock(&queue->mtx);

    while (queue_is_empty(queue)) {
        coms_pthread_cond_wait(&queue->cond, &queue->mtx);
    }

    return queue_dequeue_start(queue);
}

template <typename T>
FORCE_INLINE
T* thrd_queue_dequeue_start_sem_wait(QueueT<T>* const queue) NO_EXCEPT
{
    coms_sem_wait(&queue->full);
    mutex_lock(&queue->mtx);

    return queue_dequeue_start(queue);
}

template <typename T>
FORCE_INLINE
void queue_dequeue_end(QueueT<T>* const queue) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE((uintptr_t) queue->tail, sizeof(T));
    OMS_WRAPPED_INC_SE(
        queue->tail,
        queue->memory,
        queue->memory + queue->capacity
    );
}

template <typename T>
FORCE_INLINE
void thrd_queue_dequeue_end_wait(QueueT<T>* const queue) NO_EXCEPT
{
    queue_dequeue_end(queue);

    coms_pthread_cond_signal(&queue->cond);
    mutex_unlock(&queue->mtx);
}

template <typename T>
FORCE_INLINE
void thrd_queue_dequeue_end_sem_wait(QueueT<T>* const queue) NO_EXCEPT
{
    queue_dequeue_end(queue);

    mutex_unlock(&queue->mtx);
    coms_sem_post(&queue->empty);
}

#endif
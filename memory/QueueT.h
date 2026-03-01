/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MEMORY_QUEUE_T_H
#define COMS_MEMORY_QUEUE_T_H

#include "../stdlib/Stdlib.h"
#include "../system/Allocator.h"
#include "BufferMemory.h"
#include "MemoryArena.h"
#include "../thread/Thread.h"
#include "../thread/Semaphore.h"

/**
 * This queue implementation can be used single threaded or multi threaded
 * The programmer is responsible for calling the appropriate functions.
 * This also goes for SPSC vs. SPMC, MPMC, ...
 * This of course puts more mental load on the programmer but makes this queue very powerful
 */
// @performance Are we losing a lot of performance by using atomic_ (= volatile) in single threaded use cases
template <typename T>
struct QueueT {
    T* memory;
    atomic_ptr T* head;
    atomic_ptr T* tail;

    int capacity;

    // We support both conditional locking and semaphore locking
    // These values are not initialized and not used unless you use the queue
    mutex mtx;
    mutex_cond cond;

    sem empty;
    sem full;
};

template <typename T>
FORCE_INLINE
void queue_alloc(QueueT<T>* const queue, int capacity, int max_capacity, int alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE(PROFILE_QUEUE_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating QueueT");

    queue->capacity = capacity;
    queue->memory = (T *) platform_alloc_aligned(
        queue->capacity * sizeof(T),
        max_capacity * sizeof(T),
        alignment
    );
    queue->head = queue->memory;
    queue->tail = queue->memory;
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
    PROFILE(PROFILE_QUEUE_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(max_capacity >= capacity);
    queue->capacity = capacity;

    MemoryArena* arena = mem_arena_add(
        mem,
        sizeof(T) * capacity,
        sizeof(T) * max_capacity,
        alignment
    );
    queue->memory = (T *) arena->memory;

    queue->head = queue->memory;
    queue->tail = queue->memory;
}

template <typename T>
FORCE_INLINE
void queue_init(QueueT<T>* const queue, BufferMemory* const buf, int capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    queue->capacity = capacity;
    queue->memory = (T *) buffer_get_memory(buf, sizeof(T) * capacity, alignment);
    queue->head = queue->memory;
    queue->tail = queue->memory;
}

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
void thrd_queue_alloc(QueueT<T>* const queue, MemoryArena* mem, uint32 capacity, uint32 max_capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
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
void queue_free(QueueT<T>* const queue, MemoryArena* mem) NO_EXCEPT
{
    mem_arena_remove(mem, queue->memory);
}

template <typename T>
inline
void thrd_queue_free(QueueT<T>* const queue, MemoryArena* mem) NO_EXCEPT
{
    queue_free(queue, mem);
    thrd_queue_locks_free(queue);
}

template <typename T>
FORCE_INLINE
bool queue_is_empty(const QueueT<T>* const queue) NO_EXCEPT
{
    return queue->head == queue->tail;
}

template <typename T>
FORCE_INLINE
bool queue_is_empty_atomic(const QueueT<T>* const queue) NO_EXCEPT
{
    return atomic_get_relaxed((void **) &queue->head) == atomic_get_relaxed((void **) &queue->tail);
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
FORCE_INLINE
void queue_set_empty_atomic(QueueT<T>* const queue) NO_EXCEPT
{
    atomic_set_relaxed(&queue->head, queue->tail);
}

template <typename T>
static inline
bool queue_has_space(QueueT<T>* const queue) NO_EXCEPT
{
    return queue->tail - 1 != queue->head;
}

template <typename T>
static inline
bool queue_has_space_atomic(QueueT<T>* const queue) NO_EXCEPT
{
    return atomic_get_relaxed((void **) &queue->head) - 1 != atomic_get_relaxed((void **) &queue->tail);
}

template <typename T>
FORCE_INLINE
bool queue_is_full(QueueT<T>* const queue) NO_EXCEPT
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
FORCE_INLINE
bool queue_is_full_atomic(QueueT<T>* const queue) NO_EXCEPT
{
    return !queue_has_space_atomic(queue);
}

// @todo implement enqueue with pass by value
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

// Conditional Lock
// @todo implement enqueue with pass by value
template <typename T>
inline
void thrd_queue_enqueue(QueueT<T>* __restrict queue, const T* __restrict data) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    queue_enqueue(queue, data);

    coms_pthread_cond_signal(&queue->cond);
}

// @todo implement enqueue with pass by value
template <typename T>
inline
T* queue_enqueue_atomic(QueueT<T>* const __restrict queue, const T* __restrict data) NO_EXCEPT
{
    T* mem = atomic_fetch_increment_wrap_relaxed(
        &queue->head,
        queue->memory,
        queue->memory + queue->capacity
    );

    *mem = *data;
    DEBUG_MEMORY_WRITE((uintptr_t) mem, sizeof(T));

    return mem;
}

// @todo implement enqueue with pass by value
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

// @todo Create enqueue_unique and enqueue_unique_sem
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
T* queue_enqueue_start(QueueT<T>* const queue) NO_EXCEPT
{
    DEBUG_MEMORY_WRITE((uintptr_t) queue->head, sizeof(T));
    return queue->head;
}

template <typename T>
FORCE_INLINE
T* queue_enqueue_start_safe(QueueT<T>* const queue) NO_EXCEPT
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

template <typename T>
inline
bool thrd_queue_dequeue_atomic(QueueT<T>* const __restrict queue, T* __restrict data) NO_EXCEPT
{
    if (queue_is_empty_atomic(queue)) {
        return false;
    }

    T* mem = atomic_fetch_increment_wrap_relaxed(
        &queue->tail,
        queue->memory,
        queue->memory + queue->capacity
    );

    // @bug in a queue that fills very fast this could lead to overwriting this element befor it is copied
    *data = *mem;
    DEBUG_MEMORY_DELETE((uintptr_t) mem, sizeof(T));

    return true;
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
T* queue_dequeue_keep(QueueT<T>* const queue) NO_EXCEPT
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
/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_QUEUET_PERSISTENT_C
#define COMS_MEMORY_QUEUET_PERSISTENT_C

#include "PersistentQueueT.h"

FORCE_INLINE CONSTEXPR
size_t queue_persistent_size(size_t type_size, int max_capacity) NO_EXCEPT
{
    const size_t max_array_count = ceil_div(max_capacity, (int32) (sizeof(size_t) * 8));
    return max_capacity * type_size
        + sizeof(size_t) * max_array_count + alignof(size_t) // free
        + sizeof(size_t) * max_array_count + alignof(size_t); // complete
}

template <typename T>
FORCE_INLINE
void queue_alloc(PersistentQueueT<T>* const queue, int capacity, int max_capacity, int alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_QUEUE_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating QueueT");

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    const size_t memory_size = capacity * sizeof(T)
    + sizeof(size_t) * array_count + sizeof(size_t) // free
    + sizeof(size_t) * array_count + sizeof(size_t); // complete

    const size_t max_array_count = ceil_div(max_capacity, (int32) (sizeof(size_t) * 8));
    const size_t max_memory_size = max_capacity * sizeof(T)
        + sizeof(size_t) * max_array_count + alignof(size_t) // free
        + sizeof(size_t) * max_array_count + alignof(size_t); // complete

    queue->capacity = capacity;
    queue->memory = (T *) platform_alloc_aligned(
        memory_size,
        max_memory_size,
        alignment
    );
    queue->head = 0;
    queue->tail = 0;

    queue->free = (size_t *) align_up(
        (size_t) ((uintptr_t) (queue->memory + capacity)),
        sizeof(size_t)
    );
    memset(queue->free, 0, sizeof(size_t) * array_count);

    queue->completed = (size_t *) align_up(
        (size_t) ((uintptr_t) (queue->free + array_count)),
        sizeof(size_t)
    );
    memset(queue->completed, 0, sizeof(size_t) * array_count);
}

template <typename T>
FORCE_INLINE
void queue_alloc(
    PersistentQueueT<T>* const queue,
    MemoryArena* const mem,
    int capacity, int max_capacity,
    uint32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ASSERT_TRUE(max_capacity >= capacity);

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    const size_t memory_size = queue->capacity * sizeof(T)
    + sizeof(size_t) * array_count + sizeof(size_t) // free
    + sizeof(size_t) * array_count + sizeof(size_t); // complete

    const size_t max_array_count = ceil_div(max_capacity, (int32) (sizeof(size_t) * 8));
    const size_t max_memory_size = max_capacity * sizeof(T)
        + sizeof(size_t) * max_array_count + alignof(size_t) // free
        + sizeof(size_t) * max_array_count + alignof(size_t); // complete

    queue->capacity = capacity;
    queue->memory = (T *) mem_arena_add(
        mem,
        memory_size,
        max_memory_size,
        alignment
    );
    queue->head = 0;
    queue->tail = 0;

    queue->free = (size_t *) align_up(
        (size_t) ((uintptr_t) (queue->memory + capacity)),
        alignof(size_t)
    );
    memset(queue->free, 0, sizeof(size_t) * array_count);

    queue->completed = (size_t *) align_up(
        (size_t) ((uintptr_t) (queue->free + array_count)),
        alignof(size_t)
    );
    memset(queue->completed, 0, sizeof(size_t) * array_count);
}

template <typename T>
FORCE_INLINE
void queue_init(PersistentQueueT<T>* const queue, BufferMemory* const buf, int capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));

    queue->capacity = capacity;
    queue->memory = (T *) memory_get(
        buf,
        sizeof(T) * capacity
        + sizeof(size_t) * array_count + alignof(size_t) // free
        + sizeof(size_t) * array_count + alignof(size_t), // complete
        alignment
    );
    queue->head = 0;
    queue->tail = 0;

    queue->free = (size_t *) align_up(
        (size_t) ((uintptr_t) (queue->memory + capacity)),
        alignof(size_t)
    );
    memset(queue->free, 0, sizeof(size_t) * array_count);

    queue->completed = (size_t *) align_up(
        (size_t) ((uintptr_t) (queue->free + array_count)),
        alignof(size_t)
    );
    memset(queue->completed, 0, sizeof(size_t) * array_count);
}

template <typename T>
FORCE_INLINE
void queue_init(PersistentQueueT<T>* const queue, byte* buf, int capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));

    queue->capacity = capacity;
    queue->memory = (T *) align_up((uintptr_t) buf, alignment);
    queue->head = 0;
    queue->tail = 0;

    queue->free = (size_t *) align_up(
        (size_t) ((uintptr_t) (queue->memory + capacity)),
        alignof(size_t)
    );
    memset(queue->free, 0, sizeof(size_t) * array_count);

    queue->completed = (size_t *) align_up(
        (size_t) ((uintptr_t) (queue->free + array_count)),
        alignof(size_t)
    );
    memset(queue->completed, 0, sizeof(size_t) * array_count);
}

template <typename T>
static inline
void thrd_queue_locks_init(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    mutex_init(&queue->lock, NULL);
    coms_pthread_cond_init(&queue->cond, NULL);

    coms_sem_init(&queue->empty, queue->capacity);
    coms_sem_init(&queue->full, 0);
}

template <typename T>
inline
void thrd_queue_alloc(PersistentQueueT<T>* const queue, uint32 capacity, uint32 max_capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    queue_alloc(queue, capacity, max_capacity, alignment);
    thrd_queue_locks_init(queue);
}

template <typename T>
inline
void thrd_queue_alloc(PersistentQueueT<T>* const queue, MemoryArena* mem, uint32 capacity, uint32 max_capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    queue_alloc(queue, mem, capacity, max_capacity, alignment);
    thrd_queue_locks_init(queue);
}

template <typename T>
inline
void thrd_queue_init(PersistentQueueT<T>* const queue, BufferMemory* const buf, uint32 capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    queue_init(queue, buf, capacity, alignment);
    thrd_queue_locks_init(queue);
}

template <typename T>
inline
void thrd_queue_init(PersistentQueueT<T>* const queue, byte* buf, uint32 capacity, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    queue_init(queue, buf, capacity, alignment);
    thrd_queue_locks_init(queue);
}

template <typename T>
FORCE_INLINE
void queue_free(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    platform_aligned_free((void **) &queue->memory);
}

template <typename T>
static inline
void thrd_queue_locks_free(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    coms_sem_destroy(&queue->empty);
    coms_sem_destroy(&queue->full);
    mutex_destroy(&queue->lock);
    coms_pthread_cond_destroy(&queue->cond);
}

template <typename T>
inline
void thrd_queue_free(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    queue_free(queue);
    thrd_queue_locks_free(queue);
}

template <typename T>
FORCE_INLINE
void queue_free(PersistentQueueT<T>* const queue, MemoryArena* mem) NO_EXCEPT
{
    mem_arena_remove(mem, (byte *) queue->memory);
}

template <typename T>
inline
void thrd_queue_free(PersistentQueueT<T>* const queue, MemoryArena* mem) NO_EXCEPT
{
    queue_free(queue, mem);
    thrd_queue_locks_free(queue);
}

template <typename T>
inline
bool memory_resize(PersistentQueueT<T>* const queue, uint32 new_capacity) NO_EXCEPT
{
    const uint32 old_capacity = queue->capacity;
    if (new_capacity == old_capacity) {
        return true;
    }

    const uint32 head_index = queue->head;
    const uint32 tail_index = queue->tail;

    // Elements still "in play" (occupied — whether awaiting dequeue or
    // completed-but-retained) that must survive the resize.
    const uint32 count = (head_index >= tail_index)
        ? (head_index - tail_index)
        : (old_capacity - tail_index + head_index);

    if (new_capacity < count) {
        return false;
    }

    const bool wrapped = tail_index > head_index;
    const size_t old_array_count = ceil_div(old_capacity, (uint32) (sizeof(size_t) * 8));

    // The live region only stays physically valid across a capacity change
    // if it already starts at index 0 and doesn't wrap. Otherwise the
    // mod-capacity math changes underneath it and corrupts ordering — so
    // relocate T data + both bitmaps together to start at 0 first.
    const bool needs_linearize = count > 0 && (wrapped || tail_index != 0);

    if (needs_linearize) {
        T* temp_data = (T*) platform_alloc_aligned(sizeof(T) * count);
        size_t* temp_free = (size_t*) platform_alloc_aligned(sizeof(size_t) * old_array_count);
        size_t* temp_completed = (size_t*) platform_alloc_aligned(sizeof(size_t) * old_array_count);

        if (!temp_data || !temp_free || !temp_completed) {
            platform_free_aligned(temp_data);
            platform_free_aligned(temp_free);
            platform_free_aligned(temp_completed);

            return false;
        }

        memset(temp_free, 0, sizeof(size_t) * old_array_count);
        memset(temp_completed, 0, sizeof(size_t) * old_array_count);

        for (uint32 i = 0; i < count; ++i) {
            const uint32 src = (tail_index + i) % old_capacity;
            temp_data[i] = queue->memory[src];
            OMS_BITARRAY_SET_TO(temp_free, i, OMS_BITARRAY_CHECK(queue->free, src));
            OMS_BITARRAY_SET_TO(temp_completed, i, OMS_BITARRAY_CHECK(queue->completed, src));
        }

        memcpy(queue->memory, temp_data, sizeof(T) * count);
        memcpy(queue->free, temp_free, sizeof(size_t) * old_array_count);
        memcpy(queue->completed, temp_completed, sizeof(size_t) * old_array_count);

        platform_free_aligned(temp_data);
        platform_free_aligned(temp_free);
        platform_free_aligned(temp_completed);
    }

    const uint32 lin_head = needs_linearize ? count : head_index;
    const uint32 lin_tail = needs_linearize ? 0 : tail_index;

    const size_t new_array_count = ceil_div(new_capacity, (uint32) (sizeof(size_t) * 8));
    const size_t memory_size = new_capacity * sizeof(T)
        + sizeof(size_t) * new_array_count + sizeof(size_t) // free
        + sizeof(size_t) * new_array_count + sizeof(size_t); // complete

    if (!platform_alloc_aligned_grow(queue->memory, memory_size)) {
        return false;
    }

    size_t* const old_free = queue->free;
    size_t* const old_completed = queue->completed;

    size_t* const new_free = (size_t*) align_up(
        (uintptr_t) (queue->memory + new_capacity),
        sizeof(size_t)
    );
    size_t* const new_completed = (size_t*) align_up(
        (uintptr_t) (new_free + new_array_count),
        sizeof(size_t)
    );

    const size_t common_array_count = old_array_count < new_array_count ? old_array_count : new_array_count;
    const bool growing = new_capacity > old_capacity;

    if (growing) {
        memmove(new_completed, old_completed, sizeof(size_t) * common_array_count);
        memmove(new_free, old_free, sizeof(size_t) * common_array_count);

        memset(new_free + old_array_count, 0, sizeof(size_t) * (new_array_count - old_array_count));
        memset(new_completed + old_array_count, 0, sizeof(size_t) * (new_array_count - old_array_count));
    } else {
        memmove(new_free, old_free, sizeof(size_t) * common_array_count);
        memmove(new_completed, old_completed, sizeof(size_t) * common_array_count);

        const uint32 valid_bits = new_capacity % (sizeof(size_t) * 8);
        if (valid_bits != 0 && new_array_count > 0) {
            const size_t mask = ((size_t) 1 << valid_bits) - 1;
            new_free[new_array_count - 1] &= mask;
            new_completed[new_array_count - 1] &= mask;
        }
    }

    queue->free = new_free;
    queue->completed = new_completed;
    queue->capacity = new_capacity;

    queue->tail = lin_tail;
    queue->head = lin_head;

    if (!growing) {
        if (!platform_alloc_aligned_shrink(queue->memory, memory_size)) {
            return false;
        }
    }

    return true;
}

template <typename T>
inline
bool thrd_memory_resize(PersistentQueueT<T>* const queue, uint32 new_capacity) NO_EXCEPT
{
    MutexGuard _guard(&queue->mtx);
    return memory_resize(queue, new_capacity);
}

template <typename T>
FORCE_INLINE
bool queue_is_empty(const PersistentQueueT<T>* const queue) NO_EXCEPT
{
    return queue->head == queue->tail;
}

template <typename T>
FORCE_INLINE
bool queue_is_empty_atomic(const PersistentQueueT<T>* const queue) NO_EXCEPT
{
    return atomic_get_relaxed(&queue->head) == atomic_get_relaxed(&queue->tail);
}

template <typename T>
FORCE_INLINE
bool thrd_queue_is_empty(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    MutexGuard _guard(&queue->lock);
    return queue_is_empty(queue);
}

template <typename T>
FORCE_INLINE
void queue_set_empty(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    queue->head = queue->tail;
}

template <typename T>
FORCE_INLINE
void queue_set_empty_atomic(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    atomic_set_relaxed(&queue->head, queue->tail);
}

template <typename T>
static inline
bool queue_has_space(const PersistentQueueT<T>* const queue) NO_EXCEPT
{
    return queue->tail - 1 != queue->head;
}

template <typename T>
static inline
bool queue_has_space_atomic(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    return atomic_get_relaxed(&queue->head) - 1 != atomic_get_relaxed(&queue->tail);
}

template <typename T>
FORCE_INLINE
bool queue_is_full(const PersistentQueueT<T>* const queue) NO_EXCEPT
{
    return !queue_has_space(queue);
}

template <typename T>
inline
bool thrd_queue_is_full(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    MutexGuard _guard(&queue->lock);
    return !queue_has_space(queue);
}

template <typename T>
FORCE_INLINE
bool queue_is_full_atomic(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    return !queue_has_space_atomic(queue);
}

template <typename T>
FORCE_INLINE
size_t* chunk_find_free_array(const PersistentQueueT<T>* const queue) NO_EXCEPT
{
    return (size_t *) align_up(
        (uintptr_t) (queue->memory + queue->capacity),
        (size_t) sizeof(size_t)
    );
}

template <typename T>
inline
T* queue_enqueue(PersistentQueueT<T>* const __restrict queue, const T* __restrict data) NO_EXCEPT
{
    // In a normal queue we would only go until tail BUT the tail might be VERY far behind in a persistent queue
    uint32 old_head = queue->head;
    while (queue->head != old_head - 1
        && !chunk_is_free_internal(queue->free, queue->head)
    ) {
        OMS_WRAPPED_INCREMENT(
            queue->head,
            queue->capacity
        );
    }

    if (queue->head != old_head - 1
        || !chunk_is_free_internal(queue->free, queue->head)
    ) {
        return NULL;
    }

    // @performance This is slow we are calculating free_index and bit_index also in the loop above
    //              Calculating it here again adds 1 such calculation as overhead
    const uint32 free_index = queue->head / (sizeof(size_t) * 8);
    const uint32 bit_index = MODULO_2(queue->head, (sizeof(size_t) * 8));
    queue->free[free_index] |= (OMS_UINT_ONE << bit_index);

    queue->memory[queue->head] = *data;
    T* mem = &queue->memory[queue->head];
    DEBUG_MEMORY_WRITE((uintptr_t) mem, sizeof(T));

    OMS_WRAPPED_INCREMENT(
        queue->head,
        queue->capacity
    );

    return mem;
}

template <typename T>
inline
T* queue_enqueue(PersistentQueueT<T>* const __restrict queue, const T data) NO_EXCEPT
{
    // In a normal queue we would only go until tail BUT the tail might be VERY far behind in a persistent queue
    uint32 old_head = queue->head;
    while (queue->head != old_head - 1
        && !chunk_is_free_internal(queue->free, queue->head)
    ) {
        OMS_WRAPPED_INCREMENT(
            queue->head,
            queue->capacity
        );
    }

    if (queue->head != old_head - 1
        || !chunk_is_free_internal(queue->free, queue->head)
    ) {
        return NULL;
    }

    // @performance This is slow we are calculating free_index and bit_index also in the loop above
    //              Calculating it here again adds 1 such calculation as overhead
    const uint32 free_index = queue->head / (sizeof(size_t) * 8);
    const uint32 bit_index = MODULO_2(queue->head, (sizeof(size_t) * 8));
    queue->free[free_index] |= (OMS_UINT_ONE << bit_index);

    queue->memory[queue->head] = data;
    T* mem = &queue->memory[queue->head];
    DEBUG_MEMORY_WRITE((uintptr_t) mem, sizeof(T));

    OMS_WRAPPED_INCREMENT(
        queue->head,
        queue->capacity
    );

    return mem;
}

// Conditional Lock
template <typename T>
inline
T* thrd_queue_enqueue(PersistentQueueT<T>* __restrict queue, const T* __restrict data) NO_EXCEPT
{
    MutexGuard _guard(&queue->lock);
    T* mem = queue_enqueue(queue, data);

    coms_pthread_cond_signal(&queue->cond);

    return mem;
}

template <typename T>
inline
T* thrd_queue_enqueue(PersistentQueueT<T>* __restrict queue, const T data) NO_EXCEPT
{
    MutexGuard _guard(&queue->lock);
    T* mem = queue_enqueue(queue, data);

    coms_pthread_cond_signal(&queue->cond);

    return mem;
}

template <typename T>
inline
T* queue_enqueue_atomic(PersistentQueueT<T>* const __restrict queue, const T* __restrict data) NO_EXCEPT
{
    uint32 index;
    do {
        index = atomic_fetch_increment_wrap_relaxed(
            &queue->head,
            queue->capacity
        );
    } while (!chunk_is_free_internal(queue->free, queue->head) && queue->head != queue->tail - 1);

    // @performance This is slow we are calculating free_index and bit_index also in the loop above
    //              Calculating it here again adds 1 such calculation as overhead
    const uint32 free_index = index / (sizeof(size_t) * 8);
    const uint32 bit_index = MODULO_2(index, (sizeof(size_t) * 8));

    atomic_or_release(&queue->free[free_index], (OMS_UINT_ONE << bit_index));

    queue->memory[index] = *data;
    T* mem = &queue->memory[index];
    DEBUG_MEMORY_WRITE((uintptr_t) mem, sizeof(T));

    return mem;
}

template <typename T>
inline
T* queue_enqueue_atomic(PersistentQueueT<T>* const __restrict queue, const T data) NO_EXCEPT
{
    uint32 index;
    do {
        index = atomic_fetch_increment_wrap_relaxed(
            &queue->head,
            queue->capacity
        );
    } while (!chunk_is_free_internal(queue->free, queue->head) && queue->head != queue->tail - 1);

    // @performance This is slow we are calculating free_index and bit_index also in the loop above
    //              Calculating it here again adds 1 such calculation as overhead
    const uint32 free_index = index / (sizeof(size_t) * 8);
    const uint32 bit_index = MODULO_2(index, (sizeof(size_t) * 8));

    atomic_or_release(&queue->free[free_index], (OMS_UINT_ONE << bit_index));

    queue->memory[index] = data;
    T* mem = &queue->memory[index];
    DEBUG_MEMORY_WRITE((uintptr_t) mem, sizeof(T));

    return mem;
}

template <typename T>
inline
T* queue_enqueue_safe(PersistentQueueT<T>* const __restrict queue, const T* __restrict data) NO_EXCEPT
{
    if(!queue_has_space(queue)) {
        return NULL;
    }

    return queue_enqueue(queue, data);
}

template <typename T>
inline
T* queue_enqueue_safe(PersistentQueueT<T>* const __restrict queue, const T data) NO_EXCEPT
{
    if(!queue_has_space(queue)) {
        return NULL;
    }

    return queue_enqueue(queue, data);
}

template <typename T>
inline
T* thrd_queue_enqueue_safe(PersistentQueueT<T>* const __restrict queue, const T* __restrict data) NO_EXCEPT
{
    MutexGuard _guard(&queue->lock);
    if(!queue_has_space(queue)) {
        return NULL;
    }

    return queue_enqueue(queue, data);
}

template <typename T>
inline
T* thrd_queue_enqueue_safe(PersistentQueueT<T>* const __restrict queue, const T data) NO_EXCEPT
{
    MutexGuard _guard(&queue->lock);
    if(!queue_has_space(queue)) {
        return NULL;
    }

    return queue_enqueue(queue, data);
}

template <typename T>
inline
void queue_enqueue_unique(PersistentQueueT<T>* const __restrict queue, const T* __restrict data) NO_EXCEPT
{
    uint32 tail = queue->tail;
    while (tail != queue->head) {
        if (!chunk_is_free_internal(queue->free, tail)
            && memcmp(&queue->memory[tail], data, sizeof(T)) == 0
        ) {
            return;
        }

        OMS_WRAPPED_INCREMENT(
            tail,
            queue->capacity
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
void queue_enqueue_unique(PersistentQueueT<T>* const __restrict queue, const T data) NO_EXCEPT
{
    uint32 tail = queue->tail;
    while (tail != queue->head) {
        if (!chunk_is_free_internal(queue->free, tail)
            && memcmp(&queue->memory[tail], data, sizeof(T)) == 0
        ) {
            return;
        }

        OMS_WRAPPED_INCREMENT(
            tail,
            queue->capacity
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
void thrd_queue_enqueue_unique(PersistentQueueT<T>* __restrict queue, const T* __restrict data) NO_EXCEPT
{
    ASSERT_TRUE((uint64_t) data % 4 == 0);
    MutexGuard _guard(&queue->lock);

    queue_enqueue_unique(queue, data);

    coms_pthread_cond_signal(&queue->cond);
}

template <typename T>
inline
void thrd_queue_enqueue_unique(PersistentQueueT<T>* __restrict queue, const T data) NO_EXCEPT
{
    MutexGuard _guard(&queue->lock);

    queue_enqueue_unique(queue, data);

    coms_pthread_cond_signal(&queue->cond);
}

template <typename T>
inline
void thrd_queue_enqueue_unique_wait(PersistentQueueT<T>* __restrict queue, const T* __restrict data) NO_EXCEPT
{
    ASSERT_TRUE((uint64_t) data % 4 == 0);
    MutexGuard _guard(&queue->lock);

    uint32 tail = queue->tail;
    while (tail != queue->head) {
        if (!chunk_is_free_internal(queue->free, tail)
            && memcmp(&queue->memory[tail], data, sizeof(T)) == 0
        ) {
            return;
        }

        OMS_WRAPPED_INCREMENT(
            tail,
            queue->capacity
        );
    }

    while (!queue_enqueue_safe(queue, data)) {
        coms_pthread_cond_wait(&queue->cond, &queue->lock);
    }

    coms_pthread_cond_signal(&queue->cond);
}

template <typename T>
inline
void thrd_queue_enqueue_unique_wait(PersistentQueueT<T>* __restrict queue, const T data) NO_EXCEPT
{
    MutexGuard _guard(&queue->lock);

    uint32 tail = queue->tail;
    while (tail != queue->head) {
        if (!chunk_is_free_internal(queue->free, tail)
            && memcmp(&queue->memory[tail], data, sizeof(T)) == 0
        ) {
            return;
        }

        OMS_WRAPPED_INCREMENT(
            tail,
            queue->capacity
        );
    }

    while (!queue_enqueue_safe(queue, data)) {
        coms_pthread_cond_wait(&queue->cond, &queue->lock);
    }

    coms_pthread_cond_signal(&queue->cond);
}

template <typename T>
inline
T* queue_enqueue_start(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    // In a normal queue we would only go until tail BUT the tail might be VERY far behind in a persistent queue
    uint32 old_head = queue->head;
    while (!chunk_is_free_internal(queue->free, queue->head) && queue->head != old_head - 1) {
        OMS_WRAPPED_INCREMENT(
            queue->head,
            queue->capacity
        );
    }

    // @question could this part be considered the _safe part for the next function?
    if (!chunk_is_free_internal(queue->free, queue->head)) {
        return NULL;
    }

    DEBUG_MEMORY_WRITE((uintptr_t) &queue->memory[queue->head], sizeof(T));

    return &queue->memory[queue->head];
}

template <typename T>
FORCE_INLINE
T* queue_enqueue_start_safe(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    // In a persistent queue both safe and unsafe are the same?
    return queue_enqueue_start(queue);
}

template <typename T>
inline
void thrd_queue_enqueue_wait(PersistentQueueT<T>* __restrict queue, const T* __restrict data) NO_EXCEPT
{
    MutexGuard _guard(&queue->lock);

    while (!queue_enqueue_safe(queue, data)) {
        coms_pthread_cond_wait(&queue->cond, &queue->lock);
    }

    coms_pthread_cond_signal(&queue->cond);
}

template <typename T>
inline
void thrd_queue_enqueue_wait(PersistentQueueT<T>* __restrict queue, const T data) NO_EXCEPT
{
    MutexGuard _guard(&queue->lock);

    while (!queue_enqueue_safe(queue, data)) {
        coms_pthread_cond_wait(&queue->cond, &queue->lock);
    }

    coms_pthread_cond_signal(&queue->cond);
}

template <typename T>
inline
void thrd_queue_enqueue_sem_wait(PersistentQueueT<T>* __restrict queue, const T* __restrict data) NO_EXCEPT
{
    coms_sem_wait(&queue->empty);
    mutex_lock(&queue->lock);

    queue_enqueue(queue, data);

    mutex_unlock(&queue->lock);
    coms_sem_post(&queue->full);
}

template <typename T>
inline
void thrd_queue_enqueue_sem_wait(PersistentQueueT<T>* __restrict queue, const T data) NO_EXCEPT
{
    coms_sem_wait(&queue->empty);
    mutex_lock(&queue->lock);

    queue_enqueue(queue, data);

    mutex_unlock(&queue->lock);
    coms_sem_post(&queue->full);
}

template <typename T>
inline
bool thrd_queue_enqueue_semimedwait(PersistentQueueT<T>* __restrict queue, const T* __restrict data, uint64 wait) NO_EXCEPT
{
    if (semimedwait(&queue->empty, wait)) {
        return false;
    }

    {
        MutexGuard _guard(&queue->lock);
        queue_enqueue(queue, data);
    }

    coms_sem_post(&queue->full);

    return true;
}

template <typename T>
inline
bool thrd_queue_enqueue_semimedwait(PersistentQueueT<T>* __restrict queue, const T data, uint64 wait) NO_EXCEPT
{
    if (semimedwait(&queue->empty, wait)) {
        return false;
    }

    {
        MutexGuard _guard(&queue->lock);
        queue_enqueue(queue, data);
    }

    coms_sem_post(&queue->full);

    return true;
}

template <typename T>
inline
T* thrd_queue_enqueue_start_wait(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    mutex_lock(&queue->lock);

    // @bug I don't think this is enough for a PersistentQueue
    //      We also need to check free
    while (!queue_has_space(queue)) {
        coms_pthread_cond_wait(&queue->cond, &queue->lock);
    }

    return queue_enqueue_start(queue);
}

template <typename T>
FORCE_INLINE
T* thrd_queue_enqueue_start_sem_wait(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    coms_sem_wait(&queue->empty);
    mutex_lock(&queue->lock);

    return queue->head;
}

template <typename T>
FORCE_INLINE
void queue_enqueue_end(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    const uint32 free_index = queue->head / (sizeof(size_t) * 8);
    const uint32 bit_index = MODULO_2(queue->head, (sizeof(size_t) * 8));
    queue->free[free_index] |= (OMS_UINT_ONE << bit_index);

    OMS_WRAPPED_INCREMENT(
        queue->head,
        queue->capacity
    );
}

template <typename T>
FORCE_INLINE
void thrd_queue_enqueue_end_wait(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    queue_enqueue_end(queue);
    coms_pthread_cond_signal(&queue->cond);

    mutex_unlock(&queue->lock);
}

template <typename T>
FORCE_INLINE
void thrd_queue_enqueue_end_sem_wait(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    mutex_unlock(&queue->lock);
    coms_sem_post(&queue->full);
}

template <typename T>
inline
bool queue_dequeue(PersistentQueueT<T>* const __restrict queue, T* __restrict data) NO_EXCEPT
{
    // This part is special for this type of queue
    // We can't just use the tail index
    // We have to check if it is still stored AND if it isn't yet completed
    // If we say _keep, then completeness is 1 -> we don't want to dequeue it again
    // The element already got processed and just awaits manual releasing from somewhere

    // Some of the code below basically replaces chunk_is_free_internal
    // By doing this we can reduce the repetition of calculating free_index/bit_index
    uint32 free_index = queue->tail / (sizeof(size_t) * 8);
    uint32 bit_index = MODULO_2(queue->tail, (sizeof(size_t) * 8));

    while (queue->head != queue->tail
        && (!IS_BIT_SET_R2L(queue->free[free_index], bit_index)
        || IS_BIT_SET_R2L(queue->completed[free_index], bit_index))
    ) {
        OMS_WRAPPED_INCREMENT(
            queue->tail,
            queue->capacity
        );
    }

    if (queue->head == queue->tail) {
        return false;
    }

    *data = queue->memory[queue->tail];
    DEBUG_MEMORY_DELETE((uintptr_t) queue->memory[queue->tail], sizeof(T));

    free_index = queue->tail / (sizeof(size_t) * 8);
    bit_index = MODULO_2(queue->tail, (sizeof(size_t) * 8));

    queue->free[free_index] &= ~(OMS_UINT_ONE << bit_index);

    // We don't need to set or unset completed here
    // That variable only needs to be modified when using dequeue_keep and dequeue_release

    OMS_WRAPPED_INCREMENT(
        queue->tail,
        queue->capacity
    );

    return true;
}

template <typename T>
inline
bool thrd_queue_dequeue(PersistentQueueT<T>* __restrict queue, T* __restrict data) NO_EXCEPT
{
    if (queue_is_empty(queue)) {
        return false;
    }

    // we do this twice because the first one is very fast but may return a false positive
    MutexGuard _guard(&queue->lock);
    bool result = queue_dequeue(queue, data);

    coms_pthread_cond_signal(&queue->cond);

    return result;
}

// thrd_queue_dequeue_atomic Seems to make no sense due to the amount of operations
// tail, head, free, completed all need to be checked

// Waits until a dequeue is available
template <typename T>
inline
void thrd_queue_dequeue_wait(PersistentQueueT<T>* __restrict queue, T* __restrict data) NO_EXCEPT
{
    MutexGuard _guard(&queue->lock);

    while (queue_is_empty(queue)) {
        coms_pthread_cond_wait(&queue->cond, &queue->lock);
    }

    queue_dequeue(queue, data);

    coms_pthread_cond_signal(&queue->cond);
}

template <typename T>
inline
T* thrd_queue_dequeue_sem_wait(PersistentQueueT<T>* __restrict queue, T* __restrict data) NO_EXCEPT
{
    coms_sem_wait(&queue->full);

    {
        MutexGuard _guard(&queue->lock);
        queue_dequeue(queue, data);
    }

    coms_sem_post(&queue->empty);
}

template <typename T>
inline
bool thrd_queue_dequeue_semimedwait(PersistentQueueT<T>* __restrict queue, T* __restrict data, uint64 wait) NO_EXCEPT
{
    if (semimedwait(&queue->full, wait)) {
        return false;
    }

    {
        MutexGuard _guard(&queue->lock);
        queue_dequeue(queue, data);
    }

    coms_sem_post(&queue->empty);

    return true;
}

template <typename T>
inline
T* queue_dequeue_keep(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    // This part is special for this type of queue
    // We can't just use the tail index
    // We have to check if it is still stored AND if it isn't yet completed
    // If we say _keep, then completeness is 1 -> we don't want to dequeue it again
    // The element already got processed and just awaits manual releasing from somewhere

    // Some of the code below basically replaces chunk_is_free_internal
    // By doing this we can reduce the repetition of calculating free_index/bit_index
    uint32 free_index = queue->tail / (sizeof(size_t) * 8);
    uint32 bit_index = MODULO_2(queue->tail, (sizeof(size_t) * 8));

    while (queue->head != queue->tail
        && (!IS_BIT_SET_R2L(queue->free[free_index], bit_index)
        || IS_BIT_SET_R2L(queue->completed[free_index], bit_index))
    ) {
        OMS_WRAPPED_INCREMENT(
            queue->tail,
            queue->capacity
        );
    }

    if (queue->head == queue->tail) {
        return NULL;
    }

    free_index = queue->tail / (sizeof(size_t) * 8);
    bit_index = MODULO_2(queue->tail, (sizeof(size_t) * 8));

    // We don't mark the element as free since it still needs to keep the data stored
    // But we mark it as already handled/completed
    queue->completed[free_index] |= (OMS_UINT_ONE << bit_index);

    T* mem = &queue->memory[queue->tail];

    OMS_WRAPPED_INCREMENT(
        queue->tail,
        queue->capacity
    );

    return mem;
}

template <typename T>
inline
T* thrd_queue_dequeue_keep(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    MutexGuard _guard(&queue->lock);
    return queue_dequeue_keep(queue);
}

template <typename T>
FORCE_INLINE
void queue_dequeue_release(PersistentQueueT<T>* const queue, uint32 index) NO_EXCEPT
{
    const uint32 free_index = index / (sizeof(size_t) * 8);
    const uint32 bit_index = MODULO_2(index, (sizeof(size_t) * 8));

    queue->free[free_index] &= ~(OMS_UINT_ONE << bit_index);
    queue->completed[free_index] &= ~(OMS_UINT_ONE << bit_index);
    DEBUG_MEMORY_DELETE((uintptr_t) &queue->memory[index], sizeof(T));

    // This doesn't move the tail index since the tail index could be already somewhere entirely different
    // And for that reason we already moved it immediately when calling _keep()
}

template <typename T>
FORCE_INLINE
void queue_dequeue_release(PersistentQueueT<T>* const queue, const T* element) NO_EXCEPT
{
    const uint32 index = (uint32) (((uintptr_t) element - (uintptr_t) queue->memory) / sizeof(T));
    queue_dequeue_release(queue, index);
}

template <typename T>
FORCE_INLINE
void thrd_queue_dequeue_release(PersistentQueueT<T>* const queue, const T* element) NO_EXCEPT
{
    MutexGuard _guard(&queue->lock);
    queue_dequeue_release(queue, element);
}

template <typename T>
FORCE_INLINE
void thrd_queue_dequeue_release(PersistentQueueT<T>* const queue, uint32 index) NO_EXCEPT
{
    MutexGuard _guard(&queue->lock);
    queue_dequeue_release(queue, index);
}

template <typename T>
FORCE_INLINE
void queue_uncomplete(PersistentQueueT<T>* const queue, uint32 index) NO_EXCEPT
{
    const uint32 free_index = index / (sizeof(size_t) * 8);
    const uint32 bit_index = MODULO_2(index, (sizeof(size_t) * 8));

    queue->completed[free_index] &= ~(OMS_UINT_ONE << bit_index);
}

template <typename T>
FORCE_INLINE
void queue_uncomplete(PersistentQueueT<T>* const queue, T* element) NO_EXCEPT
{
    const uint32 index = (uint32) (((uintptr_t) element - (uintptr_t) queue->memory) / sizeof(T));
    queue_uncomplete(queue, index);
}

template <typename T>
FORCE_INLINE
void thrd_queue_uncomplete(PersistentQueueT<T>* const queue, T* element) NO_EXCEPT
{
    MutexGuard _guard(&queue->lock);
    queue_uncomplete(queue, element);
}

template <typename T>
FORCE_INLINE
void queue_uncomplete_atomic(PersistentQueueT<T>* const queue, T* element) NO_EXCEPT
{
    const uint32 index = ((uintptr_t) element - (uintptr_t) queue->memory) / sizeof(T);
    const uint32 free_index = index / (sizeof(size_t) * 8);
    const uint32 bit_index = MODULO_2(index, (sizeof(size_t) * 8));

    atomic_and_release(&queue->completed[free_index], ~(OMS_UINT_ONE << bit_index));
}

template <typename T>
FORCE_INLINE
void queue_dequeue_release_atomic(PersistentQueueT<T>* const queue, T* element) NO_EXCEPT
{
    const uint32 index = ((uintptr_t) element - (uintptr_t) queue->memory) / sizeof(T);
    const uint32 free_index = index / (sizeof(size_t) * 8);
    const uint32 bit_index = MODULO_2(index, (sizeof(size_t) * 8));

    atomic_and_release(&queue->free[free_index], ~(OMS_UINT_ONE << bit_index));
    atomic_and_release(&queue->completed[free_index], ~(OMS_UINT_ONE << bit_index));
    DEBUG_MEMORY_DELETE((uintptr_t) element, sizeof(T));

    // This doesn't move the tail index since the tail index could be already somewhere entirely different
    // And for that reason we already moved it immediately when calling _keep()
}

template <typename T>
inline
T* queue_dequeue_start(const PersistentQueueT<T>* const queue) NO_EXCEPT
{
    // Some of the code below basically replaces chunk_is_free_internal
    // By doing this we can reduce the repetition of calculating free_index/bit_index
    const uint32 free_index = queue->tail / (sizeof(size_t) * 8);
    const uint32 bit_index = MODULO_2(queue->tail, (sizeof(size_t) * 8));

    while (queue->tail != queue->head
        && (!IS_BIT_SET_R2L(queue->free[free_index], bit_index)
        || IS_BIT_SET_R2L(queue->completed[free_index], bit_index))
    ) {
        OMS_WRAPPED_INCREMENT(
            queue->tail,
            queue->capacity
        );
    }

    if (queue->head == queue->tail) {
        return NULL;
    }

    queue->completed[free_index] |= (OMS_UINT_ONE << bit_index);

    return queue->tail;
}

template <typename T>
FORCE_INLINE
T* thrd_queue_dequeue_start_wait(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    mutex_lock(&queue->lock);

    while (queue_is_empty(queue)) {
        coms_pthread_cond_wait(&queue->cond, &queue->lock);
    }

    return queue_dequeue_start(queue);
}

template <typename T>
FORCE_INLINE
T* thrd_queue_dequeue_start_sem_wait(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    coms_sem_wait(&queue->full);
    mutex_lock(&queue->lock);

    return queue_dequeue_start(queue);
}

template <typename T>
FORCE_INLINE
void queue_dequeue_end(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    const uint32 free_index = queue->tail / (sizeof(size_t) * 8);
    const uint32 bit_index = MODULO_2(queue->tail, (sizeof(size_t) * 8));

    queue->free[free_index] &= ~(OMS_UINT_ONE << bit_index);
    queue->completed[free_index] &= ~(OMS_UINT_ONE << bit_index);

    DEBUG_MEMORY_DELETE((uintptr_t) queue->tail, sizeof(T));

    OMS_WRAPPED_INCREMENT(
        queue->tail,
        queue->capacity
    );
}

template <typename T>
FORCE_INLINE
void thrd_queue_dequeue_end_wait(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    queue_dequeue_end(queue);

    coms_pthread_cond_signal(&queue->cond);
    mutex_unlock(&queue->lock);
}

template <typename T>
FORCE_INLINE
void thrd_queue_dequeue_end_sem_wait(PersistentQueueT<T>* const queue) NO_EXCEPT
{
    queue_dequeue_end(queue);

    mutex_unlock(&queue->lock);
    coms_sem_post(&queue->empty);
}

#endif
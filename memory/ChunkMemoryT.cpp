/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_CHUNK_MEMORYT_C
#define COMS_MEMORY_CHUNK_MEMORYT_C

#include "ChunkMemoryT.h"
#include "../utils/BitUtils.h"
#include "ChunkMemory.cpp"
#include "BufferMemory.cpp"
#include "../system/Allocator.h"
#include "../log/Log.h"
#include "../log/Stats.h"
#include "../log/PerformanceProfiler.h"
#include "../log/DebugMemory.h"

CONSTEXPR FORCE_INLINE
size_t chunk_size(size_t type_size, int max_capacity) NO_EXCEPT
{
    const size_t array_count = ceil_div(max_capacity, (int32) (sizeof(size_t) * 8));

    return max_capacity * type_size
        + sizeof(size_t) * array_count
        + sizeof(size_t);
}

// INFO: A chunk count of 2^n is recommended for maximum performance
template <typename T>
inline
void chunk_alloc(ChunkMemoryT<T>* const buf, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_CHUNK_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemoryT");

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    const size_t memory_size = capacity * sizeof(T) + sizeof(size_t) * array_count + alignof(size_t);

    const size_t max_array_count = ceil_div(max_capacity, (int32) (sizeof(size_t) * 8));
    const size_t max_memory_size = max_capacity * sizeof(T) + sizeof(size_t) * max_array_count + alignof(size_t);

    buf->memory = (T *) platform_alloc_aligned(
        memory_size,
        max_memory_size,
        alignment
    );

    buf->capacity = capacity;
    buf->last_pos = -1;
    buf->free = (size_t *) align_up(
        (size_t) ((uintptr_t) (buf->memory + capacity)),
        alignof(size_t)
    );
    memset(buf->free, 0, sizeof(size_t) * array_count);
}

template <typename T>
inline
void thrd_chunk_alloc(ChunkMemoryT<T>* const buf, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_CHUNK_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemoryT");

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    const size_t memory_size = capacity * sizeof(T)
            + sizeof(size_t) * array_count
            + sizeof(size_t) * array_count
            + sizeof(size_t) * 2;

    const size_t max_array_count = ceil_div(max_capacity, (int32) (sizeof(size_t) * 8));
    const size_t max_memory_size = max_capacity * sizeof(T)
            + sizeof(size_t) * max_array_count
            + sizeof(size_t) * max_array_count
            + sizeof(size_t) * 2;

    buf->memory = (T *) platform_alloc_aligned(
        memory_size,
        max_memory_size,
        alignment
    );

    buf->capacity = capacity;
    buf->last_pos = -1;
    buf->free = (size_t *) align_up(
        (size_t) ((uintptr_t) (buf->memory + capacity)),
        (size_t) alignof(size_t)
    );
    buf->completeness = (size_t *) align_up((uintptr_t) (buf->free + array_count), alignof(size_t));

    memset((void *) buf->free, 0, sizeof(size_t) * array_count);
    memset((void *) buf->completeness, 0, sizeof(size_t) * array_count);

    spinlock_init(&buf->lock);

    LOG_1("[INFO] Allocated ChunkMemoryT: %n", {DATA_TYPE_UINT64, &buf->capacity});
}

// INFO: A chunk count of 2^n is recommended for maximum performance
template <typename T>
inline
void chunk_alloc(ChunkMemoryT<T>* const buf, MemoryArena* const mem, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_CHUNK_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemoryT");

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    const size_t memory_size = capacity * sizeof(T) + sizeof(size_t) * array_count + alignof(size_t);

    const size_t max_array_count = ceil_div(max_capacity, (int32) (sizeof(size_t) * 8));
    const size_t max_memory_size = max_capacity * sizeof(T) + sizeof(size_t) * max_array_count + alignof(size_t);

    MemoryArena* arena = mem_arena_add(
        mem,
        memory_size,
        max_memory_size,
        alignment
    );
    buf->memory = (T *) arena->memory;

    buf->capacity = capacity;
    buf->last_pos = -1;
    buf->free = (size_t *) align_up(
        (size_t) ((uintptr_t) (buf->memory + capacity)),
        alignof(size_t)
    );
    memset(buf->free, 0, sizeof(size_t) * array_count);
}

template <typename T>
inline
void thrd_chunk_alloc(ChunkMemoryT<T>* const buf, MemoryArena* const mem, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_CHUNK_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemoryT");

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    const size_t memory_size = capacity * sizeof(T)
            + sizeof(size_t) * array_count
            + sizeof(size_t) * array_count
            + sizeof(size_t) * 2;

    const size_t max_array_count = ceil_div(max_capacity, (int32) (sizeof(size_t) * 8));
    const size_t max_memory_size = max_capacity * sizeof(T)
            + sizeof(size_t) * max_array_count
            + sizeof(size_t) * max_array_count
            + sizeof(size_t) * 2;

    MemoryArena* arena = mem_arena_add(
        mem,
        memory_size,
        max_memory_size,
        alignment
    );
    buf->memory = (T *) arena->memory;

    buf->capacity = capacity;
    buf->last_pos = -1;
    buf->free = (size_t *) align_up(
        (size_t) ((uintptr_t) (buf->memory + capacity)),
        (size_t) alignof(size_t)
    );
    buf->completeness = (size_t *) align_up((uintptr_t) (buf->free + array_count), alignof(size_t));

    memset((void *) buf->free, 0, sizeof(size_t) * array_count);
    memset((void *) buf->completeness, 0, sizeof(size_t) * array_count);

    spinlock_init(&buf->lock);

    LOG_1("[INFO] Allocated ChunkMemoryT: %n", {DATA_TYPE_UINT64, &buf->capacity});
}

template <typename T>
inline
void chunk_init(
    ChunkMemoryT<T>* const buf,
    BufferMemory* const data,
    int32 capacity,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));

    const size_t size = capacity * sizeof(T)
        + sizeof(size_t) * array_count
        + sizeof(size_t);

    buf->memory = (T *) memory_get(data, size, alignment);

    buf->capacity = capacity;
    buf->last_pos = -1;
    buf->free = (size_t *) align_up(
        (size_t) ((uintptr_t) (buf->memory + capacity)),
        (size_t) alignof(size_t)
    );
    memset(buf->free, 0, sizeof(size_t) * array_count);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, size);
}

template <typename T>
inline
void thrd_chunk_init(
    ChunkMemoryT<T>* const buf,
    BufferMemory* const data,
    int32 capacity,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    const size_t size = capacity * sizeof(T)
        + sizeof(size_t) * array_count
        + sizeof(size_t) * array_count
        + sizeof(size_t) * 2;

    buf->memory = (T *) memory_get(data, size, alignment);

    buf->capacity = capacity;
    buf->last_pos = -1;
    buf->free = (size_t *) align_up(
        (size_t) ((uintptr_t) (buf->memory + capacity)),
        (size_t) alignof(size_t)
    );
    buf->completeness = (size_t *) align_up((uintptr_t) (buf->free + array_count), (size_t) alignof(size_t));

    memset((void *) buf->free, 0, sizeof(size_t) * array_count);
    memset((void *) buf->completeness, 0, sizeof(size_t) * array_count);

    spinlock_init(&buf->lock);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, size);
}

template <typename T>
inline
void chunk_init(
    ChunkMemoryT<T>* const buf,
    byte* const data,
    int32 capacity,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    MAYBE_UNUSED const size_t size = capacity * sizeof(T)
        + sizeof(size_t) * array_count
        + alignment
        + alignof(size_t);

    buf->memory = (T *) align_up((uintptr_t) data, alignment);

    buf->capacity = capacity;
    buf->last_pos = -1;
    buf->free = (size_t *) align_up(
        (size_t) ((uintptr_t) (buf->memory + capacity)),
        (size_t) alignof(size_t)
    );
    memset(buf->free, 0, sizeof(size_t) * array_count);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, size);
    PSEUDO_USE(size);
}

template <typename T>
inline
void thrd_chunk_init(
    ChunkMemoryT<T>* const buf,
    byte* const data,
    int32 capacity,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    MAYBE_UNUSED const size_t size = capacity * sizeof(T)
        + sizeof(size_t) * array_count
        + sizeof(size_t) * array_count
        + sizeof(size_t) * 2;

    buf->memory = (T *) align_up((uintptr_t) data, alignment);

    buf->capacity = capacity;
    buf->last_pos = -1;
    buf->free = (size_t *) align_up(
        (size_t) ((uintptr_t) (buf->memory + capacity)),
        (size_t) alignof(size_t)
    );
    buf->completeness = (size_t *) align_up((uintptr_t) (buf->free + array_count), (size_t) alignof(size_t));

    memset((void *) buf->free, 0, sizeof(size_t) * array_count);
    memset((void *) buf->completeness, 0, sizeof(size_t) * array_count);

    spinlock_init(&buf->lock);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, size);
    PSEUDO_USE(size);
}

template <typename T>
inline
void chunk_free(ChunkMemoryT<T>* const buf) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE(
        (uintptr_t) buf->memory,
        sizeof(T) * buf->capacity + sizeof(size_t) * ceil_div(
            buf->capacity,
            (int32) (sizeof(size_t) * 8)
        )
    );

    platform_aligned_free((void **) &buf->memory);

    buf->capacity = 0;
    buf->memory = NULL;
}

template <typename T>
inline
void thrd_chunk_free(ChunkMemoryT<T>* const buf) NO_EXCEPT
{
    chunk_free(buf);
    //mutex_destroy(&buf->lock);
}

template <typename T>
inline
void chunk_free(ChunkMemoryT<T>* const buf, MemoryArena* const mem) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE((uintptr_t) buf->memory, sizeof(T) * buf->capacity + sizeof(size_t) * ceil_div(buf->capacity, (sizeof(size_t) * 8)));

    mem_arena_remove(mem, buf->memory);

    buf->capacity = 0;
    buf->memory = NULL;
}

template <typename T>
inline
void thrd_chunk_free(ChunkMemoryT<T>* const buf, MemoryArena* const mem) NO_EXCEPT
{
    chunk_free(buf, mem);
    //mutex_destroy(&buf->lock);
}

template <typename T>
FORCE_INLINE
size_t* chunk_find_free_array(const ChunkMemoryT<T>* const buf) NO_EXCEPT
{
    return (size_t *) align_up(
        (uintptr_t) (buf->memory + buf->capacity),
        (size_t) alignof(size_t)
    );
}

template <typename T>
FORCE_INLINE FORCE_FLATTEN
T* chunk_get_element(const ChunkMemoryT<T>* const buf, int32 element) NO_EXCEPT
{
    if (element >= buf->capacity) {
        return NULL;
    }

    T* const offset = &buf->memory[element];
    ASSERT_TRUE(offset);

    DEBUG_MEMORY_READ((uintptr_t) offset, sizeof(T));

    return offset;
}

template <typename T>
FORCE_INLINE
bool chunk_is_free(const ChunkMemoryT<T>* const buf, uint32 element) NO_EXCEPT
{
    return chunk_is_free_internal(buf->free, element);
}

template <typename T>
FORCE_INLINE
bool thrd_chunk_is_free(const ChunkMemoryT<T>* const buf, uint32 element) NO_EXCEPT
{
    SpinlockGuard _guard(&buf->lock, 0);
    return chunk_is_free_internal(buf->free, element);
}

template <typename T>
FORCE_INLINE
int32 chunk_reserve_one(ChunkMemoryT<T>* const buf) NO_EXCEPT
{
    return chunk_reserve_one(buf->free, (uint32) buf->capacity, buf->last_pos);
}

template <typename T>
FORCE_INLINE
int32 thrd_chunk_reserve_one(ChunkMemoryT<T>* const buf) NO_EXCEPT
{
    SpinlockGuard _guard(&buf->lock, 0);
    return chunk_reserve_one(buf);
}

// use chunk_reserve_one if possible
template <typename T>
FORCE_INLINE
int32 chunk_reserve(ChunkMemoryT<T>* const buf, uint32 elements = 1) NO_EXCEPT
{
    const int32 found = chunk_reserve_internal(buf->free, buf->capacity, buf->last_pos, elements);
    buf->last_pos = found + (elements - 1);

    DEBUG_MEMORY_WRITE((uintptr_t) &buf->memory[found], elements * sizeof(T));

    return found;
}

template <typename T>
FORCE_INLINE
int32 thrd_chunk_reserve(ChunkMemoryT<T>* const buf, uint32 elements = 1) NO_EXCEPT
{
    SpinlockGuard _guard(&buf->lock, 0);
    return chunk_reserve(buf, elements);
}

template <typename T>
FORCE_INLINE
void chunk_free_element(ChunkMemoryT<T>* const buf, size_t free_index, int32 bit_index) NO_EXCEPT
{
    buf->free[free_index] &= ~(OMS_UINT_ONE << bit_index);
    DEBUG_MEMORY_DELETE(
        (uintptr_t) &buf->memory[(free_index * (sizeof(size_t) * 8) + bit_index)],
        sizeof(T)
    );
}

template <typename T>
inline
void thrd_chunk_free_element(ChunkMemoryT<T>* const buf, size_t free_index, int32 bit_index) NO_EXCEPT
{
    SpinlockGuard _guard(&buf->lock, 0);
    chunk_free_element(buf, free_index, bit_index);
}

template <typename T>
FORCE_INLINE
void chunk_free_element(ChunkMemoryT<T>* const buf, uint32 element) NO_EXCEPT
{
    const size_t free_index = element / (sizeof(size_t) * 8);
    const uint32 bit_index = MODULO_2(element, (sizeof(size_t) * 8));
    buf->free[free_index] &= ~(OMS_UINT_ONE << bit_index);

    DEBUG_MEMORY_DELETE((uintptr_t) &buf->memory[element], sizeof(T));
}

template <typename T>
FORCE_INLINE
void chunk_free_elements(ChunkMemoryT<T>* const buf, size_t element, uint32 element_count = 1) NO_EXCEPT
{
    chunk_free_elements_internal(buf->state, element, element_count);
    DEBUG_MEMORY_DELETE((uintptr_t) &buf->memory[element], sizeof(T) * element_count);
}

template <typename T>
FORCE_INLINE
void chunk_free_elements(ChunkMemoryT<T>* const buf, T* data, uint32 element_count = 1) NO_EXCEPT
{
    const int32 element = chunk_id_from_memory(buf->memory, data, sizeof(T));
    chunk_free_elements_internal(buf->state, element, element_count);
    DEBUG_MEMORY_DELETE((uintptr_t) &buf->memory[element], sizeof(T) * element_count);
}

template <typename T>
FORCE_INLINE
void thrd_chunk_free_elements(ChunkMemoryT<T>* const buf, size_t element, uint32 element_count = 1) NO_EXCEPT
{
    SpinlockGuard _guard(&buf->lock, 0);
    chunk_free_elements(buf, element, element_count);
}

/**
 * Binary representation:
 *
 * 00 01 02 03 = capacity
 * 04 05 06 07 = last_pos
 * 08 09 0A 0B = free_offset
 * 0C .. .. .. = hash map data
 */
template <typename T>
inline
int64 chunk_dump(const ChunkMemoryT<T>* const buf, byte* data) NO_EXCEPT
{
    LOG_1("[INFO] Dump ChunkMemoryT");
    const byte* const start = data;

    data = write_le(data, buf->capacity);
    data = write_le(data, buf->last_pos);

    const uint32 free_offset = (uint32) ((uintptr_t) buf->free - (uintptr_t) buf->memory);
    data = write_le(data, free_offset);

    const size_t size = buf->capacity * sizeof(T)
        + sizeof(size_t) * ceil_div(buf->capacity, (int32) (sizeof(size_t) * 8))
        + sizeof(size_t);

    // All memory is handled in the buffer -> simply copy the buffer
    // This also includes the free array
    memcpy(data, buf->memory, size);

    SWAP_ENDIAN_LITTLE_SIMD(
        (size_t *) (data + free_offset),
        (size_t *) (data + free_offset),
        buf->capacity / sizeof(size_t),
        8
    );

    data += size;

    LOG_1("[INFO] Dumped ChunkMemoryT: %n B", {DATA_TYPE_UINT64, (void *) &size});

    return data - start;
}

template <typename T>
FORCE_INLINE
T* chunk_memory_get(ChunkMemoryT<T>* const buf, uint32 elements) NO_EXCEPT
{
    const int32 element = chunk_reserve(buf, elements);

    return chunk_get_element(buf, element);
}

template <typename T>
inline HOT_CODE
T* memory_get(ChunkMemoryT<T>* const buf, size_t size) NO_EXCEPT
{
    return chunk_memory_get(buf, (size + sizeof(T) - 1) / sizeof(T));
}

template <typename T>
inline HOT_CODE
T* memory_get_temp(ChunkMemoryT<T>* const buf, size_t size) NO_EXCEPT
{
    const uint32 element_count = (uint32) (size + sizeof(T) - 1) / sizeof(T);
    T* data = chunk_memory_get(buf, element_count);
    chunk_free_elements(buf, data, element_count);

    return data;
}

template <typename T>
FORCE_INLINE
T* chunk_memory_get_one(ChunkMemoryT<T>* const buf) NO_EXCEPT
{
    const int32 element = chunk_reserve_one(buf);

    return chunk_get_element(buf, element);
}

template <typename T>
inline
int64 chunk_load(ChunkMemoryT<T>* const buf, const byte* data) NO_EXCEPT
{
    LOG_1("[INFO] Loading ChunkMemoryT");

    const byte* const start = data;

    data = read_le(data, &buf->capacity);
    data = read_le(data, &buf->last_pos);

    uint32 free_offset;
    data = read_le(data, &free_offset);

    const size_t size = buf->capacity * sizeof(T)
        + sizeof(size_t) * ceil_div(buf->capacity, (int32) (sizeof(size_t) * 8))
        + sizeof(size_t);

    memcpy(buf->memory, data, size);
    data += size;

    buf->free = (size_t *) (((uintptr_t) buf->memory) + free_offset);

    SWAP_ENDIAN_LITTLE_SIMD(
        buf->free,
        buf->free,
        buf->capacity / sizeof(size_t),
        8
    );

    LOG_1("[INFO] Loaded ChunkMemoryT: %n B", {DATA_TYPE_UINT64, &size});

    return data - start;
}

#endif
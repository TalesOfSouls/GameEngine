/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MEMORY_CHUNK_MEMORYT_H
#define COMS_MEMORY_CHUNK_MEMORYT_H

#include "../stdlib/Stdlib.h"
#include "../utils/Assert.h"
#include "../utils/BitUtils.h"
#include "../compiler/CompilerUtils.h"
#include "../log/Log.h"
#include "../log/Stats.h"
#include "../log/PerformanceProfiler.h"
#include "../log/DebugMemory.h"
#include "../thread/ThreadDefines.h"
#include "BufferMemory.h"
#include "../system/Allocator.h"
#include "ChunkMemory.h"

/**
 * This storage system is best used for fixed sized chunks
 * What I mean by that, every element has the same size.
 * Currently a caller could reserve multiple chunks to represent a single data entity
 * This can be fine in a single threaded application
 * However, this can lead to fragmentation which is hard to clean up because
 * we can't just defragment the memory since we don't know which chunks are currently in use
 * In use could mean by pointer of id. In use data isn't allowed to move or it would become "invalid"
 * If you need a data structure that can be defragmented use DataPool, which basically builds upon ChunkMemoryT
 * Fixed sized data structures that use this ChunkMemoryT can be:
 *      1. HashMap
 *      2. Queue
 * Carefull, both examples have alternative use cases which may require variable sized elements
 * WARNING: Changing this struct has effects on other data structures
 */
template <typename T>
struct ChunkMemoryT {
    T* memory;

    int32 capacity;
    atomic_32 int32 last_pos;

    // length = count
    // free describes which locations are used and which are free
    atomic_ptr uint_max* free;

    // Chunk implementation ends here
    // The completeness indicates if the data is completely written to
    atomic_ptr uint_max* completeness;

    mutex lock;
};

// INFO: A chunk count of 2^n is recommended for maximum performance
template <typename T>
inline
void chunk_alloc(ChunkMemoryT<T>* const buf, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE(PROFILE_CHUNK_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemoryT");

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(uint_max) * 8));
    const size_t memory_size = capacity * sizeof(T) + sizeof(uint_max) * array_count + sizeof(uint_max);

    const size_t max_array_count = ceil_div(max_capacity, (int32) (sizeof(uint_max) * 8));
    const size_t max_memory_size = max_capacity * sizeof(T) + sizeof(uint_max) * max_array_count + sizeof(uint_max);

    buf->memory = (T *) platform_alloc_aligned(
        memory_size,
        max_memory_size,
        alignment
    );

    buf->capacity = capacity;
    buf->last_pos = -1;
    buf->free = (uint_max *) align_up(
        (uint_max) ((uintptr_t) (buf->memory + capacity)),
        sizeof(uint_max)
    );
    memset(buf->free, 0, sizeof(uint_max) * array_count);
}

template <typename T>
inline
void thrd_chunk_alloc(ChunkMemoryT<T>* const buf, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE(PROFILE_CHUNK_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemoryT");

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(uint_max) * 8));
    const size_t memory_size = capacity * sizeof(T)
            + sizeof(uint_max) * array_count
            + sizeof(uint_max) * array_count
            + sizeof(uint_max) * 2;

    const size_t max_array_count = ceil_div(max_capacity, (int32) (sizeof(uint_max) * 8));
    const size_t max_memory_size = max_capacity * sizeof(T)
            + sizeof(uint_max) * max_array_count
            + sizeof(uint_max) * max_array_count
            + sizeof(uint_max) * 2;

    buf->memory = (T *) platform_alloc_aligned(
        memory_size,
        max_memory_size,
        alignment
    );

    buf->capacity = capacity;
    buf->last_pos = -1;
    buf->free = (uint_max *) align_up(
        (uint_max) ((uintptr_t) (buf->memory + capacity)),
        (uint_max) sizeof(uint_max)
    );
    buf->completeness = (uint_max *) align_up((uintptr_t) (buf->free + array_count), sizeof(uint_max));

    memset((void *) buf->free, 0, sizeof(uint_max) * array_count);
    memset((void *) buf->completeness, 0, sizeof(uint_max) * array_count);

    mutex_init(&buf->lock, NULL);

    LOG_1("[INFO] Allocated ChunkMemoryT: %n", {DATA_TYPE_UINT64, &buf->capacity});
}

// INFO: A chunk count of 2^n is recommended for maximum performance
template <typename T>
inline
void chunk_alloc(ChunkMemoryT<T>* const buf, MemoryArena* mem, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE(PROFILE_CHUNK_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemoryT");

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(uint_max) * 8));
    const size_t memory_size = capacity * sizeof(T) + sizeof(uint_max) * array_count + sizeof(uint_max);

    const size_t max_array_count = ceil_div(max_capacity, (int32) (sizeof(uint_max) * 8));
    const size_t max_memory_size = max_capacity * sizeof(T) + sizeof(uint_max) * max_array_count + sizeof(uint_max);

    MemoryArena* arena = mem_arena_add(
        mem,
        memory_size,
        max_memory_size,
        alignment
    );
    buf->memory = (T *) arena->memory;

    buf->capacity = capacity;
    buf->last_pos = -1;
    buf->free = (uint_max *) align_up(
        (uint_max) ((uintptr_t) (buf->memory + capacity)),
        sizeof(uint_max)
    );
    memset(buf->free, 0, sizeof(uint_max) * array_count);
}

template <typename T>
inline
void thrd_chunk_alloc(ChunkMemoryT<T>* const buf, MemoryArena* mem, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE(PROFILE_CHUNK_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemoryT");

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(uint_max) * 8));
    const size_t memory_size = capacity * sizeof(T)
            + sizeof(uint_max) * array_count
            + sizeof(uint_max) * array_count
            + sizeof(uint_max) * 2;

    const size_t max_array_count = ceil_div(max_capacity, (int32) (sizeof(uint_max) * 8));
    const size_t max_memory_size = max_capacity * sizeof(T)
            + sizeof(uint_max) * max_array_count
            + sizeof(uint_max) * max_array_count
            + sizeof(uint_max) * 2;

    MemoryArena* arena = mem_arena_add(
        mem,
        memory_size,
        max_memory_size,
        alignment
    );
    buf->memory = (T *) arena->memory;

    buf->capacity = capacity;
    buf->last_pos = -1;
    buf->free = (uint_max *) align_up(
        (uint_max) ((uintptr_t) (buf->memory + capacity)),
        (uint_max) sizeof(uint_max)
    );
    buf->completeness = (uint_max *) align_up((uintptr_t) (buf->free + array_count), sizeof(uint_max));

    memset((void *) buf->free, 0, sizeof(uint_max) * array_count);
    memset((void *) buf->completeness, 0, sizeof(uint_max) * array_count);

    mutex_init(&buf->lock, NULL);

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

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(uint_max) * 8));

    const size_t size = capacity * sizeof(T)
        + sizeof(uint_max) * array_count
        + sizeof(uint_max);

    buf->memory = (T *) buffer_get_memory(data, size, alignment);

    buf->capacity = capacity;
    buf->last_pos = -1;
    buf->free = (uint_max *) align_up(
        (uint_max) ((uintptr_t) (buf->memory + capacity)),
        (uint_max) sizeof(uint_max)
    );
    memset(buf->free, 0, sizeof(uint_max) * array_count);

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

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(uint_max) * 8));
    const size_t size = capacity * sizeof(T)
        + sizeof(uint_max) * array_count
        + sizeof(uint_max) * array_count
        + sizeof(uint_max) * 2;

    buf->memory = (T *) buffer_get_memory(data, size, alignment);

    buf->capacity = capacity;
    buf->last_pos = -1;
    buf->free = (uint_max *) align_up(
        (uint_max) ((uintptr_t) (buf->memory + capacity)),
        (uint_max) sizeof(uint_max)
    );
    buf->completeness = (uint_max *) align_up((uintptr_t) (buf->free + array_count), (uint_max) sizeof(uint_max));

    memset((void *) buf->free, 0, sizeof(uint_max) * array_count);
    memset((void *) buf->completeness, 0, sizeof(uint_max) * array_count);

    mutex_init(&buf->lock, NULL);

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

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(uint_max) * 8));
    const size_t size = capacity * sizeof(T)
        + sizeof(uint_max) * array_count
        + sizeof(uint_max);

    buf->memory = (T *) align_up((uintptr_t) data, alignment);

    buf->capacity = capacity;
    buf->last_pos = -1;
    buf->free = (uint_max *) align_up(
        (uint_max) ((uintptr_t) (buf->memory + capacity)),
        (uint_max) sizeof(uint_max)
    );
    memset(buf->free, 0, sizeof(uint_max) * array_count);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, size);
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

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(uint_max) * 8));
    const size_t size = capacity * sizeof(T)
        + sizeof(uint_max) * array_count
        + sizeof(uint_max) * array_count
        + sizeof(uint_max) * 2;

    buf->memory = (T *) align_up((uintptr_t) data, alignment);

    buf->capacity = capacity;
    buf->last_pos = -1;
    buf->free = (uint_max *) align_up(
        (uint_max) ((uintptr_t) (buf->memory + capacity)),
        (uint_max) sizeof(uint_max)
    );
    buf->completeness = (uint_max *) align_up((uintptr_t) (buf->free + array_count), (uint_max) sizeof(uint_max));

    memset((void *) buf->free, 0, sizeof(uint_max) * array_count);
    memset((void *) buf->completeness, 0, sizeof(uint_max) * array_count);

    mutex_init(&buf->lock, NULL);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, size);
}

template <typename T>
inline
void chunk_free(ChunkMemoryT<T>* const buf) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE(
        (uintptr_t) buf->memory,
        sizeof(T) * buf->capacity + sizeof(uint_max) * ceil_div(
            buf->capacity,
            (int32) (sizeof(uint_max) * 8)
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
    mutex_destroy(&buf->lock);
}

template <typename T>
inline
void chunk_free(ChunkMemoryT<T>* const buf, MemoryArena* mem) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE((uintptr_t) buf->memory, sizeof(T) * buf->capacity + sizeof(uint_max) * ceil_div(buf->capacity, (sizeof(uint_max) * 8)));

    mem_arena_remove(mem, buf->memory);

    buf->capacity = 0;
    buf->memory = NULL;
}

template <typename T>
inline
void thrd_chunk_free(ChunkMemoryT<T>* const buf, MemoryArena* mem) NO_EXCEPT
{
    chunk_free(buf, mem);
    mutex_destroy(&buf->lock);
}

template <typename T>
FORCE_INLINE
uint_max* chunk_find_free_array(const ChunkMemoryT<T>* const buf) NO_EXCEPT
{
    return (uint_max *) align_up(
        (uintptr_t) (buf->memory + buf->capacity),
        (uint_max) sizeof(uint_max)
    );
}

/*
You can use the thrd_chunk_set_unset_atomic() from ChunkMemory.h
*/

template <typename T>
FORCE_INLINE FORCE_FLATTEN
T* chunk_get_element(const ChunkMemoryT<T>* const buf, int32 element) NO_EXCEPT
{
    // @question How is this even possible? Isn't an assert enough?
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
bool thrd_chunk_is_free_atomic(const ChunkMemoryT<T>* const buf, uint32 element) NO_EXCEPT
{
    return thrd_chunk_is_free_atomic_internal(buf->free, element);
}

template <typename T>
FORCE_INLINE
int32 chunk_reserve_one(ChunkMemoryT<T>* const buf) NO_EXCEPT
{
    return chunk_reserve_one(buf->free, (uint32) buf->capacity, buf->last_pos);
}

template <typename T>
FORCE_INLINE
int32 thrd_chunk_reserve_one_atomic(ChunkMemoryT<T>* const buf) NO_EXCEPT
{
    int32 last = atomic_fetch_increment_wrap_release(&buf->last_pos, -1, buf->capacity);

    return thrd_chunk_reserve_one_atomic(buf->free, buf->capacity, last);
}

// use chunk_reserve_one if possible
template <typename T>
FORCE_INLINE
int32 chunk_reserve(ChunkMemoryT<T>* const buf, uint32 elements = 1) NO_EXCEPT
{
    buf->last_pos = chunk_reserve_internal(buf->free, buf->capacity, buf->last_pos, elements);

    DEBUG_MEMORY_WRITE((uintptr_t) &buf->memory[buf->last_pos], elements * sizeof(T));

    return buf->last_pos;
}

template <typename T>
FORCE_INLINE
int32 thrd_chunk_reserve(ChunkMemoryT<T>* const buf, uint32 elements = 1) NO_EXCEPT
{
    MutexGuard _guard(&buf->lock);
    return chunk_reserve(buf, elements);
}

template <typename T>
FORCE_INLINE
void chunk_free_element(ChunkMemoryT<T>* const buf, uint_max free_index, int32 bit_index) NO_EXCEPT
{
    buf->free[free_index] &= ~(OMS_UINT_ONE << bit_index);
    DEBUG_MEMORY_DELETE(
        (uintptr_t) &buf->memory[(free_index * (sizeof(uint_max) * 8) + bit_index)],
        sizeof(T)
    );
}

template <typename T>
inline
void thrd_chunk_free_element(ChunkMemoryT<T>* const buf, uint_max free_index, int32 bit_index) NO_EXCEPT
{
    thrd_chunk_free_element_internal(buf->free, free_index, bit_index);
    DEBUG_MEMORY_DELETE(
        (uintptr_t) &buf->memory[(free_index * (sizeof(uint_max) * 8) + bit_index)],
        sizeof(T)
    );
}

template <typename T>
FORCE_INLINE
void chunk_free_element(ChunkMemoryT<T>* const buf, uint32 element) NO_EXCEPT
{
    const uint_max free_index = element / (sizeof(uint_max) * 8);
    const uint32 bit_index = MODULO_2(element, (sizeof(uint_max) * 8));
    buf->free[free_index] &= ~(OMS_UINT_ONE << bit_index);

    DEBUG_MEMORY_DELETE((uintptr_t) &buf->memory[element], sizeof(T));
}

template <typename T>
FORCE_INLINE
void chunk_free_elements(ChunkMemoryT<T>* const buf, uint_max element, uint32 element_count = 1) NO_EXCEPT
{
    chunk_free_elements_internal(buf->state, element, element_count);
    DEBUG_MEMORY_DELETE((uintptr_t) &buf->memory[element], sizeof(T) * element_count);
}

template <typename T>
FORCE_INLINE
void thrd_chunk_free_elements_atomic(ChunkMemoryT<T>* const buf, uint_max element, uint32 element_count = 1) NO_EXCEPT
{
    thrd_chunk_free_elements_atomic_internal(buf->free, element, element_count);
    DEBUG_MEMORY_DELETE((uintptr_t) &buf->memory[element], sizeof(T) * element_count);
}

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
        + sizeof(uint_max) * ceil_div(buf->capacity, (sizeof(uint_max) * 8))
        + sizeof(uint_max);

    // All memory is handled in the buffer -> simply copy the buffer
    // This also includes the free array
    memcpy(data, buf->memory, size);

    #if !_WIN32 && !__LITTLE_ENDIAN__
        uint_max* free_data = (uint_max *) (data + free_offset);
    #endif

    data += size;

    #if !_WIN32 && !__LITTLE_ENDIAN__
        // @todo replace with simd endian swap if it is faster
        for (uint32 i = 0; i < ceil_div(buf->capacity, (int32) (sizeof(uint_max) * 8)); ++i) {
            *free_data = SWAP_ENDIAN_LITTLE(*free_data);
            ++free_data;
        }
    #endif

    LOG_1("[INFO] Dumped ChunkMemoryT: %n B", {DATA_TYPE_UINT64, (void *) &size});

    return data - start;
}

template <typename T>
FORCE_INLINE
byte* chunk_get_memory(ChunkMemoryT<T>* const buf, uint32 elements) NO_EXCEPT
{
    const int32 element = chunk_reserve(buf, elements);

    return chunk_get_element(buf, element);
}

template <typename T>
FORCE_INLINE
byte* chunk_get_memory_one(ChunkMemoryT<T>* const buf) NO_EXCEPT
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
        + sizeof(uint_max) * ceil_div(buf->capacity, (sizeof(uint_max) * 8))
        + sizeof(uint_max);

    memcpy(buf->memory, data, size);
    data += size;

    buf->free = (uint_max *) (((uintptr_t) buf->memory) + free_offset);

    #if !_WIN32 && !__LITTLE_ENDIAN__
        uint_max* free_data = buf->free;
        // @todo replace with simd endian swap if it is faster
        for (uint32 i = 0; i < ceil_div(buf->capacity, (int32) (sizeof(uint_max) * 8)); ++i) {
            *free_data = SWAP_ENDIAN_LITTLE(*free_data);
            ++free_data;
        }
    #endif

    LOG_1("[INFO] Loaded ChunkMemoryT: %n B", {DATA_TYPE_UINT64, &size});

    return data - start;
}

#endif
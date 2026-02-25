/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MEMORY_FRAGMENT_MEMORYT_H
#define COMS_MEMORY_FRAGMENT_MEMORYT_H

#include "../stdlib/Stdlib.h"
#include "../utils/Assert.h"
#include "../compiler/CompilerUtils.h"
#include "../log/Log.h"
#include "../log/Stats.h"
#include "../log/PerformanceProfiler.h"
#include "../log/DebugMemory.h"
#include "../system/Allocator.h"
#include "MemoryArena.h"

/**
 * Similar to the ChunkMemory but:
 * Benefits
 *      * Much faster at finding free memory chunks
 *
 * Disadvantages
 *      * Requires you to release memory once no longer used
 *      * Memory gets fragmented over time/usage
 *      * Impossible to request multiple contiguous chunks
 */
template <typename T>
struct FragmentMemoryT {
    T* memory;

    int capacity;
    int last_pos;

    // Array that contains pointers into the free chunks
    T** free;

    mutex lock;
};

template <typename T>
inline
void fragment_alloc(
    FragmentMemoryT<T>* const fragment,
    int capacity, int max_capacity,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    PROFILE(PROFILE_FRAGMENT_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating FragmentMemoryT");

    const size_t memory_size = sizeof(T) * capacity + capacity * sizeof(T*) + alignment;
    const size_t max_memory_size = sizeof(T) * max_capacity + max_capacity * sizeof(T*) + alignment;

    fragment->memory = (T *) platform_alloc_aligned(
        memory_size,
        max_memory_size,
        alignment
    );

    fragment->capacity = capacity;
    fragment->last_pos = capacity - 1;
    fragment->free = (T **) align_up(
        (uint_max) ((uintptr_t) (fragment->memory + capacity)),
        (uint_max) alignment
    );

    for (int i = 0; i < capacity; ++i) {
        fragment->free[i] = &fragment->memory[i];
    }
}

template <typename T>
inline
void fragment_alloc(
    FragmentMemoryT<T>* const fragment,
    MemoryArena* mem,
    int capacity, int max_capacity,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    PROFILE(PROFILE_FRAGMENT_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating FragmentMemoryT");

    const size_t memory_size = sizeof(T) * capacity + capacity * sizeof(T*) + alignment;
    const size_t max_memory_size = sizeof(T) * max_capacity + max_capacity * sizeof(T*) + alignment;

    MemoryArena* arena = mem_arena_add(
        mem,
        memory_size,
        max_memory_size,
        alignment
    );
    fragment->memory = (T *) arena->memory;

    fragment->capacity = capacity;
    fragment->last_pos = capacity - 1;
    fragment->free = (T **) align_up(
        (uint_max) ((uintptr_t) (fragment->memory + capacity)),
        (uint_max) alignment
    );

    for (int i = 0; i < capacity; ++i) {
        fragment->free[i] = &fragment->memory[i];
    }
}

template <typename T>
inline HOT_CODE
T* fragment_get_memory(FragmentMemoryT<T>* const fragment) NO_EXCEPT
{
    if (fragment->last_pos < 0) {
        return NULL;
    }

    DEBUG_MEMORY_READ(fragment->free[fragment->last_pos - 1], sizeof(T));

    return fragment->free[fragment->last_pos--];
}

template <typename T>
inline HOT_CODE
void fragment_release_memory(FragmentMemoryT<T>* const fragment, T* data) NO_EXCEPT
{
    fragment->free[++fragment->last_pos] = data;
}

template <typename T>
inline
void fragment_free(FragmentMemoryT<T>* const fragment) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE((uintptr_t) fragment->memory, sizeof(T) * fragment->capacity);

    platform_aligned_free((void **) &fragment->memory);

    fragment->capacity = 0;
    fragment->last_pos = -1;
    fragment->memory = NULL;
}

template <typename T>
inline
void fragment_free(FragmentMemoryT<T>* const fragment, MemoryArena* mem) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE((uintptr_t) fragment->memory, sizeof(T) * fragment->capacity);

    mem_arena_remove(mem, fragment->memory);

    fragment->capacity = 0;
    fragment->last_pos = -1;
    fragment->memory = NULL;
}

#endif
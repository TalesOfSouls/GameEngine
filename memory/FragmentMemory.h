/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_FRAGMENT_MEMORY_H
#define COMS_MEMORY_FRAGMENT_MEMORY_H

#include "../stdlib/Stdlib.h"
#include "../log/Log.h"
#include "../log/Stats.h"
#include "../log/PerformanceProfiler.h"
#include "../log/DebugMemory.h"
#include "../system/Allocator.h"
#include "MemoryArena.h"

/**
 * Stores pointers to available memory chunks in an array
 *
 * Similar to the ChunkMemory but:
 * Benefits
 *      * Much faster at finding free memory chunks
 *
 * Disadvantages
 *      * Requires you to release memory once no longer used
 *      * Memory gets fragmented over time/usage
 *      * Impossible to request multiple contiguous chunks
 */
struct FragmentMemory {
    byte* memory;

    // @question Do I really want to use uint?
    size_t size;
    int32 last_pos;
    uint32 count;
    int32 chunk_size;

    // WARNING: The alignment may increase the original chunk size e.g.
    // element_size = 14, alignment = sizeof(size_t) => chunk_size = 32
    uint32 alignment;

    // Array that contains pointers into the free chunks
    int32* free;

    spinlock32 lock;
};

static FORCE_INLINE
int32 fragment_size_element(int32 element_size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    return align_up(element_size, alignment);
}

FORCE_INLINE
size_t fragment_size_total(uint32 count, int32 element_size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    element_size = fragment_size_element(element_size, alignment);

    return count * element_size
        + sizeof(int32) * count // free
        + alignof(uintptr_t) * 2; // overhead for alignment
}

inline
void fragment_alloc(
    FragmentMemory* const fragment,
    uint32 count,
    uint32 max_count,
    int32 element_size,
    int32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_FRAGMENT_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(count);
    ASSERT_TRUE(max_count >= count);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating FragmentMemory");

    element_size = fragment_size_element(element_size, alignment);
    const size_t size = fragment_size_total(count, element_size, alignment);
    const size_t max_size = fragment_size_total(max_count, element_size, alignment);

    fragment->memory = (byte *) platform_alloc_aligned(size, max_size, start_alignment);

    fragment->count = count;
    fragment->size = size;
    fragment->chunk_size = element_size;
    fragment->last_pos = count - 1;
    fragment->alignment = alignment;
    fragment->free = (int32 *) align_up(
        (size_t) ((uintptr_t) (fragment->memory + count * element_size)),
        (size_t) alignof(int32)
    );

    for (int i = 0; i < count; ++i) {
        fragment->free[i] = i * fragment->chunk_size;
    }
}

inline
void fragment_alloc(
    FragmentMemory* const fragment,
    MemoryArena* const mem,
    uint32 count,
    uint32 max_count,
    int32 element_size,
    int32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_FRAGMENT_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(count);
    ASSERT_TRUE(max_count >= count);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating FragmentMemory");

    element_size = fragment_size_element(element_size, alignment);
    const size_t size = fragment_size_total(count, element_size, alignment);
    const size_t max_size = fragment_size_total(max_count, element_size, alignment);

    MemoryArena* arena = mem_arena_add(mem, size, max_size, start_alignment);
    fragment->memory = (byte *) arena->memory;

    fragment->count = count;
    fragment->size = size;
    fragment->chunk_size = element_size;
    fragment->last_pos = count - 1;
    fragment->alignment = alignment;
    fragment->free = (int32 *) align_up(
        (size_t) ((uintptr_t) (fragment->memory + count * element_size)),
        (size_t) alignof(int32)
    );

    for (int i = 0; i < count; ++i) {
        fragment->free[i] = i * fragment->chunk_size;
    }
}

inline HOT_CODE
byte* fragment_memory_get(FragmentMemory* const fragment) NO_EXCEPT
{
    if (fragment->last_pos < 0) {
        return NULL;
    }

    DEBUG_MEMORY_READ(&fragment->memory[fragment->free[fragment->last_pos]], buf->chunk_size);

    return &fragment->memory[fragment->free[fragment->last_pos--]];
}

inline HOT_CODE
void fragment_release_memory(FragmentMemory* const fragment, byte* const data) NO_EXCEPT
{
    fragment->free[++fragment->last_pos] = (int32) (data - fragment->memory);
}

inline
void fragment_free(FragmentMemory* const fragment) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE((uintptr_t) fragment->memory, fragment->size);

    platform_aligned_free((void **) &fragment->memory);

    fragment->size = 0;
    fragment->last_pos = -1;
    fragment->memory = NULL;
}

inline
void fragment_free(FragmentMemory* const fragment, MemoryArena* mem) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE((uintptr_t) fragment->memory, fragment->size);

    mem_arena_remove(mem, fragment->memory);

    fragment->size = 0;
    fragment->last_pos = -1;
    fragment->memory = NULL;
}

#endif
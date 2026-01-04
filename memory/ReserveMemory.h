/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MEMORY_RESERVE_MEMORY_H
#define COMS_MEMORY_RESERVE_MEMORY_H

#include <string.h>
#include "../stdlib/Stdlib.h"
#include "../utils/Assert.h"
#include "../compiler/CompilerUtils.h"
#include "../log/Log.h"
#include "../log/Stats.h"
#include "../log/PerformanceProfiler.h"
#include "../log/DebugMemory.h"
#include "../system/Allocator.h"

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
struct FragmentMemory {
    byte* memory;

    // @question Do I really want to use uint?
    uint_max size;
    int32 last_pos;
    uint32 count;
    int32 chunk_size;

    // WARNING: The alignment may increase the original chunk size e.g.
    // element_size = 14, alignment = sizeof(size_t) => chunk_size = 32
    uint32 alignment;

    // Array that contains pointers into the free chunks
    byte* free;
};

FORCE_INLINE
int32 fragment_size_element(int32 element_size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    return align_up(element_size, alignment);
}

FORCE_INLINE
uint_max fragment_size_total(uint32 count, int32 element_size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    element_size = fragment_size_element(element_size, alignment);

    // @performance Can we remove the alignment * 2? This is just a shotgun method to ensure full alignment

    return count * element_size
        + sizeof(byte*) * count // free
        + alignment * 2; // overhead for alignment
}

inline
void fragment_alloc(FragmentMemory* const fragment, uint32 count, int32 element_size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE(PROFILE_FRAGMENT_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(count);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating FragmentMemory");

    element_size = fragment_size_element(element_size, alignment);
    const uint_max size = fragment_size_total(count, element_size, alignment);

    fragment->memory = alignment < 2
        ? (byte *) platform_alloc(size)
        : (byte *) platform_alloc_aligned(size, alignment);

    fragment->count = count;
    fragment->size = size;
    fragment->chunk_size = element_size;
    fragment->last_pos = count - 1;
    fragment->alignment = alignment;
    fragment->free = (uint_max *) align_up(
        (uint_max) ((uintptr_t) (fragment->memory + count * element_size)),
        (uint_max) alignment
    );

    for (int i = 0; i < count; ++i) {
        fragment->free[i] = fragment->memory + i * fragment->chunk_size;
    }
}

inline HOT_CODE
byte* fragment_get_memory(FragmentMemory* const fragment) NO_EXCEPT
{
    if (fragment->last_pos < 0) {
        return NULL;
    }

    DEBUG_MEMORY_READ(fragment->free[fragment->last_pos - 1], buf->chunk_size);

    return fragment->free[fragment->last_pos--];
}

inline HOT_CODE
byte* fragment_release_memory(FragmentMemory* const fragment, byte* data) NO_EXCEPT
{
    fragment->free[++fragment->last_pos] = data;
}

inline
void fragment_free(FragmentMemory* const fragment) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE((uintptr_t) fragment->memory, fragment->size);

    if (fragment->alignment < 2) {
        platform_free((void **) &fragment->memory);
    } else {
        platform_aligned_free((void **) &fragment->memory);
    }

    fragment->size = 0;
    fragment->last_pos = -1;
    fragment->memory = NULL;
}

#endif
/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MEMORY_ARENA_H
#define COMS_MEMORY_ARENA_H

#include "../stdlib/Stdlib.h"
#include "../utils/Assert.h"
#include "../system/Allocator.h"
#include "../log/Stats.h"
#include "../log/PerformanceProfiler.h"
#include "../log/DebugMemory.h"

/**
 * Real memory layout
 *
 *      START OF PLATFORM ALLOC OVERHEAD
 *      A: platform_alloc_header
 *      B: ... potentially free memory for alignment ...
 *      C: void* (points to A)
 *
 *      START OF MEMORY ARENA OVERHEAD
 *      D: [aligned to at least sizeof(uintptr_t)] start of first level user space
 *      E: ... potentially free memory for alignment ...
 *      F: [aligned] MemoryArena (metadata) and pointer to next block
 *      G: ... potentially free memory (max sizeof(uintptr_t)) for alignment ...
 *
 *      START OF USER MEMORY SPACE
 *      H: [aligned] second level user space/memory arena start
 */
struct MemoryArena {
    // Points to A
    platform_alloc_header* header;

    // Points to D
    byte* base;

    // Points to H
    byte* memory;

    // Points to the next block
    MemoryArena* next;
};

struct MemoryArenaStats {
    size_t reserved_size;
    size_t committed_size;
    int32 count;
    int32 overhead;
};

// Usage: call mem_arena_alloc() only once for the root element, afterwards only use mem_arena_add()
inline
MemoryArena* mem_arena_alloc(
    size_t initial_size, size_t reserve_size,
    int32 alignment = sizeof(void*)
) {
    PROFILE(PROFILE_ARENA_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);

    initial_size += align_up(sizeof(MemoryArena), alignment);
    reserve_size += align_up(sizeof(MemoryArena), alignment);

    byte* allocated_memory = (byte *) platform_alloc_aligned(initial_size, reserve_size, alignment);
    byte* memory = (byte *) align_up((uintptr_t) allocated_memory + sizeof(MemoryArena), alignment);

    /**
     * We need to do this weird align down and negative movement instead of putting it at the beginning of
     * allocated_memory since when we free a memory arena we will not pass the base/allocated_memory pointer
     * but instead we will pass the "user space" memory address which comes somewhere after MemoryArena.
     *
     * If we would put it at the beginning of allocated_memory we couldn't find where it starts by just
     * using the user space memory address since the position is dependant on the alignment which is not stored.
     *
     * By doing this negative movement we have a deterministic way to find the MemoryArena. We still need
     * to store the base address in MemoryArena which is unfortunate but we have no other way to determine
     * where the base/allocated_memory starts.
     *
     * Keep in mind even allocated_memory is not really the base address since we have additional memory header
     * data stored before allocated_memory. What have we stored before? Well, it is platform_alloc_header
     */
    MemoryArena* mem = (MemoryArena *) align_down((uintptr_t) memory - sizeof(MemoryArena), sizeof(uintptr_t));
    mem->base = allocated_memory;
    mem->memory = memory;
    mem->header = (platform_alloc_header *) ((void**) allocated_memory)[-1];
    mem->memory = (byte *) align_up((uintptr_t) allocated_memory + sizeof(MemoryArena), alignment);

    return mem;
}

inline
MemoryArena* mem_arena_add(
    MemoryArena* mem,
    size_t initial_size, size_t reserve_size,
    int32 alignment = sizeof(void*)
) {
    while (mem->next) {
        mem = mem->next;
    }

    mem->next = mem_arena_alloc(initial_size, reserve_size, alignment);

    return mem->next;
}

static inline
void mem_arena_remove(
    MemoryArena* mem,
    MemoryArena* remove
) NO_EXCEPT {
    MemoryArena* prev = NULL;
    while (mem != remove) {
        prev = mem;
        mem = mem->next;
    }

    if (prev) {
        prev->next = mem->next;
    }

    platform_aligned_free((void **) &remove->base);
}

inline
void mem_arena_remove(
    MemoryArena* mem,
    byte* remove
) NO_EXCEPT {
    MemoryArena* arena = (MemoryArena *) align_down((uintptr_t) remove - sizeof(MemoryArena), sizeof(uintptr_t));
    mem_arena_remove(mem, arena);
}

inline
MemoryArenaStats mem_arena_stats(MemoryArena* mem) {
    MemoryArenaStats stats = {0};

    while (mem) {
        stats.reserved_size += mem->header->reserved_size;
        stats.committed_size += mem->header->committed_size;
        ++stats.count;
        stats.overhead += (int32) (mem->memory - (byte *) ((void**) mem->base)[-1]);

        mem = mem->next;
    }

    return stats;
}

#endif
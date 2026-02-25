/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS__MEMORY_RING_MEMORY_C
#define COMS__MEMORY_RING_MEMORY_C

#include "../stdlib/Stdlib.h"
#include "../utils/Assert.h"
#include "BufferMemory.h"
#include "MemoryArena.h"
#include "../log/Log.h"
#include "../log/Stats.h"
#include "../log/PerformanceProfiler.h"
#include "../log/DebugMemory.h"
#include "../thread/Atomic.h"
#include "../thread/Semaphore.h"
#include "../system/Allocator.h"
#include "../thread/Thread.h"

#include "RingMemory.h"

inline
void ring_alloc(RingMemory* const ring, size_t size, size_t max_size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ASSERT_TRUE(size);
    ASSERT_TRUE(max_size >= size);
    ASSERT_TRUE(alignment % sizeof(int) == 0);
    PROFILE(PROFILE_RING_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);

    size = align_up(size, ASSUMED_CACHE_LINE_SIZE);
    LOG_1("[INFO] Allocating RingMemory: %n B", {DATA_TYPE_UINT64, &size});

    ring->memory = (byte *) platform_alloc_aligned(size, max_size, alignment);

    ring->end = ring->memory + size;
    ring->head = ring->memory;
    ring->tail = ring->memory;
    ring->size = size;
    ring->alignment = alignment;
}

inline
void ring_alloc(
    RingMemory* const __restrict ring,
    MemoryArena* const __restrict mem,
    size_t size,
    size_t max_size,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ASSERT_TRUE(size);
    ASSERT_TRUE(max_size >= size);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    size = align_up(size, (size_t) alignment);
    MemoryArena* arena = mem_arena_add(mem, size, max_size, alignment);
    ring->memory = arena->memory;

    ring->end = ring->memory + size;
    ring->head = ring->memory;
    ring->tail = ring->memory;
    ring->size = size;
    ring->alignment = alignment;

    DEBUG_MEMORY_SUBREGION((uintptr_t) ring->memory, ring->size);
}

inline
void ring_init(
    RingMemory* const __restrict ring,
    BufferMemory* const __restrict buf,
    size_t size,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ASSERT_TRUE(size);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    size = align_up(size, (size_t) alignment);
    ring->memory = buffer_get_memory(buf, size, alignment);

    ring->end = ring->memory + size;
    ring->head = ring->memory;
    ring->tail = ring->memory;
    ring->size = size;
    ring->alignment = alignment;

    DEBUG_MEMORY_SUBREGION((uintptr_t) ring->memory, ring->size);
}

inline
void ring_init(
    RingMemory* const __restrict ring,
    byte* const __restrict buf,
    size_t size,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ASSERT_TRUE(size);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    size = align_up(size, (size_t) alignment);
    ring->memory = (byte *) align_up((uintptr_t) buf, (size_t) alignment);

    ring->end = ring->memory + size;
    ring->head = ring->memory;
    ring->tail = ring->memory;
    ring->size = size;
    ring->alignment = alignment;

    DEBUG_MEMORY_SUBREGION((uintptr_t) ring->memory, ring->size);
}

FORCE_INLINE
void thrd_ring_alloc(RingMemory* const ring, size_t size, size_t max_size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ring_alloc(ring, size, max_size, alignment);
    mutex_init(&ring->lock, NULL);
}

FORCE_INLINE
void thrd_ring_alloc(
    RingMemory* const ring,
    MemoryArena* const __restrict mem,
    size_t size, size_t max_size,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ring_alloc(ring, mem, size, max_size, alignment);
    mutex_init(&ring->lock, NULL);
}

FORCE_INLINE
void thrd_ring_init(
    RingMemory* const ring,
    BufferMemory* const buf,
    size_t size,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ring_init(ring, buf, size, alignment);
    mutex_init(&ring->lock, NULL);
}

FORCE_INLINE
void thrd_ring_init(
    RingMemory* const ring,
    byte* const buf,
    size_t size,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ring_init(ring, buf, size, alignment);
    mutex_init(&ring->lock, NULL);
}

inline
void ring_free(RingMemory* const ring) NO_EXCEPT
{
    platform_aligned_free((void **) &ring->memory);

    ring->size = 0;
    ring->memory = NULL;
}

FORCE_INLINE
void thrd_ring_free(RingMemory* const ring) NO_EXCEPT
{
    ring_free(ring);
    mutex_destroy(&ring->lock);
}

inline
void ring_free(RingMemory* const ring, MemoryArena* mem) NO_EXCEPT
{
    mem_arena_remove(mem, ring->memory);

    ring->size = 0;
    ring->memory = NULL;
}

FORCE_INLINE
void thrd_ring_free(RingMemory* const ring, MemoryArena* mem) NO_EXCEPT
{
    ring_free(ring, mem);
    mutex_destroy(&ring->lock);
}

inline
byte* ring_calculate_position(
    const RingMemory* const ring,
    size_t size,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    byte* head = (byte *) align_up((uintptr_t) ring->head, alignment);
    size = align_up(size, (size_t) alignment);

    if (head + size > ring->end) { UNLIKELY
        head = (byte *) align_up((uintptr_t) ring->memory, alignment);
    }

    return head;
}

FORCE_INLINE
byte* thrd_ring_calculate_position(RingMemory* const ring, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    MutexGuard _guard(&ring->lock);
    return ring_calculate_position(ring, size, alignment);
}

FORCE_INLINE
void ring_reset(RingMemory* const ring) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE((uintptr_t) ring->memory, ring->size);
    ring->head = ring->memory;
}

FORCE_INLINE
void thrd_ring_reset(RingMemory* const ring) NO_EXCEPT
{
    MutexGuard _guard(&ring->lock);
    ring_reset(ring);
}

// Moves a pointer based on the size you want to consume (new position = after consuming size)
// Usually used to move head or tail pointer (generically called here = pos)
HOT_CODE
void ring_move_pointer(RingMemory* const ring, byte** pos, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ASSERT_TRUE(size <= ring->size);

    // Actually, we cannot be sure that this is a read, it could also be a write.
    // However, we better do it once here than manually in every place that uses this function
    DEBUG_MEMORY_READ((uintptr_t) *pos, size);

    *pos = (byte *) align_up((uintptr_t) *pos, alignment);
    size = align_up(size, (size_t) alignment);

    if (*pos + size > ring->end) { UNLIKELY
        *pos = (byte *) align_up((uintptr_t) ring->memory, alignment);
    }

    *pos += size;
}

FORCE_INLINE
void thrd_ring_move_pointer(RingMemory* const ring, byte** pos, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    MutexGuard _guard(&ring->lock);
    ring_move_pointer(ring, pos, size, alignment);
}

// @todo Implement a function called ring_grow_memory that tries to grow a memory range
// this of course is only possible if the memory range is the last memory range returned and if the growing part still fits into the ring
HOT_CODE
byte* ring_get_memory(RingMemory* const ring, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ASSERT_TRUE(size <= ring->size);

    size = align_up(size, (size_t) alignment);

    ring->head = (byte *) align_up((uintptr_t) ring->head, alignment);
    if (ring->head + size > ring->end) { UNLIKELY
        ring_reset(ring);

        ring->head = (byte *) align_up((uintptr_t) ring->head, alignment);
    }

    DEBUG_MEMORY_WRITE((uintptr_t) ring->head, size);

    byte* const offset = (byte *) ring->head;
    ring->head += size;

    ASSERT_TRUE(offset);

    return offset;
}

FORCE_INLINE
byte* thrd_ring_get_memory(RingMemory* const ring, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    MutexGuard _guard(&ring->lock);
    return ring_get_memory(ring, size, alignment);
}

byte* ring_grow_memory(RingMemory* const ring, const byte* old, size_t size_old, size_t size_new, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    size_new = align_up(size_new, alignment);

    const byte* const expected_head = old + align_up(size_old, alignment);

    // Check if we can grow in place (no allocations since old)
    if (expected_head == ring->head) {
        // Check if there's enough space left in current buffer
        if (ring->head + (size_new - size_old) <= ring->end) {
            DEBUG_MEMORY_WRITE((uintptr_t) ring->head, size_new - size_old);
            ring->head += (size_new - size_old);

            return (byte *) old;
        } else {
            // Not enough space at the end — wrap and reset
            // Allocate new space with ring_get_memory and copy over
            byte* const new_block = ring_get_memory(ring, size_new, alignment);
            memcpy(new_block, old, size_old);

            return new_block;
        }
    } else {
        // Some other allocations happened — must allocate new block
        byte* new_block = ring_get_memory(ring, size_new, alignment);
        memcpy(new_block, old, size_old);

        return new_block;
    }
}

FORCE_INLINE
byte* thrd_ring_grow_memory(RingMemory* const ring, const byte* old, size_t size_old, size_t size_new, int32 alignment = sizeof(size_t)) {
    MutexGuard _guard(&ring->lock);
    return ring_grow_memory(ring, old, size_old, size_new, alignment);
}

// Same as ring_get_memory but DOESN'T move the head
HOT_CODE
byte* ring_get_memory_nomove(RingMemory* const ring, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ASSERT_TRUE(size <= ring->size);

    byte* pos = (byte *) align_up((uintptr_t) ring->head, alignment);
    size = align_up(size, (size_t) alignment);

    if (pos + size > ring->end) { UNLIKELY
        ring_reset(ring);

        pos = (byte *) align_up((uintptr_t) pos, alignment);
    }

    DEBUG_MEMORY_WRITE((uintptr_t) pos, size);

    return pos;
}

FORCE_INLINE
byte* thrd_ring_get_memory_nomove(RingMemory* const ring, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    MutexGuard _guard(&ring->lock);
    return ring_get_memory_nomove(ring, size, alignment);
}

// Used if the ring only contains elements of a certain size
// This way you can get a certain element
FORCE_INLINE HOT_CODE
byte* ring_get_element(const RingMemory* const ring, uint64 element, size_t size) NO_EXCEPT
{
    DEBUG_MEMORY_READ((uintptr_t) (ring->memory + element * size), 1);

    return ring->memory + element * size;
}

FORCE_INLINE
byte* thrd_ring_get_element(RingMemory* const ring, uint64 element, size_t size) NO_EXCEPT
{
    MutexGuard _guard(&ring->lock);
    return ring_get_element(ring, element, size);
}

/**
 * Checks if one additional element can be inserted without overwriting the tail index
 */
inline HOT_CODE
bool ring_commit_safe(const RingMemory* const ring, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    // alignment * 2 since that should be the maximum overhead for an element
    // -1 since that is the worst case, we can't be missing a complete alignment because than it would be already alignment
    // This is not 100% correct BUT it is way faster than any correct version I can come up with
    const uint64 max_mem_required = size + (alignment - 1) * 2;

    // @question Is it beneficial to define the first if as LIKELY?
    // The first if should be the most likely in a situation with a decently fast runing dequeue
    if (ring->tail < ring->head) {
        return ((uint64) (ring->end - ring->head)) >= max_mem_required
            || ((uint64) (ring->tail - ring->memory)) >= max_mem_required;
    } else if (ring->tail > ring->head) {
        return ((uint64) (ring->tail - ring->head)) >= max_mem_required;
    }

    return true;
}

FORCE_INLINE
bool thrd_ring_commit_safe(RingMemory* const ring, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    MutexGuard _guard(&ring->lock);
    return ring_commit_safe(ring, size, alignment);
}

inline
int64 ring_dump(const RingMemory* const ring, byte* data) NO_EXCEPT
{
    const byte* const start = data;

    data = write_le(data, ring->size);
    data = write_le(data, (uint64) (ring->head - ring->memory));
    data = write_le(data, ring->alignment);
    data = write_le(data, (uint64) (ring->tail - ring->memory));
    data = write_le(data, (uint64) (ring->end - ring->memory));

    // All memory is handled in the buffer -> simply copy the buffer
    memcpy(data, ring->memory, ring->size);
    data += ring->size;

    return data - start;
}

FORCE_INLINE
int64 thrd_ring_dump(RingMemory* const ring, byte* data) NO_EXCEPT
{
    MutexGuard _guard(&ring->lock);
    return ring_dump(ring, data);
}

#endif
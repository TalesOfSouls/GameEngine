/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS__MEMORY_RING_MEMORY_H
#define COMS__MEMORY_RING_MEMORY_H

#include <string.h>
#include "../stdlib/Types.h"
#include "../utils/EndianUtils.h"
#include "../utils/Assert.h"

#include "BufferMemory.h"
#include "../log/Log.h"
#include "../log/Stats.h"
#include "../log/PerformanceProfiler.h"
#include "../log/DebugMemory.h"
#include "../thread/Atomic.h"
#include "../thread/Semaphore.h"
#include "../thread/ThreadDefines.h"
#include "../system/Allocator.h"

// WARNING: Changing this structure has effects on other data structures (e.g. Queue)
// When changing make sure you understand what you are doing
struct RingMemory {
    byte* memory;
    byte* end;

    byte* head;

    // This variable is usually only used by single producer/consumer code mostly found in threads.
    // One thread inserts elements -> updates head
    // The other thread reads elements -> updates tail
    // This code itself doesn't change this variable
    byte* tail;

    size_t size;
    uint32 alignment;
};

inline
void ring_alloc(RingMemory* ring, size_t size, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ASSERT_TRUE(size);
    ASSERT_TRUE(alignment % sizeof(int) == 0);
    PROFILE(PROFILE_RING_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);

    size = OMS_ALIGN_UP(size, 64);
    LOG_1("[INFO] Allocating RingMemory: %n B", {DATA_TYPE_UINT64, &size});

    ring->memory = alignment < 2
        ? (byte *) platform_alloc(size)
        : (byte *) platform_alloc_aligned(size, alignment);

    ring->end = ring->memory + size;
    ring->head = ring->memory;
    ring->tail = ring->memory;
    ring->size = size;
    ring->alignment = alignment;

    // @question why is this even here?
    compiler_memset_aligned_8(ring->memory, 0, ring->size);
}

inline
void ring_init(RingMemory* __restrict ring, BufferMemory* __restrict buf, size_t size, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ASSERT_TRUE(size);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    size = OMS_ALIGN_UP(size, (uint64) alignment);
    ring->memory = buffer_get_memory(buf, size, alignment);

    ring->end = ring->memory + size;
    ring->head = ring->memory;
    ring->tail = ring->memory;
    ring->size = size;
    ring->alignment = alignment;

    DEBUG_MEMORY_SUBREGION((uintptr_t) ring->memory, ring->size);
}

inline
void ring_init(RingMemory* __restrict ring, byte* __restrict buf, size_t size, uint32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ASSERT_TRUE(size);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    size = OMS_ALIGN_UP(size, (uint64) alignment);
    ring->memory = (byte *) OMS_ALIGN_UP((uintptr_t) buf, (uint64) alignment);

    ring->end = ring->memory + size;
    ring->head = ring->memory;
    ring->tail = ring->memory;
    ring->size = size;
    ring->alignment = alignment;

    memset(ring->memory, 0, ring->size);

    DEBUG_MEMORY_SUBREGION((uintptr_t) ring->memory, ring->size);
}

inline
void ring_free(RingMemory* ring) NO_EXCEPT
{
    if (ring->alignment < 2) {
        platform_free((void **) &ring->memory);
    } else {
        platform_aligned_free((void **) &ring->memory);
    }

    ring->size = 0;
    ring->memory = NULL;
}

inline
byte* ring_calculate_position(const RingMemory* ring, size_t size, uint64 aligned = 4) NO_EXCEPT
{
    byte* head = (byte *) OMS_ALIGN_UP((uintptr_t) ring->head, aligned);
    size = OMS_ALIGN_UP(size, (uint64) aligned);

    if (head + size > ring->end) { UNLIKELY
        head = (byte *) OMS_ALIGN_UP((uintptr_t) ring->memory, aligned);
    }

    return head;
}

FORCE_INLINE
void ring_reset(RingMemory* ring) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE((uintptr_t) ring->memory, ring->size);
    ring->head = ring->memory;
}

// Moves a pointer based on the size you want to consume (new position = after consuming size)
// Usually used to move head or tail pointer (generically called here = pos)
HOT_CODE
void ring_move_pointer(RingMemory* ring, byte** pos, size_t size, uint64 aligned = 4) NO_EXCEPT
{
    ASSERT_TRUE(size <= ring->size);

    // Actually, we cannot be sure that this is a read, it could also be a write.
    // However, we better do it once here than manually in every place that uses this function
    DEBUG_MEMORY_READ((uintptr_t) *pos, size);

    *pos = (byte *) OMS_ALIGN_UP((uintptr_t) *pos, aligned);
    size = OMS_ALIGN_UP(size, (uint64) aligned);

    if (*pos + size > ring->end) { UNLIKELY
        *pos = (byte *) OMS_ALIGN_UP((uintptr_t) ring->memory, aligned);
    }

    *pos += size;
}

// @todo Implement a function called ring_grow_memory that tries to grow a memory range
// this of course is only possible if the memory range is the last memory range returned and if the growing part still fits into the ring
HOT_CODE
byte* ring_get_memory(RingMemory* ring, size_t size, uint64 aligned = 4) NO_EXCEPT
{
    ASSERT_TRUE(size <= ring->size);

    ring->head = (byte *) OMS_ALIGN_UP((uintptr_t) ring->head, aligned);
    size = OMS_ALIGN_UP(size, (uint64) aligned);

    if (ring->head + size > ring->end) { UNLIKELY
        ring_reset(ring);

        ring->head = (byte *) OMS_ALIGN_UP((uintptr_t) ring->head, aligned);
    }

    DEBUG_MEMORY_WRITE((uintptr_t) ring->head, size);

    byte* offset = ring->head;
    ring->head += size;

    ASSERT_TRUE(offset);

    return offset;
}

byte* ring_grow_memory(RingMemory* ring, const byte* old, size_t size_old, size_t size_new, uint64 aligned = 4) NO_EXCEPT
{
    size_new = OMS_ALIGN_UP(size_new, aligned);

    const byte* expected_head = old + OMS_ALIGN_UP(size_old, aligned);

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
            byte* new_block = ring_get_memory(ring, size_new, aligned);
            memcpy(new_block, old, size_old);

            return new_block;
        }
    } else {
        // Some other allocations happened — must allocate new block
        byte* new_block = ring_get_memory(ring, size_new, aligned);
        memcpy(new_block, old, size_old);

        return new_block;
    }
}

// Same as ring_get_memory but DOESN'T move the head
HOT_CODE
byte* ring_get_memory_nomove(RingMemory* ring, size_t size, uint64 aligned = 4) NO_EXCEPT
{
    ASSERT_TRUE(size <= ring->size);

    byte* pos = (byte *) OMS_ALIGN_UP((uintptr_t) ring->head, aligned);
    size = OMS_ALIGN_UP(size, (uint64) aligned);

    if (pos + size > ring->end) { UNLIKELY
        ring_reset(ring);

        pos = (byte *) OMS_ALIGN_UP((uintptr_t) pos, aligned);
    }

    DEBUG_MEMORY_WRITE((uintptr_t) pos, size);

    return pos;
}

// Used if the ring only contains elements of a certain size
// This way you can get a certain element
FORCE_INLINE HOT_CODE
byte* ring_get_element(const RingMemory* ring, uint64 element, size_t size) NO_EXCEPT
{
    DEBUG_MEMORY_READ((uintptr_t) (ring->memory + element * size), 1);

    return ring->memory + element * size;
}

/**
 * Checks if one additional element can be inserted without overwriting the tail index
 */
inline HOT_CODE
bool ring_commit_safe(const RingMemory* ring, size_t size, uint64 aligned = 4) NO_EXCEPT
{
    // aligned * 2 since that should be the maximum overhead for an element
    // -1 since that is the worst case, we can't be missing a complete alignment because than it would be already aligned
    // This is not 100% correct BUT it is way faster than any correct version I can come up with
    const uint64 max_mem_required = size + (aligned - 1) * 2;

    // @question Is it beneficial to define the first if as LIKELY?
    // The first if should be the most likely in a situation with a decently fast runing dequeue
    if (ring->tail < ring->head) {
        return ((uint64) (ring->end - ring->head)) >= max_mem_required
            || ((uint64) (ring->tail - ring->memory)) >= max_mem_required;
    } else if (ring->tail > ring->head) {
        return ((uint64) (ring->tail - ring->head)) >= max_mem_required;
    } else { UNLIKELY
        return true;
    }
}

inline HOT_CODE
bool ring_commit_safe_atomic(const RingMemory* ring, size_t size, uint64 aligned = 4) NO_EXCEPT
{
    // aligned * 2 since that should be the maximum overhead for an element
    // -1 since that is the worst case, we can't be missing a complete alignment because than it would be already aligned
    // This is not 100% correct BUT it is way faster than any correct version I can come up with
    const uint64 max_mem_required = size + (aligned - 1) * 2;

    // @todo consider to switch to uintptr_t
    const uint64 tail = (uint64) atomic_get_relaxed((void **) &ring->tail);

    // This doesn't have to be atomic since we assume single producer/consumer and a commit is performed by the consumer
    const uint64 head = (uint64) ring->head;

    // @question Is it beneficial to define the first if as LIKELY?
    // The first if should be the most likely in a situation with a decently fast runing dequeue
    if (tail < head) {
        return ((uint64) (ring->end - head)) >= max_mem_required
            || ((uint64) (tail - (uint64) ring->memory)) >= max_mem_required;
    } else if (tail > head) {
        return ((uint64) (tail - head)) >= max_mem_required;
    } else { UNLIKELY
        return true;
    }
}

inline
int64 ring_dump(const RingMemory* ring, byte* data) NO_EXCEPT
{
    byte* start = data;

    // Size
    *((uint64 *) data) = SWAP_ENDIAN_LITTLE(ring->size);
    data += sizeof(ring->size);

    // head
    *((uint64 *) data) = SWAP_ENDIAN_LITTLE((uint64) (ring->head - ring->memory));
    data += sizeof(ring->head);

    // Alignment
    *((int32 *) data) = SWAP_ENDIAN_LITTLE(ring->alignment);
    data += sizeof(ring->alignment);

    // tail/End
    *((uint64 *) data) = SWAP_ENDIAN_LITTLE((uint64) (ring->tail - ring->memory));
    data += sizeof(ring->tail);

    *((uint64 *) data) = SWAP_ENDIAN_LITTLE((uint64) (ring->end - ring->memory));
    data += sizeof(ring->end);

    // All memory is handled in the buffer -> simply copy the buffer
    memcpy(data, ring->memory, ring->size);
    data += ring->size;

    return data - start;
}

#endif
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
#include "../utils/TestUtils.h"

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

    uint64 size;
    uint32 alignment;
};

inline
void ring_alloc(RingMemory* ring, uint64 size, uint32 alignment = 64)
{
    ASSERT_SIMPLE(size);
    PROFILE(PROFILE_RING_ALLOC, NULL, false, true);
    LOG_1("[INFO] Allocating RingMemory: %n B", {{LOG_DATA_UINT64, &size}});

    ring->memory = alignment < 2
        ? (byte *) platform_alloc(size)
        : (byte *) platform_alloc_aligned(size, alignment);

    ring->end = ring->memory + size;
    ring->head = ring->memory;
    ring->tail = ring->memory;
    ring->size = size;
    ring->alignment = alignment;

    memset(ring->memory, 0, ring->size);
}

inline
void ring_init(RingMemory* ring, BufferMemory* buf, uint64 size, uint32 alignment = 64)
{
    ASSERT_SIMPLE(size);

    ring->memory = buffer_get_memory(buf, size, alignment, true);

    ring->end = ring->memory + size;
    ring->head = ring->memory;
    ring->tail = ring->memory;
    ring->size = size;
    ring->alignment = alignment;

    DEBUG_MEMORY_SUBREGION((uintptr_t) ring->memory, ring->size);
}

inline
void ring_init(RingMemory* ring, byte* buf, uint64 size, uint32 alignment = 64)
{
    ASSERT_SIMPLE(size);

    ring->memory = (byte *) ROUND_TO_NEAREST((uintptr_t) buf, (uint64) alignment);

    ring->end = ring->memory + size;
    ring->head = ring->memory;
    ring->tail = ring->memory;
    ring->size = size;
    ring->alignment = alignment;

    memset(ring->memory, 0, ring->size);

    DEBUG_MEMORY_SUBREGION((uintptr_t) ring->memory, ring->size);
}

inline
void ring_free(RingMemory* ring)
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
byte* ring_calculate_position(const RingMemory* ring, uint64 size, uint32 aligned = 4) noexcept
{
    byte* head = ring->head;

    if (aligned > 1) {
        uintptr_t address = (uintptr_t) head;
        head += (aligned - (address & (aligned - 1))) % aligned;
    }

    size = ROUND_TO_NEAREST(size, (uint64) aligned);
    if (head + size > ring->end) {
        head = ring->memory;

        if (aligned > 1) {
            uintptr_t address = (uintptr_t) head;
            head += (aligned - (address & (aligned - 1))) % aligned;
        }
    }

    return head;
}

inline
void ring_reset(RingMemory* ring) noexcept
{
    DEBUG_MEMORY_DELETE((uintptr_t) ring->memory, ring->size);
    ring->head = ring->memory;
}

// Moves a pointer based on the size you want to consume (new position = after consuming size)
// Usually used to move head or tail pointer (= pos)
void ring_move_pointer(RingMemory* ring, byte** pos, uint64 size, uint32 aligned = 4) noexcept
{
    ASSERT_SIMPLE(size <= ring->size);

    // Actually, we cannot be sure that this is a read, it could also be a write.
    // However, we better do it once here than manually in every place that uses this function
    DEBUG_MEMORY_READ((uintptr_t) *pos, size);

    if (aligned > 1) {
        uintptr_t address = (uintptr_t) *pos;
        *pos += (aligned - (address& (aligned - 1))) % aligned;
    }

    size = ROUND_TO_NEAREST(size, (uint64) aligned);
    if (*pos + size > ring->end) {
        *pos = ring->memory;

        if (aligned > 1) {
            uintptr_t address = (uintptr_t) *pos;
            *pos += (aligned - (address & (aligned - 1))) % aligned;
        }
    }

    *pos += size;
}

// @todo Implement a function called ring_grow_memory that tries to grow a memory range
// this of course is only possible if the memory range is the last memory range returned and if the growing part still fits into the ring
byte* ring_get_memory(RingMemory* ring, uint64 size, uint32 aligned = 4, bool zeroed = false) noexcept
{
    ASSERT_SIMPLE(size <= ring->size);

    if (aligned > 1) {
        uintptr_t address = (uintptr_t) ring->head;
        ring->head += (aligned - (address& (aligned - 1))) % aligned;
    }

    size = ROUND_TO_NEAREST(size, (uint64) aligned);
    if (ring->head + size > ring->end) {
        ring_reset(ring);

        if (aligned > 1) {
            uintptr_t address = (uintptr_t) ring->head;
            ring->head += (aligned - (address & (aligned - 1))) % aligned;
        }
    }

    if (zeroed) {
        memset((void *) ring->head, 0, size);
    }

    DEBUG_MEMORY_WRITE((uintptr_t) ring->head, size);

    byte* offset = ring->head;
    ring->head += size;

    ASSERT_SIMPLE(offset);

    return offset;
}

// Same as ring_get_memory but DOESN'T move the head
byte* ring_get_memory_nomove(RingMemory* ring, uint64 size, uint32 aligned = 4, bool zeroed = false) noexcept
{
    ASSERT_SIMPLE(size <= ring->size);

    byte* pos = ring->head;

    if (aligned > 1) {
        uintptr_t address = (uintptr_t) pos;
        pos += (aligned - (address& (aligned - 1))) % aligned;
    }

    size = ROUND_TO_NEAREST(size, (uint64) aligned);
    if (pos + size > ring->end) {
        ring_reset(ring);

        if (aligned > 1) {
            uintptr_t address = (uintptr_t) pos;
            pos += (aligned - (address & (aligned - 1))) % aligned;
        }
    }

    if (zeroed) {
        memset((void *) pos, 0, size);
    }

    DEBUG_MEMORY_WRITE((uintptr_t) pos, size);

    return pos;
}

// Used if the ring only contains elements of a certain size
// This way you can get a certain element
inline
byte* ring_get_element(const RingMemory* ring, uint64 element, uint64 size) noexcept
{
    DEBUG_MEMORY_READ((uintptr_t) (ring->memory + element * size), 1);

    return ring->memory + element * size;
}

/**
 * Checks if one additional element can be inserted without overwriting the tail index
 */
inline
bool ring_commit_safe(const RingMemory* ring, uint64 size, uint32 aligned = 4) noexcept
{
    // aligned * 2 since that should be the maximum overhead for an element
    // -1 since that is the worst case, we can't be missing a complete alignment because than it would be already aligned
    // This is not 100% correct BUT it is way faster than any correct version I can come up with
    uint64 max_mem_required = size + (aligned - 1) * 2;

    if (ring->tail < ring->head) {
        return ((uint64) (ring->end - ring->head)) >= max_mem_required
            || ((uint64) (ring->tail - ring->memory)) >= max_mem_required;
    } else if (ring->tail > ring->head) {
        return ((uint64) (ring->tail - ring->head)) >= max_mem_required;
    } else {
        return true;
    }
}

inline
bool ring_commit_safe_atomic(const RingMemory* ring, uint64 size, uint32 aligned = 4) noexcept
{
    // aligned * 2 since that should be the maximum overhead for an element
    // -1 since that is the worst case, we can't be missing a complete alignment because than it would be already aligned
    // This is not 100% correct BUT it is way faster than any correct version I can come up with
    uint64 max_mem_required = size + (aligned - 1) * 2;

    // @todo consider to switch to uintptr_t
    uint64 tail = (uint64) atomic_get_relaxed((void **) &ring->tail);

    // This doesn't have to be atomic since we assume single producer/consumer and a commit is performed by the consumer
    uint64 head = (uint64) ring->head;

    if (tail < head) {
        return ((uint64) (ring->end - head)) >= max_mem_required
            || ((uint64) (tail - (uint64) ring->memory)) >= max_mem_required;
    } else if (tail > head) {
        return ((uint64) (tail - head)) >= max_mem_required;
    } else {
        return true;
    }
}

inline
int64 ring_dump(const RingMemory* ring, byte* data)
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
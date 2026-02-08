/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MEMORY_CHUNK_MEMORY_H
#define COMS_MEMORY_CHUNK_MEMORY_H

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

/**
 * This storage system is best used for fixed sized chunks
 * What I mean by that, every element has the same size.
 * Currently a caller could reserve multiple chunks to represent a single data entity
 * This can be fine in a single threaded application
 * However, this can lead to fragmentation which is hard to clean up because
 * we can't just defragment the memory since we don't know which chunks are currently in use
 * In use could mean by pointer of id. In use data isn't allowed to move or it would become "invalid"
 * If you need a data structure that can be defragmented use DataPool, which basically builds upon ChunkMemory
 * Fixed sized data structures that use this ChunkMemory can be:
 *      1. HashMap
 *      2. Queue
 * Carefull, both examples have alternative use cases which may require variable sized elements
 * WARNING: Changing this struct has effects on other data structures
 */
// @performance Are we losing a lot of performance by using atomic_ (= volatile) in single threaded use cases
struct ChunkMemory {
    byte* memory;

    // @question Do I really want to use uint?
    uint_max size;
    atomic_32 int32 last_pos;
    uint32 capacity;
    int32 chunk_size;

    // WARNING: The alignment may increase the original chunk size e.g.
    // element_size = 14, alignment = sizeof(size_t) => chunk_size = 32
    uint32 alignment;

    // length = count
    // free describes which locations are used and which are free
    atomic_ptr uint_max* free;

    // Chunk implementation ends here
    // The completeness indicates if the data is completely written to
    atomic_ptr uint_max* completeness;

    mutex lock;
};

FORCE_INLINE
int32 chunk_size_element(int32 element_size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    return align_up(element_size, alignment);
}

FORCE_INLINE
uint_max chunk_size_total(uint32 capacity, int32 element_size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    element_size = chunk_size_element(element_size, alignment);

    // @performance Can we remove the alignment * 2? This is just a shotgun method to ensure full alignment

    return capacity * element_size
        + sizeof(uint_max) * ceil_div(capacity, (uint32) (sizeof(uint_max) * 8)) // free
        + alignment * 2; // overhead for alignment
}

FORCE_INLINE
uint_max thrd_chunk_size_total(uint32 capacity, int32 element_size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    element_size = chunk_size_element(element_size, alignment);

    // @performance Can we remove the alignment * 2? This is just a shotgun method to ensure full alignment

    const size_t array_count = ceil_div(capacity, (uint32) (sizeof(uint_max) * 8));

    return capacity * element_size
        + sizeof(uint_max) * array_count // free
        + sizeof(uint_max) * array_count // completeness
        + alignment * 2; // overhead for alignment
}

// INFO: A chunk count of 2^n is recommended for maximum performance
// @performance Do we maybe want to define both the element alignment and memory start alignment (e.g. 64 byte = cache line alignment for the memory)
inline
void chunk_alloc(ChunkMemory* const buf, uint32 capacity, int32 element_size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE(PROFILE_CHUNK_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemory");

    element_size = chunk_size_element(element_size, alignment);
    const uint_max size = chunk_size_total(capacity, element_size, alignment);

    buf->memory = alignment < 2
        ? (byte *) platform_alloc(size)
        : (byte *) platform_alloc_aligned(size, size, alignment);

    // @question Why don't I memset the memory to 0 here, but do it in chunk_init

    buf->capacity = capacity;
    buf->size = size;
    buf->chunk_size = element_size;
    buf->last_pos = -1;
    buf->alignment = alignment;
    buf->free = (uint_max *) align_up(
        (uint_max) ((uintptr_t) (buf->memory + capacity * element_size)),
        (uint_max) alignment
    );
    memset((void *) buf->free, 0, sizeof(uint_max) * ceil_div(capacity, (uint32) (sizeof(uint_max) * 8)));

    LOG_1("[INFO] Allocated ChunkMemory: %n B", {DATA_TYPE_UINT64, &buf->size});
}

inline
void thrd_chunk_alloc(ChunkMemory* const buf, uint32 capacity, int32 element_size, int32 alignment = 32)
{
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    PROFILE(PROFILE_CHUNK_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("[INFO] Allocating ChunkMemory");

    element_size = chunk_size_element(element_size, alignment);
    const uint_max size = thrd_chunk_size_total(capacity, element_size, alignment);

    buf->memory = alignment < 2
        ? (byte *) platform_alloc(size)
        : (byte *) platform_alloc_aligned(size, size, alignment);

    buf->capacity = capacity;
    buf->size = size;
    buf->chunk_size = element_size;
    buf->last_pos = -1;
    buf->alignment = alignment;

    const size_t array_count = ceil_div(capacity, (uint32) (sizeof(uint_max) * 8));

    // @question Could it be beneficial to have this before the element data?
    buf->free = (uint_max *) align_up((uintptr_t) (buf->memory + capacity * element_size), alignment);
    buf->completeness = (uint_max *) align_up((uintptr_t) (buf->free + array_count), alignment);

    memset((void *) buf->free, 0, sizeof(uint_max) * array_count);
    memset((void *) buf->completeness, 0, sizeof(uint_max) * array_count);

    mutex_init(&buf->lock, NULL);

    LOG_1("[INFO] Allocated ChunkMemory: %n B", {DATA_TYPE_UINT64, &buf->size});
}

inline
void chunk_init(
    ChunkMemory* const buf,
    BufferMemory* const data,
    uint32 capacity,
    int32 element_size,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    element_size = chunk_size_element(element_size, alignment);

    const uint_max size = chunk_size_total(capacity, element_size, alignment);

    buf->memory = buffer_get_memory(data, size, alignment);
    memset(buf->memory, 0, size);

    buf->capacity = capacity;
    buf->size = size;
    buf->chunk_size = element_size;
    buf->last_pos = -1;
    buf->alignment = alignment;
    buf->free = (uint_max *) align_up((uintptr_t) (buf->memory + capacity * element_size), (sizeof(uint_max) * 8));
    memset((void *) buf->free, 0, sizeof(uint_max) * ceil_div(capacity, (uint32) (sizeof(uint_max) * 8)));

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

inline
void thrd_chunk_alloc(
    ChunkMemory* const buf,
    BufferMemory* const data,
    uint32 capacity,
    int32 element_size,
    int32 alignment = sizeof(size_t)
)
{
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemory");

    element_size = chunk_size_element(element_size, alignment);
    const uint_max size = thrd_chunk_size_total(capacity, element_size, alignment);

    buf->memory = buffer_get_memory(data, size);

    buf->capacity = capacity;
    buf->size = size;
    buf->chunk_size = element_size;
    buf->last_pos = -1;
    buf->alignment = alignment;

    const size_t array_count = ceil_div(capacity, (uint32) (sizeof(uint_max) * 8));

    // @question Could it be beneficial to have this before the element data?
    buf->free = (uint_max *) align_up((uintptr_t) (buf->memory + capacity * element_size), alignment);
    buf->completeness = (uint_max *) align_up((uintptr_t) (buf->free + array_count), alignment);

    memset((void *) buf->free, 0, sizeof(uint_max) * array_count);
    memset((void *) buf->completeness, 0, sizeof(uint_max) * array_count);

    mutex_init(&buf->lock, NULL);

    LOG_1("[INFO] Allocated ChunkMemory: %n B", {DATA_TYPE_UINT64, &buf->size});
}

inline
void chunk_init(
    ChunkMemory* const buf,
    byte* const data,
    uint32 capacity,
    int32 element_size,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    element_size = chunk_size_element(element_size, alignment);

    const uint_max size = chunk_size_total(capacity, element_size, alignment);

    buf->memory = (byte *) align_up((uintptr_t) data, alignment);

    buf->capacity = capacity;
    buf->size = size;
    buf->chunk_size = element_size;
    buf->last_pos = -1;
    buf->alignment = alignment;
    buf->free = (uint_max *) align_up(
        (uintptr_t) (buf->memory + capacity * element_size),
        (uint_max) alignment
    );
    memset((void *) buf->free, 0, sizeof(uint_max) * ceil_div(capacity, (uint32) (sizeof(uint_max) * 8)));

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

inline
void thrd_chunk_alloc(
    ChunkMemory* const buf,
    byte* const data,
    uint32 capacity,
    int32 element_size,
    int32 alignment = sizeof(size_t)
)
{
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemory");

    element_size = chunk_size_element(element_size, alignment);
    const uint_max size = thrd_chunk_size_total(capacity, element_size, alignment);

    buf->memory = data;

    buf->capacity = capacity;
    buf->size = size;
    buf->chunk_size = element_size;
    buf->last_pos = -1;
    buf->alignment = alignment;

    const size_t array_count = ceil_div(capacity, (uint32) (sizeof(uint_max) * 8));

    // @question Could it be beneficial to have this before the element data?
    buf->free = (uint_max *) align_up((uintptr_t) (buf->memory + capacity * element_size), alignment);
    buf->completeness = (uint_max *) align_up((uintptr_t) (buf->free + array_count), alignment);

    memset((void *) buf->free, 0, sizeof(uint_max) * array_count);
    memset((void *) buf->completeness, 0, sizeof(uint_max) * array_count);

    mutex_init(&buf->lock, NULL);

    LOG_1("[INFO] Allocated ChunkMemory: %n B", {DATA_TYPE_UINT64, &buf->size});
}

inline
void chunk_free(ChunkMemory* const buf) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE((uintptr_t) buf->memory, buf->size);

    if (buf->alignment < 2) {
        platform_free((void **) &buf->memory);
    } else {
        platform_aligned_free((void **) &buf->memory);
    }

    buf->size = 0;
    buf->memory = NULL;
}

FORCE_INLINE
void thrd_chunk_free(ChunkMemory* const buf) NO_EXCEPT
{
    chunk_free(buf);
    mutex_destroy(&buf->lock);
}

FORCE_INLINE
uint_max* chunk_find_free_array(const ChunkMemory* const buf) NO_EXCEPT
{
    return (uint_max *) align_up((uintptr_t) (buf->memory + buf->capacity * buf->chunk_size), (uint_max) buf->alignment);
}

FORCE_INLINE
uint32 chunk_id_from_memory(uintptr_t memory, uintptr_t pos, size_t chunk_size) NO_EXCEPT
{
    return (uint32) ((pos - memory) / chunk_size);
}

inline
void thrd_chunk_set_unset_atomic(uint32 element, uint_max* state) NO_EXCEPT
{
    const int32 free_index = element / (sizeof(uint_max) * 8);
    const int32 bit_index = MODULO_2(element, (sizeof(uint_max) * 8));

    const uint_max mask = ~(OMS_UINT_ONE << bit_index);
    atomic_fetch_and_release(&state[free_index], mask);
}

FORCE_INLINE FORCE_FLATTEN
byte* chunk_get_element(const ChunkMemory* const buf, uint32 element) NO_EXCEPT
{
    // @question How is this even possible? Isn't an assert enough?
    if (element >= buf->capacity) {
        ASSERT_TRUE(element < buf->capacity);
        return NULL;
    }

    byte* const offset = buf->memory + element * buf->chunk_size;
    ASSERT_TRUE(offset);

    DEBUG_MEMORY_READ((uintptr_t) offset, buf->chunk_size);

    return offset;
}

FORCE_INLINE
bool chunk_is_free_internal(const uint_max* const state, uint32 element) NO_EXCEPT
{
    const uint32 free_index = element / (sizeof(uint_max) * 8);
    const uint32 bit_index = MODULO_2(element, (sizeof(uint_max) * 8));

    return !IS_BIT_SET_R2L(state[free_index], bit_index);
}

FORCE_INLINE
bool chunk_is_free(const ChunkMemory* const buf, uint32 element) NO_EXCEPT
{
    return chunk_is_free_internal(buf->free, element);
}

FORCE_INLINE
bool thrd_chunk_is_free_atomic_internal(const uint_max* const state, uint32 element) NO_EXCEPT
{
    const uint32 free_index = element / (sizeof(uint_max) * 8);
    const uint32 bit_index = MODULO_2(element, (sizeof(uint_max) * 8));

    uint_max mask = atomic_get_acquire(&state[free_index]);

    return !IS_BIT_SET_R2L(mask, bit_index);
}

FORCE_INLINE
bool thrd_chunk_is_free_atomic(const ChunkMemory* const buf, uint32 element) NO_EXCEPT
{
    return thrd_chunk_is_free_atomic_internal(buf->free, element);;
}

// This is effectively the same as reserve with elements = 1 which allows for some performance improvements
// state_count = number of maximum elements in the state array.
// @todo change state_count to int
HOT_CODE FORCE_FLATTEN
int32 chunk_reserve_one(uint_max* state, uint32 state_count, int32 start_index = 0) NO_EXCEPT
{
    if ((uint32) start_index >= state_count) { UNLIKELY
        start_index = 0;
    }

    uint32 free_index = start_index / (sizeof(uint_max) * 8);
    uint32 bit_index = MODULO_2(start_index, (sizeof(uint_max) * 8));

    // Check standard simple solution
    if (!IS_BIT_SET_R2L(state[free_index], bit_index)) {
        state[free_index] |= (OMS_UINT_ONE << bit_index);

        return start_index;
    }

    for (uint32 i = 0; i < state_count; i+= (sizeof(uint_max) * 8)) {
        if (state[free_index] != OMS_UINT_MAX) {
            // @bug This doesn't return the next best element in a hash map case
            // In a hash map we want the next free element AFTER start_index
            // However, this below may return a previous element since it ignores the start_index
            // The reason why we want the next best element is because it is faster to iterate (cache locality)
            bit_index = compiler_find_first_bit_r2l(~state[free_index]);

            const uint32 id = free_index * (sizeof(uint_max) * 8) + bit_index;
            if (id >= state_count) { UNLIKELY
                free_index = 0;

                continue;
            }

            state[free_index] |= (OMS_UINT_ONE << bit_index);

            return id;
        } else {
            ++free_index;
            if (free_index * (sizeof(uint_max) * 8) >= state_count) {
                free_index = 0;
            }
        }
    }

    return -1;
}

HOT_CODE
int32 thrd_chunk_reserve_one_atomic(uint_max* state, uint32 state_count, int32 start_index = 0) NO_EXCEPT
{
    if ((uint32) start_index >= state_count) {
        start_index = 0;
    }

    uint32 free_index = start_index / (sizeof(uint_max) * 8);
    uint32 bit_index = MODULO_2(start_index, (sizeof(uint_max) * 8));

    // Check standard simple solution
    uint_max current = atomic_get_acquire(&state[free_index]);
    if (!(current & (OMS_UINT_ONE << bit_index))) {
        uint_max desired = current | (OMS_UINT_ONE << bit_index);
        if (atomic_compare_exchange_strong_acquire_release(&state[free_index], current, desired) == current) {
            return free_index * (sizeof(uint_max) * 8) + bit_index;
        }
    }

    for (uint32 i = 0; i < state_count; i += (sizeof(uint_max) * 8)) {
        uint_max current_free = atomic_get_acquire(&state[free_index]);
        if (current_free != OMS_UINT_MAX) {
            uint_max inverted = ~current_free;

            int32 j = 0; // We will only try 3 times to avoid infinite or long loops
            while (j < 3 && (bit_index = compiler_find_first_bit_r2l(inverted)) >= 0) {
                uint32 id = free_index * (sizeof(uint_max) * 8) + bit_index;
                if (id >= state_count) {
                    free_index = 0;

                    break;
                }

                uint_max new_free = current_free | (OMS_UINT_ONE << bit_index);
                if ((new_free = atomic_compare_exchange_strong_acquire_release(&state[free_index], current_free, new_free)) == current_free) {
                    return id;
                }

                inverted = ~new_free;
                ++j;
            }
        } else {
            ++free_index;
            if (free_index * (sizeof(uint_max) * 8) >= state_count) {
                free_index = 0;
            }
        }
    }

    return -1;
}

FORCE_INLINE
int32 chunk_reserve_one(ChunkMemory* const buf) NO_EXCEPT
{
    return chunk_reserve_one(buf->free, buf->capacity, buf->last_pos);
}

FORCE_INLINE
int32 thrd_chunk_reserve_one_atomic(ChunkMemory* const buf) NO_EXCEPT
{
    int32 last = atomic_fetch_increment_wrap_release(&buf->last_pos, -1, (int32) buf->capacity);

    return thrd_chunk_reserve_one_atomic(buf->free, buf->capacity, last);
}

HOT_CODE FORCE_FLATTEN
int32 chunk_reserve_internal(uint_max* const state, uint32 capacity, int32 last_pos, uint32 elements = 1) NO_EXCEPT
{
    ASSERT_TRUE(elements > 0);

    if ((uint32) (last_pos + 1) >= capacity) { UNLIKELY
        last_pos = -1;
    }

    uint32 free_index = (last_pos + 1) / (sizeof(uint_max) * 8);
    uint32 bit_index = MODULO_2(last_pos + 1, (sizeof(uint_max) * 8));

    // Check standard simple solution
    if (elements == 1 && !IS_BIT_SET_R2L(state[free_index], bit_index)) {
        state[free_index] |= (OMS_UINT_ONE << bit_index);

        return ++last_pos;
    }

    int32 free_element = -1;
    uint32 i = 0;
    uint32 consecutive_free_bits = 0;

    while (i++ <= capacity) {
        if (state[free_index] == OMS_UINT_MAX) {
            // Skip fully filled ranges
            ++free_index;
            bit_index = 0;
            i += (sizeof(uint_max) * 8);
            consecutive_free_bits = 0;

            continue;
        } else if (free_index * (sizeof(uint_max) * 8) + bit_index + elements - consecutive_free_bits > capacity) { UNLIKELY
            // Go to beginning after overflow
            i += capacity - (free_index * (sizeof(uint_max) * 8) + bit_index);
            consecutive_free_bits = 0;
            free_index = 0;
            bit_index = 0;

            continue;
        }

        // Find first free element
        // This MUST find a free element, otherwise we wouldn't have gotten here
        bit_index = compiler_find_first_bit_r2l(~state[free_index]);

        // Let's check if we have enough free space, we need more than just one free bit
        do {
            ++i;
            ++consecutive_free_bits;
            ++bit_index;

            if (bit_index > (sizeof(uint_max) * 8 - 1)) {
                bit_index = 0;
                ++free_index;

                break;
            }
        } while (!IS_BIT_SET_R2L(state[free_index], bit_index)
            && consecutive_free_bits != elements
            && free_index * (sizeof(uint_max) * 8) + bit_index + elements - consecutive_free_bits <= capacity
            && i <= capacity
        );

        // Do we have enough free bits?
        if (consecutive_free_bits == elements) {
            free_element = free_index * (sizeof(uint_max) * 8) + bit_index - elements;
            const uint32 possible_free_index = free_element / (sizeof(uint_max) * 8);
            const uint32 possible_bit_index = MODULO_2(free_element, (sizeof(uint_max) * 8));

            // Mark as used
            if (elements == 1) {
                state[possible_free_index] |= (OMS_UINT_ONE << possible_bit_index);
            } else {
                uint32 elements_temp = elements;
                uint_max current_free_index = possible_free_index;
                uint32 current_bit_index = possible_bit_index;

                while (elements_temp > 0) {
                    // Calculate the number of bits we can set in the current 64-bit block
                    uint32 bits_in_current_block = OMS_MIN(((uint32) (sizeof(uint_max) * 8) - current_bit_index), elements_temp);

                    // Create a mask to set the bits
                    uint_max mask = ((OMS_UINT_ONE << (bits_in_current_block & (sizeof(uint_max) * 8 - 1))) - 1) << current_bit_index | ((bits_in_current_block >> 6) * ((uint_max) -1));
                    state[current_free_index] |= mask;

                    // Update the counters and indices
                    elements_temp -= bits_in_current_block;
                    ++current_free_index;
                    current_bit_index = 0;
                }
            }

            break;
        }
    }

    if (free_element < 0) { UNLIKELY
        LOG_3("No free chunk memory index found");

        // This shouldn't happen in an ideal world and we should adjust our code
        ASSERT_TRUE_CONST(false);

        return -1;
    }

    return free_element;
}

// use chunk_reserve_one if possible
HOT_CODE FORCE_FLATTEN
int32 chunk_reserve(ChunkMemory* const buf, uint32 elements = 1) NO_EXCEPT
{
    buf->last_pos = chunk_reserve_internal(buf->free, buf->capacity, buf->last_pos, elements);

    DEBUG_MEMORY_WRITE((uintptr_t) (buf->memory + buf->last_pos * buf->chunk_size), elements * buf->chunk_size);

    return buf->last_pos;
}


FORCE_INLINE
int32 thrd_chunk_reserve(ChunkMemory* const buf, uint32 elements = 1) NO_EXCEPT
{
    MutexGuard _guard(&buf->lock);
    return chunk_reserve((ChunkMemory *) buf, elements);
}

FORCE_INLINE
void chunk_free_element(uint_max* const state, uint_max free_index, int32 bit_index) NO_EXCEPT
{
    state[free_index] &= ~(OMS_UINT_ONE << bit_index);
}

FORCE_INLINE
void chunk_free_element(ChunkMemory* const buf, uint_max free_index, int32 bit_index) NO_EXCEPT
{
    buf->free[free_index] &= ~(OMS_UINT_ONE << bit_index);
    DEBUG_MEMORY_DELETE(
        (uintptr_t) (buf->memory + (free_index * (sizeof(uint_max) * 8) + bit_index) * buf->chunk_size),
        buf->chunk_size
    );
}

inline
void thrd_chunk_free_element_internal(uint_max* const state, uint_max free_index, int32 bit_index) NO_EXCEPT
{
    atomic_max uint_max* target = &state[free_index];
    uint_max old_value, new_value;

    do {
        old_value = atomic_get_relaxed(target);
        new_value = old_value | (OMS_UINT_ONE << bit_index);

        if (old_value == new_value) {
            return;
        }
        // @bug Wrong use
    } while (!atomic_compare_exchange_strong_release(target, old_value, new_value));
}

inline
void thrd_chunk_free_element(ChunkMemory* const buf, uint_max free_index, int32 bit_index) NO_EXCEPT
{
    thrd_chunk_free_element_internal(buf->free, free_index, bit_index);
    DEBUG_MEMORY_DELETE(
        (uintptr_t) (buf->memory + (free_index * (sizeof(uint_max) * 8) + bit_index) * buf->chunk_size),
        buf->chunk_size
    );
}

FORCE_INLINE
void chunk_free_element(ChunkMemory* const buf, uint32 element) NO_EXCEPT
{
    const uint_max free_index = element / (sizeof(uint_max) * 8);
    const uint32 bit_index = MODULO_2(element, (sizeof(uint_max) * 8));
    buf->free[free_index] &= ~(OMS_UINT_ONE << bit_index);

    DEBUG_MEMORY_DELETE(
        (uintptr_t) (buf->memory + (free_index * (sizeof(uint_max) * 8) + bit_index) * buf->chunk_size),
        buf->chunk_size
    );
}

HOT_CODE
void chunk_free_elements_internal(uint_max* const state, uint32 element, uint32 element_count = 1) NO_EXCEPT
{
    uint_max free_index = element / (sizeof(uint_max) * 8);
    uint32 bit_index = MODULO_2(element, (sizeof(uint_max) * 8));

    if (element == 1) {
        chunk_free_element(state, free_index, bit_index);
        return;
    }

    while (element_count > 0) {
        // Calculate the number of bits we can clear in the current 64-bit block
        const uint32 bits_in_current_block = OMS_MIN((uint32) ((sizeof(uint_max) * 8) - bit_index), element_count);

        // Create a mask to clear the bits
        const uint_max mask = ((OMS_UINT_ONE << bits_in_current_block) - 1) << bit_index;
        state[free_index] &= ~mask;

        // Update the counters and indices
        element_count -= bits_in_current_block;
        ++free_index;
        bit_index = 0;
    }
}

HOT_CODE
void chunk_free_elements(ChunkMemory* const buf, uint32 element, uint32 element_count = 1) NO_EXCEPT
{
    chunk_free_elements_internal(buf->free, element, element_count);
    DEBUG_MEMORY_DELETE((uintptr_t) (buf->memory + element * buf->chunk_size), buf->chunk_size * element_count);
}

inline
void thrd_chunk_free_elements_atomic_internal(uint_max* const state, uint_max element, uint32 element_count = 1) NO_EXCEPT
{
    uint_max free_index = element / (sizeof(uint_max) * 8);
    uint32 bit_index = MODULO_2(element, (sizeof(uint_max) * 8));

    if (element == 1) {
        thrd_chunk_free_element_internal(state, free_index, bit_index);
        return;
    }

    while (element_count > 0) {
        // Calculate the number of bits we can clear in the current 64-bit block
        uint32 bits_in_current_block = OMS_MIN((uint32) ((sizeof(uint_max) * 8) - bit_index), element_count);

        // Create a mask to clear the bits
        uint_max mask = ((OMS_UINT_ONE << bits_in_current_block) - 1) << bit_index;

        uint_max old_value, new_value;
        atomic_max uint_max* target = &state[free_index];

        do {
            old_value = atomic_get_relaxed(target);
            new_value = old_value & ~mask;

            if (old_value == new_value) {
                break;
            }
            // @bug Wrong use
        } while (!atomic_compare_exchange_strong_release(target, old_value, new_value));

        // Update the counters and indices
        element_count -= bits_in_current_block;
        ++free_index;
        bit_index = 0;
    }
}

FORCE_INLINE
void thrd_chunk_free_elements_atomic(ChunkMemory* const buf, uint_max element, uint32 element_count = 1) NO_EXCEPT
{
    thrd_chunk_free_elements_atomic_internal(buf->free, element, element_count);
    DEBUG_MEMORY_DELETE((uintptr_t) (buf->memory + element * buf->chunk_size), buf->chunk_size * element_count);
}

// @performance We can optimize it by checking if we can just append additional chunks if they are free
inline
int32 thrd_chunk_resize(ChunkMemory* const buf, int32 element_id, uint32 elements_old, uint32 elements_new) NO_EXCEPT
{
    const byte* data = chunk_get_element(buf, element_id);

    int32 chunk_id = thrd_chunk_reserve(buf, elements_new);
    byte* data_new = chunk_get_element(buf, chunk_id);

    memcpy(data_new, data, buf->chunk_size * elements_old);

    // @see performance remark above
    //if (element_id != chunk_id) {
        thrd_chunk_free_elements_atomic(buf, element_id, elements_old);
    //}

    return chunk_id;
}

inline
int64 chunk_dump(const ChunkMemory* const buf, byte* data) NO_EXCEPT
{
    LOG_1("[INFO] Dump ChunkMemory");
    const byte* const start = data;

    data = write_le(data, buf->capacity);
    data = write_le(data, buf->size);
    data = write_le(data, buf->chunk_size);
    data = write_le(data, buf->last_pos);
    data = write_le(data, buf->alignment);

    const uint32 free_offset = (uint32) ((uintptr_t) buf->free - (uintptr_t) buf->memory);
    data = write_le(data, free_offset);

    // All memory is handled in the buffer -> simply copy the buffer
    // This also includes the free array
    memcpy(data, buf->memory, buf->size);

    #if !_WIN32 && !__LITTLE_ENDIAN__
        uint_max* free_data = (uint_max *) (data + free_offset);
    #endif

    data += buf->size;

    #if !_WIN32 && !__LITTLE_ENDIAN__
        // @todo replace with simd endian swap if it is faster
        for (uint32 i = 0; i < ceil_div(buf->capacity, (uint32) (sizeof(uint_max) * 8)); ++i) {
            *free_data = SWAP_ENDIAN_LITTLE(*free_data);
            ++free_data;
        }
    #endif

    LOG_1("[INFO] Dumped ChunkMemory: %n B", {DATA_TYPE_UINT64, (void *) &buf->size});

    return data - start;
}

inline HOT_CODE
byte* chunk_get_memory(ChunkMemory* const buf, uint32 elements) NO_EXCEPT
{
    const int32 element = chunk_reserve(buf, elements);
    return chunk_get_element(buf, element);
}

inline HOT_CODE
byte* chunk_get_memory_one(ChunkMemory* const buf) NO_EXCEPT
{
    const int32 element = chunk_reserve_one(buf);
    return chunk_get_element(buf, element);
}

inline
int64 chunk_load(ChunkMemory* const buf, const byte* data) NO_EXCEPT
{
    LOG_1("[INFO] Loading ChunkMemory");

    const byte* const start = data;

    data = read_le(data, &buf->capacity);
    data = read_le(data, &buf->size);
    data = read_le(data, &buf->chunk_size);
    data = read_le(data, &buf->last_pos);
    data = read_le(data, &buf->alignment);

    uint32 free_offset;
    data = read_le(data, &free_offset);

    memcpy(buf->memory, data, buf->size);
    data += buf->size;

    buf->free = (uint_max *) (buf->memory + free_offset);

    #if !_WIN32 && !__LITTLE_ENDIAN__
        uint_max* free_data = buf->free;
        // @todo replace with simd endian swap if it is faster
        for (uint32 i = 0; i < ceil_div(buf->capacity, (uint32) (sizeof(uint_max) * 8)); ++i) {
            *free_data = SWAP_ENDIAN_LITTLE(*free_data);
            ++free_data;
        }
    #endif

    LOG_1("[INFO] Loaded ChunkMemory: %n B", {DATA_TYPE_UINT64, &buf->size});

    return data - start;
}

// @performance Is _BitScanForward faster?
// @performance We could probably even reduce the number of iterations by only iterating until popcount is reached?
#define chunk_iterate_start(buf, chunk_id) {                                                    \
    uint32 free_index = 0;                                                                      \
    uint32 bit_index = 0;                                                                       \
                                                                                                \
    /* Iterate the chunk memory */                                                              \
    for (; chunk_id < (buf)->capacity; ++chunk_id) {                                               \
        /* Check if asset is defined */                                                         \
        if (!(buf)->free[free_index]) {                                                         \
            /* Skip various elements */                                                         \
            /* @performance Consider to only check 1 byte instead of 8 */                       \
            /* There are probably even better ways by using compiler intrinsics if available */ \
            bit_index += (sizeof(uint_max) * 8 - 1); /* +64 - 1 since the loop also increases by 1 */                   \
        } else if ((buf)->free[free_index] & (OMS_UINT_ONE << bit_index))

// INTERNAL: Not intended for use by any programmer
#define chunk_iterate_end_internal { \
        ++bit_index;                 \
        if (bit_index > (sizeof(uint_max) * 8 - 1)) {        \
            bit_index = 0;           \
            ++free_index;            \
        }                            \
    }

// This is needed because if bit_index can be larger than 127 we need to skip multiple free_index
// But even for less than 127 we still may have to change the bit_index to a value != 0
// bit_index = 0 is only allowed for a 1 skip or (sizeof(uint_max) * 8) skip (as used in chunk_iterate_end_internal)
// INTERNAL: Not intended for use by any programmer
#define chunk_iterate_end_internal_n(n) {  \
        if (bit_index > (sizeof(uint_max) * 8 - 1)) {               \
            bit_index %= (sizeof(uint_max) * 8);                \
            free_index += ((n) / (sizeof(uint_max) * 8));         \
        }                                   \
    }

// Breaks out of the iteration (uses break, like you would use in a normal loop)
// #define chunk_iterate_break break

// Skip this element (uses continue, like you would use in a normal loop)
#define chunk_iterate_continue chunk_iterate_end_internal continue

// This is the fix to the skip from chunk_iterate_small_skip.
// Use only when actually needed.
// If the skip is guaranteed by the algorithm to be <=elements use chunk_iterate_small_skip
#define chunk_iterate_continue_n(n) { \
        bit_index += (n); \
    } chunk_iterate_end_internal_n((n)) continue

// Ends the for loop from chunk_iterate_start
#define chunk_iterate_end chunk_iterate_end_internal }}

#endif
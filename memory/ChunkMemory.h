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

#include <string.h>
#include "../stdlib/Types.h"
#include "../utils/Assert.h"
#include "../utils/EndianUtils.h"
#include "../utils/BitUtils.h"
#include "../compiler/CompilerUtils.h"
#include "../log/Log.h"
#include "../log/Stats.h"
#include "../log/PerformanceProfiler.h"
#include "../log/DebugMemory.h"
#include "BufferMemory.h"
#include "../system/Allocator.h"
#include "../thread/Thread.h"

// This storage system is best used for fixed sized chunks
// What I mean by that, every element has the same size.
// Currently a caller could reserve multiple chunks to represent a single data entity
// This can be fine in a single threaded application
// However, this can lead to fragmentation which is hard to clean up because
// we can't just defragment the memory since we don't know which chunks are currently in use
// In use could mean by pointer of id. In use data isn't allowed to move or it would become "invalid"
// If you need a data structure that can be defragmented use DataPool, which basically builds upon ChunkMemory
// Fixed sized data structures that use this ChunkMemory can be:
//      1. HashMap
//      2. Queue
// Carefull, both examples have alternative use cases which may require variable sized elements
struct ChunkMemory {
    byte* memory;

    // @question Do I really want to use uint?
    uint64 size;
    int32 last_pos;
    uint32 count;
    uint32 chunk_size;

    // WARNING: The alignment may increase the original chunk size e.g.
    // element_size = 14, alignment = sizeof(size_t) => chunk_size = 32
    uint32 alignment;

    // length = count
    // free describes which locations are used and which are free
    alignas(8) uint64* free;
};

FORCE_INLINE
uint32 chunk_size_element(uint32 element_size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    return OMS_ALIGN_UP(element_size, alignment);
}

FORCE_INLINE
uint64 chunk_size_total(uint32 count, uint32 element_size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    element_size = chunk_size_element(element_size, alignment);

    // @performance Can we remove the alignment * 2? This is just a shotgun method to ensure full alignment

    return count * element_size
        + sizeof(uint64) * CEIL_DIV(count, 64) // free
        + alignment * 2; // overhead for alignment
}

// INFO: A chunk count of 2^n is recommended for maximum performance
inline
void chunk_alloc(ChunkMemory* buf, uint32 count, uint32 element_size, int32 alignment = sizeof(size_t))
{
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(count);
    PROFILE(PROFILE_CHUNK_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("[INFO] Allocating ChunkMemory");

    element_size = chunk_size_element(element_size, alignment);
    const uint64 size = chunk_size_total(count, element_size, alignment);

    buf->memory = alignment < 2
        ? (byte *) platform_alloc(size)
        : (byte *) platform_alloc_aligned(size, alignment);

    // @question Why don't I memset the memory to 0 here, but do it in chunk_init

    buf->count = count;
    buf->size = size;
    buf->chunk_size = element_size;
    buf->last_pos = -1;
    buf->alignment = alignment;
    buf->free = (uint64 *) OMS_ALIGN_UP(
        (uint64) ((uintptr_t) (buf->memory + count * element_size)),
        (uint64) alignment
    );
    compiler_memset_aligned(buf->free, 0, (count + 63) / 64);
}

inline
void chunk_init(ChunkMemory* __restrict buf, BufferMemory* __restrict data, uint32 count, uint32 element_size, int32 alignment = sizeof(size_t))
{
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(count);

    element_size = chunk_size_element(element_size, alignment);

    const uint64 size = chunk_size_total(count, element_size, alignment);

    buf->memory = buffer_get_memory(data, size, alignment);
    memset(buf->memory, 0, size);

    buf->count = count;
    buf->size = size;
    buf->chunk_size = element_size;
    buf->last_pos = -1;
    buf->alignment = alignment;
    buf->free = (uint64 *) OMS_ALIGN_UP((uintptr_t) (buf->memory + count * element_size), 64);
    compiler_memset_aligned(buf->free, 0, (count + 63) / 64);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

inline
void chunk_init(ChunkMemory* __restrict buf, byte* __restrict data, uint32 count, uint32 element_size, int32 alignment = sizeof(size_t))
{
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(count);

    element_size = chunk_size_element(element_size, alignment);

    const uint64 size = chunk_size_total(count, element_size, alignment);

    buf->memory = (byte *) OMS_ALIGN_UP((uintptr_t) data, alignment);

    buf->count = count;
    buf->size = size;
    buf->chunk_size = element_size;
    buf->last_pos = -1;
    buf->alignment = alignment;
    buf->free = (uint64 *) OMS_ALIGN_UP(
        (uintptr_t) (buf->memory + count * element_size),
        (uint64) alignment
    );
    compiler_memset_aligned(buf->free, 0, (count + 63) / 64);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

inline HOT_CODE
void chunk_free(ChunkMemory* buf)
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

FORCE_INLINE HOT_CODE
uint64* chunk_find_free_array(const ChunkMemory* buf) NO_EXCEPT
{
    return (uint64 *) OMS_ALIGN_UP((uintptr_t) (buf->memory + buf->count * buf->chunk_size), (uint64) buf->alignment);
}

FORCE_INLINE HOT_CODE
uint32 chunk_id_from_memory(const ChunkMemory* buf, const byte* pos) NO_EXCEPT {
    return (uint32) ((uintptr_t) pos - (uintptr_t) buf->memory) / buf->chunk_size;
}

FORCE_INLINE HOT_CODE FORCE_FLATTEN
byte* chunk_get_element(ChunkMemory* buf, uint32 element) NO_EXCEPT
{
    // @question How is this even possible? Isn't an assert enough?
    if (element >= buf->count || element < 0) {
        return NULL;
    }

    byte* offset = buf->memory + element * buf->chunk_size;
    ASSERT_TRUE(offset);

    DEBUG_MEMORY_READ((uintptr_t) offset, buf->chunk_size);

    return offset;
}

FORCE_INLINE HOT_CODE
bool chunk_is_free(const ChunkMemory* buf, uint32 element) NO_EXCEPT
{
    const uint32 free_index = element / 64;
    const uint32 bit_index = MODULO_2(element, 64);

    return !IS_BIT_SET_64_R2L(buf->free[free_index], bit_index);
}

// This is effectively the same as reserve with elements = 1 which allows for some performance improvements
// state_count = number of maximum elements in the state array.
HOT_CODE FORCE_FLATTEN
int32 chunk_reserve_one(uint64* state, uint32 state_count, int32 start_index = 0) NO_EXCEPT
{
    if ((uint32) start_index >= state_count) { UNLIKELY
        start_index = -1;
    }

    uint32 free_index = start_index / 64;
    uint32 bit_index = MODULO_2(start_index, 64);

    // Check standard simple solution
    if (!IS_BIT_SET_64_R2L(state[free_index], bit_index)) {
        state[free_index] |= (1ULL << bit_index);

        return start_index;
    }

    for (uint32 i = 0; i < state_count; i+= 64) {
        if (state[free_index] != 0xFFFFFFFFFFFFFFFFULL) {
            // @bug This doesn't return the next best element in a hash map case
            // In a hash map we want the next free element AFTER start_index
            // However, this below may return a previous element since it ignores the start_index
            // The reason why we want the next best element is because it is faster to iterate (cache locality)
            bit_index = compiler_find_first_bit_r2l(~state[free_index]);

            const uint32 id = free_index * 64 + bit_index;
            if (id >= state_count) { UNLIKELY
                free_index = 0;

                continue;
            }

            state[free_index] |= (1ULL << bit_index);

            return id;
        } else {
            ++free_index;
            if (free_index * 64 >= state_count) {
                free_index = 0;
            }
        }
    }

    return -1;
}

HOT_CODE FORCE_FLATTEN
int32 chunk_reserve_one(ChunkMemory* buf) NO_EXCEPT
{
    if ((uint32) (buf->last_pos + 1) >= buf->count) { UNLIKELY
        buf->last_pos = -1;
    }

    uint32 free_index = (buf->last_pos + 1) / 64;
    uint32 bit_index = MODULO_2(buf->last_pos + 1, 64);

    // Check standard simple solution
    if (!IS_BIT_SET_64_R2L(buf->free[free_index], bit_index)) {
        buf->free[free_index] |= (1ULL << bit_index);
        ++buf->last_pos;

        return buf->last_pos;
    }

    const uint32 total_words = (buf->count + 63) / 64;

    for (uint32 w = 0; w < total_words; ++w) {
        if (buf->free[free_index] != 0xFFFFFFFFFFFFFFFFULL) {
            bit_index = compiler_find_first_bit_r2l(~buf->free[free_index]);

            const uint32 id = free_index * 64 + bit_index;
            if (id < buf->count) { LIKELY
                buf->free[free_index] |= (1ULL << bit_index);
                buf->last_pos = id;

                return id;
            }
        }

        ++free_index;
        if (free_index * 64 >= buf->count) { UNLIKELY
            free_index = 0;
        }
    }

    return -1;
}

// use chunk_reserve_one if possible
HOT_CODE FORCE_FLATTEN
int32 chunk_reserve(ChunkMemory* buf, uint32 elements = 1) NO_EXCEPT
{
    ASSERT_TRUE(elements > 0);

    if ((uint32) (buf->last_pos + 1) >= buf->count) { UNLIKELY
        buf->last_pos = -1;
    }

    uint32 free_index = (buf->last_pos + 1) / 64;
    uint32 bit_index = MODULO_2(buf->last_pos + 1, 64);

    // Check standard simple solution
    if (elements == 1 && !IS_BIT_SET_64_R2L(buf->free[free_index], bit_index)) {
        buf->free[free_index] |= (1ULL << bit_index);
        ++buf->last_pos;

        return buf->last_pos;
    }

    int32 free_element = -1;
    uint32 i = 0;
    uint32 consecutive_free_bits = 0;

    while (i++ <= buf->count) {
        if (buf->free[free_index] == 0xFFFFFFFFFFFFFFFFULL) {
            // Skip fully filled ranges
            ++free_index;
            bit_index = 0;
            i += 64;
            consecutive_free_bits = 0;

            continue;
        } else if (free_index * 64 + bit_index + elements - consecutive_free_bits > buf->count) { UNLIKELY
            // Go to beginning after overflow
            i += buf->count - (free_index * 64 + bit_index);
            consecutive_free_bits = 0;
            free_index = 0;
            bit_index = 0;

            continue;
        }

        // Find first free element
        // This MUST find a free element, otherwise we wouldn't have gotten here
        bit_index = compiler_find_first_bit_r2l(~buf->free[free_index]);

        // Let's check if we have enough free space, we need more than just one free bit
        do {
            ++i;
            ++consecutive_free_bits;
            ++bit_index;

            if (bit_index > 63) {
                bit_index = 0;
                ++free_index;

                break;
            }
        } while (!IS_BIT_SET_64_R2L(buf->free[free_index], bit_index)
            && consecutive_free_bits != elements
            && free_index * 64 + bit_index + elements - consecutive_free_bits <= buf->count
            && i <= buf->count
        );

        // Do we have enough free bits?
        if (consecutive_free_bits == elements) {
            free_element = free_index * 64 + bit_index - elements;
            const uint32 possible_free_index = free_element / 64;
            const uint32 possible_bit_index = MODULO_2(free_element, 64);

            // Mark as used
            if (elements == 1) {
                buf->free[possible_free_index] |= (1ULL << possible_bit_index);
            } else {
                uint32 elements_temp = elements;
                uint64 current_free_index = possible_free_index;
                uint32 current_bit_index = possible_bit_index;

                while (elements_temp > 0) {
                    // Calculate the number of bits we can set in the current 64-bit block
                    uint32 bits_in_current_block = OMS_MIN(64 - current_bit_index, elements_temp);

                    // Create a mask to set the bits
                    uint64 mask = ((1ULL << (bits_in_current_block & 63)) - 1) << current_bit_index | ((bits_in_current_block >> 6) * ((uint64_t)-1));
                    buf->free[current_free_index] |= mask;

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

    DEBUG_MEMORY_WRITE((uintptr_t) (buf->memory + free_element * buf->chunk_size), elements * buf->chunk_size);

    buf->last_pos = free_element;

    return free_element;
}

FORCE_INLINE HOT_CODE
void chunk_free_element(ChunkMemory* buf, uint64 free_index, int32 bit_index) NO_EXCEPT
{
    buf->free[free_index] &= ~(1ULL << bit_index);
    DEBUG_MEMORY_DELETE((uintptr_t) (buf->memory + (free_index * 64 + bit_index) * buf->chunk_size), buf->chunk_size);
}

FORCE_INLINE HOT_CODE
void chunk_free_element(ChunkMemory* buf, uint32 element) NO_EXCEPT
{
    const uint64 free_index = element / 64;
    const uint32 bit_index = MODULO_2(element, 64);
    chunk_free_element(buf, free_index, bit_index);
}

HOT_CODE
void chunk_free_elements(ChunkMemory* buf, uint64 element, uint32 element_count = 1) NO_EXCEPT
{
    uint64 free_index = element / 64;
    uint32 bit_index = MODULO_2(element, 64);

    if (element == 1) {
        chunk_free_element(buf, free_index, bit_index);
        return;
    }

    while (element_count > 0) {
        // Calculate the number of bits we can clear in the current 64-bit block
        const uint32 bits_in_current_block = OMS_MIN(64 - bit_index, element_count);

        // Create a mask to clear the bits
        const uint64 mask = ((1ULL << bits_in_current_block) - 1) << bit_index;
        buf->free[free_index] &= ~mask;

        // Update the counters and indices
        element_count -= bits_in_current_block;
        ++free_index;
        bit_index = 0;
    }

    DEBUG_MEMORY_DELETE((uintptr_t) (buf->memory + element * buf->chunk_size), buf->chunk_size);
}

inline
int64 chunk_dump(const ChunkMemory* buf, byte* data)
{
    LOG_1("[INFO] Dump ChunkMemory");
    byte* start = data;

    // Count
    *((uint32 *) data) = SWAP_ENDIAN_LITTLE(buf->count);
    data += sizeof(buf->count);

    // Size
    *((uint64 *) data) = SWAP_ENDIAN_LITTLE(buf->size);
    data += sizeof(buf->size);

    // Chunk Size
    *((uint32 *) data) = SWAP_ENDIAN_LITTLE(buf->chunk_size);
    data += sizeof(buf->chunk_size);

    // Last pos
    *((int32 *) data) = SWAP_ENDIAN_LITTLE(buf->last_pos);
    data += sizeof(buf->last_pos);

    // Alignment
    *((uint32 *) data) = SWAP_ENDIAN_LITTLE(buf->alignment);
    data += sizeof(buf->alignment);

    // Free start
    uint32 free_offset = (uint32) ((uintptr_t) buf->free - (uintptr_t) buf->memory);
    *((uint32 *) data) = SWAP_ENDIAN_LITTLE(free_offset);
    data += sizeof(free_offset);

    // All memory is handled in the buffer -> simply copy the buffer
    // This also includes the free array
    memcpy(data, buf->memory, buf->size);

    #if !_WIN32 && !__LITTLE_ENDIAN__
        uint64* free_data = (uint64 *) (data + free_offset);
    #endif

    data += buf->size;

    #if !_WIN32 && !__LITTLE_ENDIAN__
        // @todo replace with simd endian swap if it is faster
        for (uint32 i = 0; i < (buf->count + 63) / 64; ++i) {
            *free_data = SWAP_ENDIAN_LITTLE(*free_data);
            ++free_data;
        }
    #endif

    LOG_1("[INFO] Dumped ChunkMemory: %n B", {LOG_DATA_UINT64, (void *) &buf->size});

    return data - start;
}

inline HOT_CODE
byte* chunk_get_memory(ChunkMemory* buf, uint32 elements) NO_EXCEPT
{
    int32 element = chunk_reserve(buf, elements);

    return chunk_get_element(buf, element);
}

inline
int64 chunk_load(ChunkMemory* buf, const byte* data)
{
    LOG_1("[INFO] Loading ChunkMemory");

    const byte* start = data;

    // Count
    buf->count = SWAP_ENDIAN_LITTLE(*((uint32 *) data));
    data += sizeof(buf->count);

    // Size
    buf->size = SWAP_ENDIAN_LITTLE(*((uint64 *) data));
    data += sizeof(buf->size);

    // Chunk Size
    buf->chunk_size = SWAP_ENDIAN_LITTLE(*((uint32 *) data));
    data += sizeof(buf->chunk_size);

    // Last pos
    buf->last_pos = SWAP_ENDIAN_LITTLE(*((int32 *) data));
    data += sizeof(buf->last_pos);

    // Alignment
    buf->alignment = SWAP_ENDIAN_LITTLE(*((uint32 *) data));
    data += sizeof(buf->alignment);

    // Free start
    uint32 free_offset = SWAP_ENDIAN_LITTLE(*((uint32 *) data));
    data += sizeof(free_offset);

    memcpy(buf->memory, data, buf->size);
    data += buf->size;

    buf->free = (uint64 *) (buf->memory + free_offset);

    #if !_WIN32 && !__LITTLE_ENDIAN__
        uint64* free_data = buf->free;
        // @todo replace with simd endian swap if it is faster
        for (uint32 i = 0; i < (buf->count + 63) / 64; ++i) {
            *free_data = SWAP_ENDIAN_LITTLE(*free_data);
            ++free_data;
        }
    #endif

    LOG_1("[INFO] Loaded ChunkMemory: %n B", {LOG_DATA_UINT64, &buf->size});

    return data - start;
}

// @performance Is _BitScanForward faster?
// @performance We could probably even reduce the number of iterations by only iterating until popcount is reached?
#define chunk_iterate_start(buf, chunk_id) {                                                    \
    uint32 free_index = 0;                                                                      \
    uint32 bit_index = 0;                                                                       \
                                                                                                \
    /* Iterate the chunk memory */                                                              \
    for (; chunk_id < (buf)->count; ++chunk_id) {                                               \
        /* Check if asset is defined */                                                         \
        if (!(buf)->free[free_index]) {                                                         \
            /* Skip various elements */                                                         \
            /* @performance Consider to only check 1 byte instead of 8 */                       \
            /* There are probably even better ways by using compiler intrinsics if available */ \
            bit_index += 63; /* +64 - 1 since the loop also increases by 1 */                   \
        } else if ((buf)->free[free_index] & (1ULL << bit_index))

// INTERNAL: Not intended for use by any programmer
#define chunk_iterate_end_internal { \
        ++bit_index;                 \
        if (bit_index > 63) {        \
            bit_index = 0;           \
            ++free_index;            \
        }                            \
    }

// This is needed because if bit_index can be larger than 127 we need to skip multiple free_index
// But even for less than 127 we still may have to change the bit_index to a value != 0
// bit_index = 0 is only allowed for a 1 skip or 64 skip (as used in chunk_iterate_end_internal)
// INTERNAL: Not intended for use by any programmer
#define chunk_iterate_end_internal_n(n) {  \
        if (bit_index > 63) {               \
            bit_index %= 64;                \
            free_index += ((n) / 64);         \
        }                                   \
    }

// Breaks out of the iteration (uses break, like you would use in a normal loop)
// #define chunk_iterate_break break

// Skip this element (uses continue, like you would use in a normal loop)
#define chunk_iterate_continue chunk_iterate_end_internal continue

// This is the fix to the skip from chunk_iterate_small_skip.
// Use only when actually needed.
// If the skip is guaranteed by the algorithm to be <= 63 elements use chunk_iterate_small_skip
#define chunk_iterate_continue_n(n) { \
        bit_index += (n); \
    } chunk_iterate_end_internal_n((n)) continue

// Ends the for loop from chunk_iterate_start
#define chunk_iterate_end chunk_iterate_end_internal }}

#endif
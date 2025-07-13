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
#include "../utils/TestUtils.h"
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

struct ChunkMemory {
    byte* memory;

    uint64 size;
    int32 last_pos;
    uint32 count;
    uint32 chunk_size;
    uint32 alignment;

    // length = count
    // free describes which locations are used and which are free
    alignas(8) uint64* free;
};

// INFO: A chunk count of 2^n is recommended for maximum performance
inline
void chunk_alloc(ChunkMemory* buf, uint32 count, uint32 chunk_size, int32 alignment = 64)
{
    ASSERT_SIMPLE(chunk_size);
    ASSERT_SIMPLE(count);
    PROFILE(PROFILE_CHUNK_ALLOC, NULL, false, true);
    LOG_1("[INFO] Allocating ChunkMemory");

    chunk_size = ROUND_TO_NEAREST(chunk_size, alignment);

    uint64 size = count * chunk_size
        + sizeof(uint64) * CEIL_DIV(count, alignment) // free
        + alignment * 2; // overhead for alignment

    buf->memory = alignment < 2
        ? (byte *) platform_alloc(size)
        : (byte *) platform_alloc_aligned(size, alignment);

    buf->count = count;
    buf->size = size;
    buf->chunk_size = chunk_size;
    buf->last_pos = -1;
    buf->alignment = alignment;

    // @question Could it be beneficial to have this before the element data?
    buf->free = (uint64 *) ROUND_TO_NEAREST((uintptr_t) (buf->memory + count * chunk_size), alignment);

    memset(buf->memory, 0, buf->size);
}

inline
void chunk_init(ChunkMemory* buf, BufferMemory* data, uint32 count, uint32 chunk_size, int32 alignment = 64)
{
    ASSERT_SIMPLE(chunk_size);
    ASSERT_SIMPLE(count);

    chunk_size = ROUND_TO_NEAREST(chunk_size, alignment);

    uint64 size = count * chunk_size
        + sizeof(uint64) * CEIL_DIV(count, alignment) // free
        + alignment * 2; // overhead for alignment

    buf->memory = buffer_get_memory(data, size);

    buf->count = count;
    buf->size = size;
    buf->chunk_size = chunk_size;
    buf->last_pos = -1;
    buf->alignment = alignment;

    // @question Could it be beneficial to have this before the element data?
    //  On the other hand the way we do it right now we never have to move past the free array since it is at the end
    //  On another hand we could by accident overwrite the values in free if we are not careful
    buf->free = (uint64 *) ROUND_TO_NEAREST((uintptr_t) (buf->memory + count * chunk_size), 64);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

inline
void chunk_init(ChunkMemory* buf, byte* data, uint32 count, uint32 chunk_size, int32 alignment = 64)
{
    ASSERT_SIMPLE(chunk_size);
    ASSERT_SIMPLE(count);

    chunk_size = ROUND_TO_NEAREST(chunk_size, alignment);

    uint64 size = count * chunk_size
        + sizeof(uint64) * CEIL_DIV(count, alignment) // free
        + alignment * 2; // overhead for alignment

    buf->memory = (byte *) ROUND_TO_NEAREST((uintptr_t) data, alignment);

    buf->count = count;
    buf->size = size;
    buf->chunk_size = chunk_size;
    buf->last_pos = -1;
    buf->alignment = alignment;

    // @question Could it be beneficial to have this before the element data?
    //  On the other hand the way we do it right now we never have to move past the free array since it is at the end
    //  On another hand we could by accident overwrite the values in free if we are not careful
    buf->free = (uint64 *) ROUND_TO_NEAREST((uintptr_t) (buf->memory + count * chunk_size), alignment);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

inline
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

FORCE_INLINE
uint32 chunk_id_from_memory(const ChunkMemory* buf, const byte* pos) noexcept {
    return (uint32) ((uintptr_t) pos - (uintptr_t) buf->memory) / buf->chunk_size;
}

inline
byte* chunk_get_element(ChunkMemory* buf, uint32 element, bool zeroed = false) noexcept
{
    if (element >= buf->count) {
        return NULL;
    }

    byte* offset = buf->memory + element * buf->chunk_size;
    ASSERT_SIMPLE(offset);

    if (zeroed) {
        memset((void *) offset, 0, buf->chunk_size);
    }

    DEBUG_MEMORY_READ((uintptr_t) offset, buf->chunk_size);

    return offset;
}

// This is a special case of the chunk_reserve code where we try to find n unset elements
int32 chunk_get_unset(uint64* state, uint32 state_count, int32 start_index = 0) noexcept {
    if ((uint32) start_index >= state_count) {
        start_index = 0;
    }

    uint32 free_index = start_index / 64;
    uint32 bit_index = start_index & 63;

    // Check standard simple solution
    if (!IS_BIT_SET_64_R2L(state[free_index], bit_index)) {
        state[free_index] |= (1ULL << bit_index);

        return free_index * 64 + bit_index;
    }

    for (uint32 i = 0; i < state_count; i+= 64) {
        if (state[free_index] != 0xFFFFFFFFFFFFFFFF) {
            bit_index = compiler_find_first_bit_r2l(~state[free_index]);

            uint32 id = free_index * 64 + bit_index;
            if (id >= state_count) {
                ++free_index;
                if (free_index * 64 >= state_count) {
                    free_index = 0;
                }

                continue;
            }

            state[free_index] |= (1ULL << bit_index);

            return id;
        }

        ++free_index;
        if (free_index * 64 >= state_count) {
            free_index = 0;
        }
    }

    return -1;
}

inline
int32 chunk_reserve(ChunkMemory* buf, uint32 elements = 1) noexcept
{
    if ((uint32) (buf->last_pos + 1) >= buf->count) {
        buf->last_pos = -1;
    }

    uint32 free_index = (buf->last_pos + 1) / 64;
    uint32 bit_index = (buf->last_pos + 1) & 63;

    // Check standard simple solution
    if (elements == 1 && !IS_BIT_SET_64_R2L(buf->free[free_index], bit_index)) {
        buf->free[free_index] |= (1ULL << bit_index);
        ++buf->last_pos;

        return free_index * 64 + bit_index;
    }

    int32 free_element = -1;
    uint32 i = 0;
    uint32 consecutive_free_bits = 0;

    while (free_element < 0 && i++ <= buf->count) {
        if (free_index * 64 + bit_index + elements - consecutive_free_bits > buf->count) {
            // Go to beginning after overflow
            i += buf->count - (free_index * 64 + bit_index);
            consecutive_free_bits = 0;
            free_index = 0;
            bit_index = 0;

            continue;
        } else if (buf->free[free_index] == 0xFFFFFFFFFFFFFFFF) {
            // Skip fully filled ranges
            ++free_index;
            bit_index = 0;
            i += 64;
            consecutive_free_bits = 0;

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
            uint32 possible_free_index = free_element / 64;
            uint32 possible_bit_index = free_element & 63;

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

    if (free_element < 0) {
        LOG_3("No free chunk memory index found");
        return -1;
    }

    DEBUG_MEMORY_WRITE((uintptr_t) (buf->memory + free_element * buf->chunk_size), elements * buf->chunk_size);

    buf->last_pos = free_element;

    return (int32) free_element;
}

inline
void chunk_free_element(ChunkMemory* buf, uint64 free_index, int32 bit_index) noexcept
{
    buf->free[free_index] &= ~(1ULL << bit_index);
    DEBUG_MEMORY_DELETE((uintptr_t) (buf->memory + (free_index * 64 + bit_index) * buf->chunk_size), buf->chunk_size);
}

inline
void chunk_free_elements(ChunkMemory* buf, uint64 element, uint32 element_count = 1) noexcept
{
    uint64 free_index = element / 64;
    uint32 bit_index = element & 63;

    if (element == 1) {
        chunk_free_element(buf, free_index, bit_index);
        return;
    }

    while (element_count > 0) {
        // Calculate the number of bits we can clear in the current 64-bit block
        uint32 bits_in_current_block = OMS_MIN(64 - bit_index, element_count);

        // Create a mask to clear the bits
        uint64 mask = ((1ULL << bits_in_current_block) - 1) << bit_index;
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

    // All memory is handled in the buffer -> simply copy the buffer
    // This also includes the free array
    memcpy(data, buf->memory, buf->size);
    data += buf->size;

    LOG_1("[INFO] Dumped ChunkMemory: %n B", {{LOG_DATA_UINT64, (void *) &buf->size}});

    return data - start;
}

inline
int64 chunk_load(ChunkMemory* buf, const byte* data)
{
    LOG_1("[INFO] Loading ChunkMemory");

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

    memcpy(buf->memory, data, buf->size);
    //data += buf->size;

    buf->free = (uint64 *) (buf->memory + buf->count * buf->chunk_size);

    LOG_1("[INFO] Loaded ChunkMemory: %n B", {{LOG_DATA_UINT64, &buf->size}});

    return buf->size;
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

#define chunk_iterate_end       \
        ++bit_index;            \
        if (bit_index > 63) {   \
            bit_index = 0;      \
            ++free_index;       \
        }                       \
    }}

#endif
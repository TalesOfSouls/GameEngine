/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_CHUNK_MEMORY_C
#define COMS_MEMORY_CHUNK_MEMORY_C

#include "ChunkMemory.h"
#include "../utils/BitUtils.h"
#include "../log/Log.h"
#include "../log/Stats.h"
#include "../log/PerformanceProfiler.h"
#include "../log/DebugMemory.h"
#include "BufferMemory.cpp"
#include "../system/Allocator.h"
#include "../thread/ThreadHelper.cpp"

FORCE_INLINE
size_t chunk_size_total(int32 capacity, int32 element_size) NO_EXCEPT
{
    return capacity * element_size
        + sizeof(size_t) * ceil_div(capacity, (int32) (sizeof(size_t) * 8)) // free
        + alignof(size_t) * 2; // overhead for alignment
}

// INFO: A chunk count of 2^n is recommended for maximum performance
inline
void chunk_alloc(
    ChunkMemory* const buf,
    int32 capacity,
    int32 max_capacity,
    int32 element_size,
    int32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_CHUNK_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemory");

    const size_t size = chunk_size_total(capacity, element_size);
    const size_t max_size = chunk_size_total(max_capacity, element_size);

    buf->memory = (byte *) platform_alloc_aligned(size, max_size, start_alignment);

    buf->capacity = capacity;
    buf->size = size;
    buf->chunk_size = element_size;
    buf->last_pos = -1;
    buf->alignment = alignment;
    buf->free = (size_t *) align_up(
        (size_t) ((uintptr_t) (buf->memory + capacity * element_size)),
        (size_t) alignof(size_t)
    );
    memset((void *) buf->free, 0, sizeof(size_t) * ceil_div(capacity, (int32) (sizeof(size_t) * 8)));

    LOG_1("[INFO] Allocated ChunkMemory: %n B", {DATA_TYPE_UINT64, &buf->size});
}

inline
void chunk_alloc(
    ChunkMemory* const buf,
    MemoryArena* mem,
    int32 capacity,
    int32 max_capacity,
    int32 element_size,
    int32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_CHUNK_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemory");

    const size_t size = chunk_size_total(capacity, element_size);
    const size_t max_size = chunk_size_total(max_capacity, element_size);

    MemoryArena* arena = mem_arena_add(mem, size, max_size, start_alignment);
    buf->memory = (byte *) arena->memory;

    buf->capacity = capacity;
    buf->size = size;
    buf->chunk_size = element_size;
    buf->last_pos = -1;
    buf->alignment = alignment;
    buf->free = (size_t *) align_up(
        (size_t) ((uintptr_t) (buf->memory + capacity * element_size)),
        (size_t) alignof(size_t)
    );
    memset((void *) buf->free, 0, sizeof(size_t) * ceil_div(capacity, (int32) (sizeof(size_t) * 8)));

    LOG_1("[INFO] Allocated ChunkMemory: %n B", {DATA_TYPE_UINT64, &buf->size});
}

inline
void chunk_init(
    ChunkMemory* const buf,
    BufferMemory* const data,
    int32 capacity,
    int32 element_size,
    int32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    const size_t size = chunk_size_total(capacity, element_size);

    buf->memory = memory_get(data, size, start_alignment);
    memset(buf->memory, 0, size);

    buf->capacity = capacity;
    buf->size = size;
    buf->chunk_size = element_size;
    buf->last_pos = -1;
    buf->alignment = alignment;
    buf->free = (size_t *) align_up((uintptr_t) (buf->memory + capacity * element_size), alignof(size_t));
    memset((void *) buf->free, 0, sizeof(size_t) * ceil_div(capacity, (int32) (sizeof(size_t) * 8)));

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

inline
void chunk_init(
    ChunkMemory* const buf,
    byte* const data,
    int32 capacity,
    int32 element_size,
    int32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    const size_t size = chunk_size_total(capacity, element_size);

    buf->memory = (byte *) align_up((uintptr_t) data, start_alignment);

    buf->capacity = capacity;
    buf->size = size;
    buf->chunk_size = element_size;
    buf->last_pos = -1;
    buf->alignment = alignment;
    buf->free = (size_t *) align_up(
        (uintptr_t) (buf->memory + capacity * element_size),
        (size_t) alignof(size_t)
    );
    memset((void *) buf->free, 0, sizeof(size_t) * ceil_div(capacity, (int32) (sizeof(size_t) * 8)));

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

inline
void chunk_free(ChunkMemory* const buf) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE((uintptr_t) buf->memory, buf->size);

    platform_aligned_free((void **) &buf->memory);

    buf->size = 0;
    buf->memory = NULL;
}

inline
void chunk_free(ChunkMemory* const buf, MemoryArena* mem) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE((uintptr_t) buf->memory, buf->size);

    mem_arena_remove(mem, buf->memory);

    buf->size = 0;
    buf->memory = NULL;
}

FORCE_INLINE
size_t* chunk_find_free_array(const ChunkMemory* const buf) NO_EXCEPT
{
    return (size_t *) align_up(
        (uintptr_t) (buf->memory + buf->capacity * buf->chunk_size),
        (size_t) alignof(size_t)
    );
}

FORCE_INLINE
uint32 chunk_id_from_memory(uintptr_t memory, uintptr_t pos, size_t chunk_size) NO_EXCEPT
{
    return (uint32) ((pos - memory) / chunk_size);
}

FORCE_INLINE
uint32 chunk_id_from_memory(void* memory, void* pos, size_t chunk_size) NO_EXCEPT
{
    return (uint32) (((uintptr_t)pos - (uintptr_t)memory) / chunk_size);
}

FORCE_INLINE FORCE_FLATTEN
byte* chunk_element_get(const ChunkMemory* const buf, int32 element) NO_EXCEPT
{
    if (element >= buf->capacity) {
        return NULL;
    }

    byte* const offset = buf->memory + element * buf->chunk_size;
    ASSERT_TRUE(offset);

    DEBUG_MEMORY_READ((uintptr_t) offset, buf->chunk_size);

    return offset;
}

FORCE_INLINE
bool chunk_is_free_internal(const size_t* const state, int32 element) NO_EXCEPT
{
    const uint32 free_index = element / (sizeof(size_t) * 8);
    const uint32 bit_index = MODULO_2(element, (sizeof(size_t) * 8));

    return !IS_BIT_SET_R2L(state[free_index], bit_index);
}

FORCE_INLINE
bool chunk_is_free(const ChunkMemory* const buf, int32 element) NO_EXCEPT
{
    return chunk_is_free_internal(buf->free, element);
}

// This is effectively the same as reserve with elements = 1 which allows for some performance improvements
// state_count = number of maximum elements in the state array.
HOT_CODE FORCE_FLATTEN
int32 chunk_reserve_one(size_t* state, uint32 state_count, int32 start_index = 0) NO_EXCEPT
{
    if ((uint32) start_index >= state_count) { UNLIKELY
        start_index = 0;
    }

    uint32 free_index = start_index / (sizeof(size_t) * 8);
    uint32 bit_index = MODULO_2(start_index, (sizeof(size_t) * 8));

    // Check standard simple solution
    if (!IS_BIT_SET_R2L(state[free_index], bit_index)) {
        state[free_index] |= (OMS_UINT_ONE << bit_index);

        return start_index;
    }

    for (uint32 i = 0; i < state_count; i+= (sizeof(size_t) * 8)) {
        if (state[free_index] != OMS_UINT_MAX) {
            // @bug This doesn't return the next best element in a hash map case
            // In a hash map we want the next free element AFTER start_index
            // However, this below may return a previous element since it ignores the start_index
            // The reason why we want the next best element is because it is faster to iterate (cache locality)
            bit_index = compiler_find_first_bit_r2l(~state[free_index]);

            const uint32 id = free_index * (sizeof(size_t) * 8) + bit_index;
            if (id >= state_count) { UNLIKELY
                free_index = 0;

                continue;
            }

            state[free_index] |= (OMS_UINT_ONE << bit_index);

            return id;
        } else {
            ++free_index;
            if (free_index * (sizeof(size_t) * 8) >= state_count) {
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

HOT_CODE FORCE_FLATTEN
int32 chunk_reserve_internal(size_t* const state, int32 capacity, int32 last_pos, int32 elements = 1) NO_EXCEPT
{
    ASSERT_TRUE(elements > 0);

    // There is some fundamental problem if this happens
    ASSERT_TRUE(elements < capacity);

    if ((int32) (last_pos + 1) >= capacity) { UNLIKELY
        last_pos = -1;
    }

    uint32 free_index = (last_pos + 1) / (sizeof(size_t) * 8);
    uint32 bit_index = MODULO_2(last_pos + 1, (sizeof(size_t) * 8));

    // Check standard simple solution
    if (elements == 1 && !IS_BIT_SET_R2L(state[free_index], bit_index)) {
        state[free_index] |= (OMS_UINT_ONE << bit_index);

        return ++last_pos;
    }

    int32 free_element = -1;
    int32 i = 0;
    int32 consecutive_free_bits = 0;

    while (i++ <= capacity) {
        if (state[free_index] == OMS_UINT_MAX) {
            // Skip fully filled ranges
            ++free_index;
            bit_index = 0;
            i += (sizeof(size_t) * 8);
            consecutive_free_bits = 0;

            continue;
        } else if (free_index * (sizeof(size_t) * 8) + bit_index + elements - consecutive_free_bits > capacity) { UNLIKELY
            // Go to beginning after overflow
            i += capacity - (free_index * (sizeof(size_t) * 8) + bit_index);
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

            if (bit_index > (sizeof(size_t) * 8 - 1)) {
                bit_index = 0;
                ++free_index;

                break;
            }
        } while (!IS_BIT_SET_R2L(state[free_index], bit_index)
            && consecutive_free_bits != elements
            && free_index * (sizeof(size_t) * 8) + bit_index + elements - consecutive_free_bits <= capacity
            && i <= capacity
        );

        // Do we have enough free bits?
        if (consecutive_free_bits == elements) {
            free_element = free_index * (sizeof(size_t) * 8) + bit_index - elements;
            const uint32 possible_free_index = free_element / (sizeof(size_t) * 8);
            const uint32 possible_bit_index = MODULO_2(free_element, (sizeof(size_t) * 8));

            // Mark as used
            if (elements == 1) {
                state[possible_free_index] |= (OMS_UINT_ONE << possible_bit_index);
            } else {
                int32 elements_temp = elements;
                size_t current_free_index = possible_free_index;
                int32 current_bit_index = possible_bit_index;

                while (elements_temp > 0) {
                    // Calculate the number of bits we can set in the current 64-bit block
                    uint32 bits_in_current_block = (uint32) OMS_MIN(((int32) (sizeof(size_t) * 8) - current_bit_index), elements_temp);

                    // Create a mask to set the bits
                    size_t mask = ((OMS_UINT_ONE << (bits_in_current_block & (sizeof(size_t) * 8 - 1))) - 1) << current_bit_index | ((bits_in_current_block >> 6) * ((size_t) -1));
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
int32 chunk_reserve(ChunkMemory* const buf, int32 elements = 1) NO_EXCEPT
{
    const int32 found = chunk_reserve_internal(buf->free, buf->capacity, buf->last_pos, elements);
    buf->last_pos = found + (elements - 1);

    DEBUG_MEMORY_WRITE((uintptr_t) (buf->memory + found * buf->chunk_size), elements * buf->chunk_size);

    return found;
}

FORCE_INLINE
void chunk_free_element(size_t* const state, size_t free_index, int32 bit_index) NO_EXCEPT
{
    state[free_index] &= ~(OMS_UINT_ONE << bit_index);
}

FORCE_INLINE
void chunk_free_element(ChunkMemory* const buf, size_t free_index, int32 bit_index) NO_EXCEPT
{
    buf->free[free_index] &= ~(OMS_UINT_ONE << bit_index);
    DEBUG_MEMORY_DELETE(
        (uintptr_t) (buf->memory + (free_index * (sizeof(size_t) * 8) + bit_index) * buf->chunk_size),
        buf->chunk_size
    );
}

FORCE_INLINE
void chunk_free_element(ChunkMemory* const buf, int32 element) NO_EXCEPT
{
    const size_t free_index = element / (sizeof(size_t) * 8);
    const uint32 bit_index = MODULO_2(element, (sizeof(size_t) * 8));
    buf->free[free_index] &= ~(OMS_UINT_ONE << bit_index);

    DEBUG_MEMORY_DELETE(
        (uintptr_t) (buf->memory + (free_index * (sizeof(size_t) * 8) + bit_index) * buf->chunk_size),
        buf->chunk_size
    );
}

HOT_CODE
void chunk_clear_bit_range_internal(size_t* const state, int32 element, int32 element_count = 1) NO_EXCEPT
{
    size_t free_index = element / (sizeof(size_t) * 8);
    uint32 bit_index = MODULO_2(element, (sizeof(size_t) * 8));

    if (element == 1) {
        chunk_free_element(state, free_index, bit_index);
        return;
    }

    while (element_count > 0) {
        // Calculate the number of bits we can clear in the current 64-bit block
        const uint32 bits_in_current_block = (uint32) OMS_MIN((int32) ((sizeof(size_t) * 8) - bit_index), element_count);

        // Create a mask to clear the bits
        const size_t mask = ((OMS_UINT_ONE << bits_in_current_block) - 1) << bit_index;
        state[free_index] &= ~mask;

        // Update the counters and indices
        element_count -= bits_in_current_block;
        ++free_index;
        bit_index = 0;
    }
}

HOT_CODE
void chunk_free_elements(ChunkMemory* const buf, int32 element, int32 element_count = 1) NO_EXCEPT
{
    chunk_clear_bit_range_internal(buf->free, element, element_count);
    DEBUG_MEMORY_DELETE((uintptr_t) (buf->memory + element * buf->chunk_size), buf->chunk_size * element_count);
}

HOT_CODE
void chunk_free_elements(ChunkMemory* const buf, byte* data, int32 element_count = 1) NO_EXCEPT
{
    const int32 element = chunk_id_from_memory(buf->memory, data, buf->chunk_size);
    chunk_clear_bit_range_internal(buf->free, element, element_count);
    DEBUG_MEMORY_DELETE((uintptr_t) (buf->memory + element * buf->chunk_size), buf->chunk_size * element_count);
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

    #if !defined(_WIN32) && !defined(__LITTLE_ENDIAN__)
        size_t* free_data = (size_t *) (data + free_offset);
        for (uint32 i = 0; i < ceil_div(buf->capacity, (uint32) (sizeof(size_t) * 8)); ++i) {
            *free_data = SWAP_ENDIAN_LITTLE(*free_data);
            ++free_data;
        }
    #endif

    data += buf->size;

    LOG_1("[INFO] Dumped ChunkMemory: %n B", {DATA_TYPE_UINT64, (void *) &buf->size});

    return data - start;
}

inline HOT_CODE
byte* chunk_memory_get(ChunkMemory* const buf, int32 elements) NO_EXCEPT
{
    const int32 element = chunk_reserve(buf, elements);
    return chunk_element_get(buf, element);
}

inline HOT_CODE
byte* memory_get(ChunkMemory* const buf, size_t size) NO_EXCEPT
{
    return chunk_memory_get(buf, (uint32) ((size + buf->chunk_size - 1) / buf->chunk_size));
}

inline HOT_CODE
byte* memory_get_temp(ChunkMemory* const buf, size_t size) NO_EXCEPT
{
    const uint32 element_count = (uint32) ((size + buf->chunk_size - 1) / buf->chunk_size);
    byte* data = chunk_memory_get(buf, element_count);
    chunk_free_elements(buf, data, element_count);

    return data;
}

/**
 * This allows us to use the chunk memory similar to a stack which automatically cleans itself up after leaving scope
 */
struct ChunkStackMemory {
    ChunkMemory* buffer;
    int32 element;
    uint32 count;

    HOT_CODE inline
    explicit ChunkStackMemory(
        ChunkMemory* buf,
        byte** mem,
        size_t size
    ) NO_EXCEPT
    {
        this->count = (uint32) ((size + buf->chunk_size - 1) / buf->chunk_size);
        this->element = chunk_reserve(buf, this->count);
        this->buffer = buf;

        *mem = chunk_element_get(buf, this->element);
    }

    HOT_CODE inline
    ~ChunkStackMemory() NO_EXCEPT
    {
        this->buffer->last_pos = this->element - 1;
        if (this->count == 1) {
            chunk_free_element(this->buffer, this->element);
        } else {
            chunk_free_elements(this->buffer, this->element, this->count);
        }
    }
};
#define CHUNK_STACK_MEMORY(buf, mem, size) ChunkStackMemory __chunk_stack_##__func__##_##__LINE__((buf), (mem), (size))

inline HOT_CODE
byte* chunk_memory_get_one(ChunkMemory* const buf) NO_EXCEPT
{
    const int32 element = chunk_reserve_one(buf);
    return chunk_element_get(buf, element);
}

inline
int64 chunk_load(ChunkMemory* const buf, const byte* data, size_t data_size = 0) NO_EXCEPT
{
    LOG_1("[INFO] Loading ChunkMemory");

    const byte* const start = data;

    MAYBE_UNUSED const size_t initial_size = buf->size;
    // Asset if we even have enough space
    ASSERT_TRUE(data_size ? initial_size >= data_size : true);
    PSEUDO_USE(data_size);
    PSEUDO_USE(initial_size);

    data = read_le(data, &buf->capacity);
    data = read_le(data, &buf->size);
    data = read_le(data, &buf->chunk_size);
    data = read_le(data, &buf->last_pos);
    data = read_le(data, &buf->alignment);

    uint32 free_offset;
    data = read_le(data, &free_offset);

    memcpy(buf->memory, data, buf->size);
    data += buf->size;

    buf->free = (size_t *) (buf->memory + free_offset);

    #if !defined(_WIN32) && !defined(__LITTLE_ENDIAN__)
        size_t* free_data = buf->free;
        for (uint32 i = 0; i < ceil_div(buf->capacity, (uint32) (sizeof(size_t) * 8)); ++i) {
            *free_data = SWAP_ENDIAN_LITTLE(*free_data);
            ++free_data;
        }
    #endif

    LOG_1("[INFO] Loaded ChunkMemory: %n B", {DATA_TYPE_UINT64, &buf->size});

    return data - start;
}

// @performance Is _BitScanForward faster?
// @performance We could probably even reduce the number of iterations by only iterating until popcount is reached?
#define chunk_iterate_start(buf, chunk_id) {                                                          \
    uint32 free_index = 0;                                                                            \
    uint32 bit_index = 0;                                                                             \
                                                                                                      \
    /* Iterate the chunk memory */                                                                    \
    for (; chunk_id < (buf)->capacity; ++chunk_id) {                                                  \
        /* Check if asset is defined */                                                               \
        if (!(buf)->free[free_index]) {                                                               \
            /* Skip various elements */                                                               \
            /* @performance Consider to only check 1 byte instead of 8 */                             \
            /* There are probably even better ways by using compiler intrinsics if available */       \
            bit_index += (sizeof(size_t) * 8 - 1); /* +64 - 1 since the loop also increases by 1 */ \
            chunk_id += (sizeof(size_t) * 8 - 1);                                                   \
        } else if ((buf)->free[free_index] & (OMS_UINT_ONE << bit_index))

// INTERNAL: Not intended for use by any programmer
#define chunk_iterate_end_internal {                  \
        ++bit_index;                                  \
        if (bit_index > (sizeof(size_t) * 8 - 1)) { \
            bit_index = 0;                            \
            ++free_index;                             \
        }                                             \
    }

// This is needed because if bit_index can be larger than 127 we need to skip multiple free_index
// But even for less than 127 we still may have to change the bit_index to a value != 0
// bit_index = 0 is only allowed for a 1 skip or (sizeof(size_t) * 8) skip (as used in chunk_iterate_end_internal)
// INTERNAL: Not intended for use by any programmer
#define chunk_iterate_end_internal_n(n) {                 \
        if (bit_index > (sizeof(size_t) * 8 - 1)) {     \
            bit_index %= (sizeof(size_t) * 8);          \
            free_index += ((n) / (sizeof(size_t) * 8)); \
        }                                                 \
    }

// Breaks out of the iteration (uses break, like you would use in a normal loop)
// #define chunk_iterate_break break

// Skip this element (uses continue, like you would use in a normal loop)
#define chunk_iterate_continue chunk_iterate_end_internal continue

// This is the fix to the skip from chunk_iterate_small_skip.
// Use only when actually needed.
// If the skip is guaranteed by the algorithm to be <=elements use chunk_iterate_small_skip
#define chunk_iterate_continue_n(n) {            \
        bit_index += (n);                        \
    } chunk_iterate_end_internal_n((n)) continue

// Ends the for loop from chunk_iterate_start
#define chunk_iterate_end chunk_iterate_end_internal }}

#endif
/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_THRD_CHUNK_MEMORY_C
#define COMS_MEMORY_THRD_CHUNK_MEMORY_C

#include "ThrdChunkMemory.h"

inline
void chunk_init(
    ThrdChunkMemory* const buf,
    byte* const data,
    int32 capacity,
    int32 element_size,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ASSERT_TRUE(element_size);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    MAYBE_UNUSED const size_t size = capacity * element_size
        + sizeof(size_t) * array_count
        + sizeof(size_t) * array_count
        + sizeof(size_t) * 2;

    buf->memory = (byte *) align_up((uintptr_t) data, alignment);

    buf->capacity = capacity;
    buf->chunk_size = element_size;
    buf->size = size;
    buf->last_pos.store(-1, memory_order_relaxed);

    buf->free = (atomic<size_t> *) align_up(
        (size_t) ((uintptr_t) (buf->memory + capacity * element_size)),
        (size_t) alignof(size_t)
    );
    buf->completeness = (atomic<size_t> *) align_up(
        (uintptr_t) (buf->free + array_count),
        (size_t) alignof(size_t)
    );

    memset((void *) buf->free, 0, sizeof(size_t) * array_count);
    memset((void *) buf->completeness, 0, sizeof(size_t) * array_count);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, size);
    PSEUDO_USE(size);
}

// INFO: A chunk count of 2^n is recommended for maximum performance
inline
void chunk_alloc(ThrdChunkMemory* const buf, int32 capacity, int32 max_capacity, int32 element_size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_CHUNK_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemoryT");

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    const size_t memory_size = capacity * element_size
        + sizeof(size_t) * array_count
        + sizeof(size_t) * array_count
        + sizeof(size_t) * 2;

    const size_t max_array_count = ceil_div(max_capacity, (int32) (sizeof(size_t) * 8));
    const size_t max_memory_size = max_capacity * element_size
        + sizeof(size_t) * max_array_count
        + sizeof(size_t) * max_array_count
        + sizeof(size_t) * 2;

    byte* const buffer = (byte *) platform_alloc_aligned(
        memory_size,
        max_memory_size,
        alignment
    );

    chunk_init(buf, buffer, capacity, element_size, alignment);
}

// INFO: A chunk count of 2^n is recommended for maximum performance
inline
void chunk_alloc(ThrdChunkMemory* const buf, MemoryArena* const mem, int32 capacity, int32 max_capacity, int32 element_size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_CHUNK_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemoryT");

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    const size_t memory_size = capacity * element_size
        + sizeof(size_t) * array_count
        + sizeof(size_t) * array_count
        + sizeof(size_t) * 2;

    const size_t max_array_count = ceil_div(max_capacity, (int32) (sizeof(size_t) * 8));
    const size_t max_memory_size = max_capacity * element_size
        + sizeof(size_t) * max_array_count
        + sizeof(size_t) * max_array_count
        + sizeof(size_t) * 2;

    MemoryArena* arena = mem_arena_add(
        mem,
        memory_size,
        max_memory_size,
        alignment
    );
    chunk_init(buf, arena->memory, capacity, element_size, alignment);
}

inline
void chunk_init(
    ThrdChunkMemory* const buf,
    BufferMemory* const data,
    int32 capacity,
    int32 element_size,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    const size_t size = capacity * element_size
        + sizeof(size_t) * array_count
        + sizeof(size_t) * array_count
        + sizeof(size_t) * 2;

    byte* buffer = memory_get(data, size, alignment);
    chunk_init(buf, buffer, capacity, element_size, alignment);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, size);
}

inline
void chunk_free(ThrdChunkMemory* const buf) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE(
        (uintptr_t) buf->memory,
        buf->chunk_size * buf->capacity + sizeof(size_t) * ceil_div(buf->capacity, (int32) (sizeof(size_t) * 8))
    );

    platform_aligned_free((void **) &buf->memory);

    buf->capacity = 0;
    buf->memory = NULL;
}

inline
void chunk_free(ThrdChunkMemory* const buf, MemoryArena* const mem) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE((uintptr_t) buf->memory, buf->chunk_size * buf->capacity + sizeof(size_t) * ceil_div(buf->capacity, (int32) (sizeof(size_t) * 8)));

    mem_arena_remove(mem, buf->memory);

    buf->capacity = 0;
    buf->memory = NULL;
}


FORCE_INLINE
int32 chunk_find_first_zero_bit(size_t word) NO_EXCEPT
{
    const size_t inverted = ~word;
    if (inverted == 0) {
        return -1;
    }

    return compiler_find_first_bit_r2l(inverted);
}

FORCE_INLINE
size_t chunk_range_mask(int32 bit_offset, int32 bit_count) NO_EXCEPT
{
    return (((OMS_UINT_ONE << bit_count) - 1) << bit_offset);
}

FORCE_INLINE
void chunk_clear_bit_range(atomic<size_t>* free_words, int32 element, int32 element_count) NO_EXCEPT
{
    int32 pos = element;
    int32 remaining = element_count;

    while (remaining > 0) {
        const int32 free_index = pos / (sizeof(size_t) * 8);
        const int32 bit_index = pos % (sizeof(size_t) * 8);
        const int32 bits_here = OMS_MIN((int32) (sizeof(size_t) * 8) - bit_index, remaining);
        const size_t mask = chunk_range_mask(bit_index, bits_here);

        free_words[free_index].fetch_and(~mask, memory_order_acq_rel);

        pos += bits_here;
        remaining -= bits_here;
    }
}

inline
bool chunk_try_claim_range(atomic<size_t>* free_words, int32 element, int32 element_count) NO_EXCEPT
{
    int32 pos = element;
    int32 claimed = 0;

    while (claimed < element_count) {
        const int32 free_index = pos / (sizeof(size_t) * 8);
        const int32 bit_index = pos % (sizeof(size_t) * 8);
        const int32 bits_here = OMS_MIN((int32) (sizeof(size_t) * 8) - bit_index, element_count - claimed);
        const size_t mask = chunk_range_mask(bit_index, bits_here);

        size_t word = free_words[free_index].load(memory_order_relaxed);
        while (true) {
            if (word & mask) {
                // Someone already holds at least one bit_index in this slice,
                // undo everything claimed so far in this attempt.
                if (claimed > 0) {
                    chunk_clear_bit_range(free_words, element, claimed);
                }

                return false;
            }

            const size_t new_word = word | mask;
            if (free_words[free_index].compare_exchange_weak(
                word, new_word, memory_order_acq_rel, memory_order_relaxed
            )) {
                // This slice claimed, move to the next word
                break;
            }
            // CAS refreshed `word` with the current value, loop back
            // and re-check it against `mask` before retrying.
        }

        claimed += bits_here;
        pos += bits_here;
    }

    return true;
}

FORCE_INLINE FORCE_FLATTEN
byte* chunk_element_get(const ThrdChunkMemory* const buf, int32 element) NO_EXCEPT
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
bool chunk_is_free(const ThrdChunkMemory* const buf, uint32 element) NO_EXCEPT
{
    const uint32 free_index = element / (uint32) (sizeof(size_t) * 8);
    const uint32 bit_index = element % (uint32) (sizeof(size_t) * 8);

    return !(buf->free[free_index].load(memory_order_relaxed) & (OMS_UINT_ONE << bit_index));
}

inline
int32 chunk_reserve_one(ThrdChunkMemory* const buf) NO_EXCEPT
{
    const int32 word_count = ceil_div(buf->capacity, (int32) (sizeof(size_t) * 8));
    if (word_count <= 0) {
        return -1;
    }

    const int32 hint = buf->last_pos.load(memory_order_relaxed);
    const int32 start_word = ((hint < 0 ? 0 : hint) / (sizeof(size_t) * 8)) % word_count;

    for (int32 offset = 0; offset < word_count; ++offset) {
        const int32 free_index = (start_word + offset) % word_count;

        size_t word = buf->free[free_index].load(memory_order_relaxed);
        while (true) {
            const int32 bit_index = chunk_find_first_zero_bit(word);
            if (bit_index < 0) {
                // word full, try next word
                break;
            }

            const int32 element = free_index * (sizeof(size_t) * 8) + bit_index;
            if (element >= buf->capacity) {
                // padding bits past capacity in the final word
                break;
            }

            const size_t new_word = word | (OMS_UINT_ONE << bit_index);
            if (buf->free[free_index].compare_exchange_weak(
                word, new_word, memory_order_acq_rel, memory_order_relaxed
            )) {
                buf->last_pos.store(element, memory_order_relaxed);
                DEBUG_MEMORY_WRITE((uintptr_t) (buf->memory + element * buf->chunk_size), buf->chunk_size);

                return element;
            }
            // word now holds the fresh value from the failed CAS,
            // look for another zero bit_index before moving on
        }
    }

    return -1;
}

inline
int32 chunk_reserve(ThrdChunkMemory* const buf, uint32 elements = 1) NO_EXCEPT
{
    if (elements <= 1) {
        return chunk_reserve_one(buf);
    }

    if ((int32) elements > buf->capacity) {
        return -1;
    }

    const int32 hint = buf->last_pos.load(memory_order_relaxed);
    int32 search_start = (hint < 0 ? 0 : hint + 1) % buf->capacity;

    for (int32 pass = 0; pass < 8; ++pass) {
        int32 run_start = -1;
        int32 run_len = 0;

        for (int32 i = 0; i < buf->capacity; ++i) {
            const int32 element = search_start + i;
            if (element >= buf->capacity) {
                // End reached, start at beginning
                run_start = -1;
                run_len = 0;
                break;
            }

            if (chunk_is_free(buf, (uint32) element)) {
                if (run_len == 0) {
                    run_start = element;
                }
                ++run_len;

                if (run_len == (int32) elements) {
                    if (chunk_try_claim_range(buf->free, run_start, (int32) elements)) {
                        buf->last_pos.store(run_start + (int32) elements - 1, memory_order_relaxed);
                        DEBUG_MEMORY_WRITE((uintptr_t) (buf->memory + run_start * buf->chunk_size), elements * buf->chunk_size);

                        return run_start;
                    }

                    // Lost the race on this run, the bitmap moved under us
                    // -> do a fresh scan
                    run_start = -1;
                    run_len = 0;
                }
            } else {
                run_start = -1;
                run_len = 0;
            }
        }

        // Nothing found this pass (or we kept losing races)
        // -> try again from the beginning
        search_start = 0;
    }

    return -1;
}

FORCE_INLINE
void chunk_free_element(ThrdChunkMemory* const buf, size_t free_index, int32 bit_index) NO_EXCEPT
{
    buf->completeness[free_index].fetch_and(~(OMS_UINT_ONE << bit_index), memory_order_acq_rel);
    buf->free[free_index].fetch_and(~(OMS_UINT_ONE << bit_index), memory_order_acq_rel);

    DEBUG_MEMORY_DELETE(
        (uintptr_t) (buf->memory + (free_index * (sizeof(size_t) * 8) + bit_index) * buf->chunk_size),
        buf->chunk_size
    );
}

FORCE_INLINE
void chunk_free_element(ThrdChunkMemory* const buf, uint32 element) NO_EXCEPT
{
    const uint32 free_index = element / (uint32) (sizeof(size_t) * 8);
    const uint32 bit_index = element % (uint32) (sizeof(size_t) * 8);

    buf->completeness[free_index].fetch_and(~(OMS_UINT_ONE << bit_index), memory_order_acq_rel);
    buf->free[free_index].fetch_and(~(OMS_UINT_ONE << bit_index), memory_order_acq_rel);

    DEBUG_MEMORY_DELETE((uintptr_t) (buf->memory + element * buf->chunk_size), buf->chunk_size);
}

FORCE_INLINE
void chunk_free_elements(ThrdChunkMemory* const buf, size_t element, uint32 element_count = 1) NO_EXCEPT
{
    chunk_clear_bit_range(buf->completeness, (int32) element, (int32) element_count);
    chunk_clear_bit_range(buf->free, (int32) element, (int32) element_count);
    DEBUG_MEMORY_DELETE((uintptr_t) (buf->memory + element * buf->chunk_size), buf->chunk_size * element_count);
}

FORCE_INLINE
void chunk_free_elements(ThrdChunkMemory* const buf, byte* data, uint32 element_count = 1) NO_EXCEPT
{
    const int32 element = chunk_id_from_memory(buf->memory, data, buf->chunk_size);
    chunk_clear_bit_range(buf->completeness, element, (int32) element_count);
    chunk_clear_bit_range(buf->free, element, (int32) element_count);
    DEBUG_MEMORY_DELETE((uintptr_t) (buf->memory + element * buf->chunk_size), buf->chunk_size * element_count);
}

FORCE_INLINE
void chunk_mark_complete(ThrdChunkMemory* const buf, uint32 element) NO_EXCEPT
{
    const uint32 free_index = element / (uint32) (sizeof(size_t) * 8);
    const uint32 bit_index = element % (uint32) (sizeof(size_t) * 8);

    buf->completeness[free_index].fetch_or(OMS_UINT_ONE << bit_index, memory_order_release);
}

FORCE_INLINE
void chunk_clear_complete(ThrdChunkMemory* const buf, uint32 element) NO_EXCEPT
{
    const uint32 free_index = element / (uint32) (sizeof(size_t) * 8);
    const uint32 bit_index = element % (uint32) (sizeof(size_t) * 8);

    buf->completeness[free_index].fetch_and(~(OMS_UINT_ONE << bit_index), memory_order_relaxed);
}

FORCE_INLINE
bool chunk_is_complete(const ThrdChunkMemory* const buf, uint32 element) NO_EXCEPT
{
    const uint32 free_index = element / (uint32) (sizeof(size_t) * 8);
    const uint32 bit_index = element % (uint32) (sizeof(size_t) * 8);

    return (buf->completeness[free_index].load(memory_order_acquire) & (OMS_UINT_ONE << bit_index)) != 0;
}

/**
 * Binary representation:
 *
 * 00 01 02 03 = capacity
 * 04 05 06 07 = last_pos
 * 08 09 0A 0B = free_offset
 * 0C .. .. .. = hash map data
 */
// @important dump/load bulk-memcpy this region as raw bytes. That's only a
// valid snapshot at a quiescent point - i.e. no concurrent chunk_reserve /
// chunk_free_elements / chunk_mark_complete in flight on this buffer while
// dumping or loading. This was already implicitly true under the old
// spinlock design (the lock was never held across the whole dump), it's just
// worth stating explicitly now that there's no lock to (mis-)rely on.
inline
int64 chunk_dump(const ThrdChunkMemory* const buf, byte* data) NO_EXCEPT
{
    LOG_1("[INFO] Dump ChunkMemoryT");
    const byte* const start = data;

    data = write_le(data, buf->capacity);
    data = write_le(data, buf->size);
    data = write_le(data, buf->chunk_size);
    data = write_le(data, buf->last_pos.load(memory_order_relaxed));

    const uint32 free_offset = (uint32) ((uintptr_t) buf->free - (uintptr_t) buf->memory);
    data = write_le(data, free_offset);

    // @todo also store completeness
    const size_t size = buf->capacity * buf->chunk_size
        + sizeof(size_t) * ceil_div(buf->capacity, (int32) (sizeof(size_t) * 8))
        + sizeof(size_t);

    // All memory is handled in the buffer -> simply copy the buffer
    // This also includes the free array
    memcpy(data, buf->memory, size);

    SWAP_ENDIAN_LITTLE_SIMD(
        (size_t *) (data + free_offset),
        (size_t *) (data + free_offset),
        buf->capacity / sizeof(size_t),
        8
    );

    data += size;

    LOG_1("[INFO] Dumped ChunkMemoryT: %n B", {DATA_TYPE_UINT64, (void *) &size});

    return data - start;
}

FORCE_INLINE
byte* chunk_memory_get(ThrdChunkMemory* const buf, uint32 elements) NO_EXCEPT
{
    const int32 element = chunk_reserve(buf, elements);

    return chunk_element_get(buf, element);
}

inline HOT_CODE
byte* memory_get(ThrdChunkMemory* const buf, size_t size) NO_EXCEPT
{
    return chunk_memory_get(buf, (uint32) ((size + buf->chunk_size - 1) / buf->chunk_size));
}

inline HOT_CODE
byte* memory_get_temp(ThrdChunkMemory* const buf, size_t size) NO_EXCEPT
{
    const uint32 element_count = (uint32) (size + buf->chunk_size - 1) / buf->chunk_size;
    byte* data = chunk_memory_get(buf, element_count);
    chunk_free_elements(buf, data, element_count);

    return data;
}

FORCE_INLINE
byte* chunk_memory_get_one(ThrdChunkMemory* const buf) NO_EXCEPT
{
    const int32 element = chunk_reserve_one(buf);

    return chunk_element_get(buf, element);
}

inline
int64 chunk_load(ThrdChunkMemory* const buf, const byte* data) NO_EXCEPT
{
    LOG_1("[INFO] Loading ChunkMemoryT");

    const byte* const start = data;

    data = read_le(data, &buf->capacity);
    data = read_le(data, &buf->size);
    data = read_le(data, &buf->chunk_size);

    int32 last_pos;
    data = read_le(data, &last_pos);
    buf->last_pos.store(last_pos, memory_order_relaxed);

    uint32 free_offset;
    data = read_le(data, &free_offset);

    // @todo also load completeness
    const size_t size = buf->capacity * buf->chunk_size
        + sizeof(size_t) * ceil_div(buf->capacity, (int32) (sizeof(size_t) * 8))
        + sizeof(size_t);

    memcpy(buf->memory, data, size);
    data += size;

    buf->free = (atomic<size_t> *) (((uintptr_t) buf->memory) + free_offset);

    SWAP_ENDIAN_LITTLE_SIMD(
        buf->free,
        buf->free,
        buf->capacity / sizeof(size_t),
        8
    );

    LOG_1("[INFO] Loaded ChunkMemoryT: %n B", {DATA_TYPE_UINT64, &size});

    return data - start;
}

/**
 * This allows us to use the chunk memory similar to a stack which automatically cleans itself up after leaving scope
 */
struct ThrdChunkStackMemory {
    ThrdChunkMemory* buffer;
    int32 element;
    uint32 count;

    HOT_CODE inline
    explicit ThrdChunkStackMemory(
        ThrdChunkMemory* buf,
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
    explicit ThrdChunkStackMemory(
        ThrdChunkMemory* buf,
        BufferMemory* mem,
        size_t size
    ) NO_EXCEPT
    {
        this->count = (uint32) ((size + buf->chunk_size - 1) / buf->chunk_size);
        this->buffer = buf;

        this->element = chunk_reserve(buf, this->count);
        byte* data = chunk_element_get(buf, this->element);

        buffer_init(mem, data, size, sizeof(size_t));
    }

    HOT_CODE inline
    ~ThrdChunkStackMemory() NO_EXCEPT
    {
        this->buffer->last_pos.store(this->element - 1);
        if (this->count == 1) {
            chunk_free_element(this->buffer, this->element);
        } else {
            chunk_free_elements(this->buffer, this->element, this->count);
        }
    }
};
#define THRD_CHUNK_STACK_MEMORY(buf, mem, size) ThrdChunkStackMemory __chunk_stack_##__func__##_##__LINE__((buf), (mem), (size))

// @performance Is _BitScanForward faster?
// @performance We could probably even reduce the number of iterations by only iterating until popcount is reached?
#define thrd_chunk_iterate_start(buf, chunk_id) {                                                        \
    uint32 free_index = 0;                                                                          \
    uint32 bit_index = 0;                                                                            \
                                                                                                       \
    /* Iterate the chunk memory */                                                                    \
    for (; chunk_id < (buf)->capacity; ++chunk_id) {                                                 \
        const size_t chunk_iter_word = (buf)->free[free_index].load(memory_order_relaxed);           \
        const size_t complete_iter_word = (buf)->completeness[free_index].load(memory_order_relaxed);           \
        /* Check if asset is defined */                                                              \
        if (!chunk_iter_word || !complete_iter_word) {                                                                       \
            /* Skip various elements */                                                              \
            /* @performance Consider to only check 1 byte instead of 8 */                            \
            /* There are probably even better ways by using compiler intrinsics if available */      \
            bit_index += (sizeof(size_t) * 8 - 1); /* +64 - 1 since the loop also increases by 1 */ \
            chunk_id += (sizeof(size_t) * 8 - 1);                                                    \
        } else if ((chunk_iter_word & (OMS_UINT_ONE << bit_index)) && complete_iter_word & (OMS_UINT_ONE << bit_index))

// INTERNAL: Not intended for use by any programmer
#define thrd_chunk_iterate_end_internal {                    \
        ++bit_index;                                    \
        if (bit_index > (sizeof(size_t) * 8 - 1)) {      \
            bit_index = 0;                               \
            ++free_index;                                \
        }                                                \
    }

// This is needed because if bit_index can be larger than 127 we need to skip multiple free_index
// But even for less than 127 we still may have to change the bit_index to a value != 0
// bit_index = 0 is only allowed for a 1 skip or (sizeof(size_t) * 8) skip (as used in thrd_chunk_iterate_end_internal)
// INTERNAL: Not intended for use by any programmer
#define thrd_chunk_iterate_end_internal_n(n) {                   \
        if (bit_index > (sizeof(size_t) * 8 - 1)) {          \
            bit_index %= (sizeof(size_t) * 8);                \
            free_index += ((n) / (sizeof(size_t) * 8));       \
        }                                                    \
    }

// Breaks out of the iteration (uses break, like you would use in a normal loop)
// #define thrd_chunk_iterate_break break

// Skip this element (uses continue, like you would use in a normal loop)
#define thrd_chunk_iterate_continue thrd_chunk_iterate_end_internal continue

// This is the fix to the skip from chunk_iterate_small_skip.
// Use only when actually needed.
// If the skip is guaranteed by the algorithm to be <=elements use chunk_iterate_small_skip
#define thrd_chunk_iterate_continue_n(n) {            \
        bit_index += (n);                        \
    } thrd_chunk_iterate_end_internal_n((n)) continue

// Ends the for loop from chunk_iterate_start
#define thrd_chunk_iterate_end thrd_chunk_iterate_end_internal }}

#endif
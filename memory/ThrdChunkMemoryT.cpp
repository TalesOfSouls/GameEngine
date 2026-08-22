/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_THRD_CHUNK_MEMORYT_C
#define COMS_MEMORY_THRD_CHUNK_MEMORYT_C

#include "ThrdChunkMemoryT.h"
#include "ThrdChunkMemory.cpp"

template <typename T>
inline
void chunk_init(
    ThrdChunkMemoryT<T>* const buf,
    byte* const data,
    int32 capacity,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    MAYBE_UNUSED const size_t size = capacity * sizeof(T)
        + sizeof(size_t) * array_count
        + sizeof(size_t) * array_count
        + sizeof(size_t) * 2;

    buf->memory = (T *) align_up((uintptr_t) data, alignment);

    buf->capacity = capacity;
    buf->last_pos.store(-1, memory_order_relaxed);

    buf->free = (atomic<size_t> *) align_up(
        (size_t) ((uintptr_t) (buf->memory + capacity)),
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
template <typename T>
inline
void chunk_alloc(ThrdChunkMemoryT<T>* const buf, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_CHUNK_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemoryT");

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    const size_t memory_size = capacity * sizeof(T)
        + sizeof(size_t) * array_count
        + sizeof(size_t) * array_count
        + sizeof(size_t) * 2;

    const size_t max_array_count = ceil_div(max_capacity, (int32) (sizeof(size_t) * 8));
    const size_t max_memory_size = max_capacity * sizeof(T)
        + sizeof(size_t) * max_array_count
        + sizeof(size_t) * max_array_count
        + sizeof(size_t) * 2;

    byte* buffer = (byte *) platform_alloc_aligned(
        memory_size,
        max_memory_size,
        alignment
    );

    chunk_init(buf, buffer, capacity, alignment);
}

// INFO: A chunk count of 2^n is recommended for maximum performance
template <typename T>
inline
void chunk_alloc(ThrdChunkMemoryT<T>* const buf, MemoryArena* const mem, int32 capacity, int32 max_capacity, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_CHUNK_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(max_capacity >= capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating ChunkMemoryT");

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    const size_t memory_size = capacity * sizeof(T)
        + sizeof(size_t) * array_count
        + sizeof(size_t) * array_count
        + sizeof(size_t) * 2;

    const size_t max_array_count = ceil_div(max_capacity, (int32) (sizeof(size_t) * 8));
    const size_t max_memory_size = max_capacity * sizeof(T)
        + sizeof(size_t) * max_array_count
        + sizeof(size_t) * max_array_count
        + sizeof(size_t) * 2;

    MemoryArena* arena = mem_arena_add(
        mem,
        memory_size,
        max_memory_size,
        alignment
    );
    chunk_init(buf, arena->memory, capacity, alignment);
}

template <typename T>
inline
void chunk_init(
    ThrdChunkMemoryT<T>* const buf,
    BufferMemory* const data,
    int32 capacity,
    int32 alignment = sizeof(size_t)
) NO_EXCEPT
{
    ASSERT_TRUE(capacity);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    const size_t array_count = ceil_div(capacity, (int32) (sizeof(size_t) * 8));
    const size_t size = capacity * sizeof(T)
        + sizeof(size_t) * array_count
        + sizeof(size_t) * array_count
        + sizeof(size_t) * 2;

    byte* buffer = memory_get(data, size, alignment);
    chunk_init(buf, buffer, capacity, alignment);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, size);
}

template <typename T>
inline
void chunk_free(ThrdChunkMemoryT<T>* const buf) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE(
        (uintptr_t) buf->memory,
        sizeof(T) * buf->capacity + sizeof(size_t) * ceil_div(buf->capacity, (int32) (sizeof(size_t) * 8))
    );

    platform_aligned_free((void **) &buf->memory);

    buf->capacity = 0;
    buf->memory = NULL;
}

template <typename T>
inline
void chunk_free(ThrdChunkMemoryT<T>* const buf, MemoryArena* const mem) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE((uintptr_t) buf->memory, sizeof(T) * buf->capacity + sizeof(size_t) * ceil_div(buf->capacity, (int32) (sizeof(size_t) * 8)));

    mem_arena_remove(mem, buf->memory);

    buf->capacity = 0;
    buf->memory = NULL;
}

template <typename T>
FORCE_INLINE FORCE_FLATTEN
T* chunk_element_get(const ThrdChunkMemoryT<T>* const buf, int32 element) NO_EXCEPT
{
    if (element >= buf->capacity) {
        return NULL;
    }

    T* const offset = &buf->memory[element];
    ASSERT_TRUE(offset);

    DEBUG_MEMORY_READ((uintptr_t) offset, sizeof(T));

    return offset;
}

template <typename T>
FORCE_INLINE
bool chunk_is_free(const ThrdChunkMemoryT<T>* const buf, uint32 element) NO_EXCEPT
{
    const uint32 free_index = element / (uint32) (sizeof(size_t) * 8);
    const uint32 bit_index = element % (uint32) (sizeof(size_t) * 8);

    return !(buf->free[free_index].load(memory_order_relaxed) & (OMS_UINT_ONE << bit_index));
}

template <typename T>
inline
int32 chunk_reserve_one(ThrdChunkMemoryT<T>* const buf) NO_EXCEPT
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
                DEBUG_MEMORY_WRITE((uintptr_t) &buf->memory[element], sizeof(T));

                return element;
            }
            // word now holds the fresh value from the failed CAS,
            // look for another zero bit_index before moving on
        }
    }

    return -1;
}

template <typename T>
inline
int32 chunk_reserve(ThrdChunkMemoryT<T>* const buf, uint32 elements = 1) NO_EXCEPT
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
                        DEBUG_MEMORY_WRITE((uintptr_t) &buf->memory[run_start], elements * sizeof(T));

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

template <typename T>
FORCE_INLINE
void chunk_free_element(ThrdChunkMemoryT<T>* const buf, size_t free_index, int32 bit_index) NO_EXCEPT
{
    buf->completeness[free_index].fetch_and(~(OMS_UINT_ONE << bit_index), memory_order_acq_rel);
    buf->free[free_index].fetch_and(~(OMS_UINT_ONE << bit_index), memory_order_acq_rel);

    DEBUG_MEMORY_DELETE(
        (uintptr_t) &buf->memory[(free_index * (sizeof(size_t) * 8) + bit_index)],
        sizeof(T)
    );
}

template <typename T>
FORCE_INLINE
void chunk_free_element(ThrdChunkMemoryT<T>* const buf, uint32 element) NO_EXCEPT
{
    const uint32 free_index = element / (uint32) (sizeof(size_t) * 8);
    const uint32 bit_index = element % (uint32) (sizeof(size_t) * 8);

    buf->completeness[free_index].fetch_and(~(OMS_UINT_ONE << bit_index), memory_order_acq_rel);
    buf->free[free_index].fetch_and(~(OMS_UINT_ONE << bit_index), memory_order_acq_rel);

    DEBUG_MEMORY_DELETE((uintptr_t) &buf->memory[element], sizeof(T));
}

template <typename T>
FORCE_INLINE
void chunk_free_elements(ThrdChunkMemoryT<T>* const buf, size_t element, uint32 element_count = 1) NO_EXCEPT
{
    chunk_clear_bit_range(buf->completeness, (int32) element, (int32) element_count);
    chunk_clear_bit_range(buf->free, (int32) element, (int32) element_count);
    DEBUG_MEMORY_DELETE((uintptr_t) &buf->memory[element], sizeof(T) * element_count);
}

template <typename T>
FORCE_INLINE
void chunk_free_elements(ThrdChunkMemoryT<T>* const buf, T* data, uint32 element_count = 1) NO_EXCEPT
{
    const int32 element = chunk_id_from_memory(buf->memory, data, sizeof(T));
    chunk_clear_bit_range(buf->completeness, element, (int32) element_count);
    chunk_clear_bit_range(buf->free, element, (int32) element_count);
    DEBUG_MEMORY_DELETE((uintptr_t) &buf->memory[element], sizeof(T) * element_count);
}

template <typename T>
FORCE_INLINE
void chunk_mark_complete(ThrdChunkMemoryT<T>* const buf, uint32 element) NO_EXCEPT
{
    const uint32 free_index = element / (uint32) (sizeof(size_t) * 8);
    const uint32 bit_index = element % (uint32) (sizeof(size_t) * 8);

    buf->completeness[free_index].fetch_or(OMS_UINT_ONE << bit_index, memory_order_release);
}

template <typename T>
FORCE_INLINE
void chunk_clear_complete(ThrdChunkMemoryT<T>* const buf, uint32 element) NO_EXCEPT
{
    const uint32 free_index = element / (uint32) (sizeof(size_t) * 8);
    const uint32 bit_index = element % (uint32) (sizeof(size_t) * 8);

    buf->completeness[free_index].fetch_and(~(OMS_UINT_ONE << bit_index), memory_order_relaxed);
}

template <typename T>
FORCE_INLINE
bool chunk_is_complete(const ThrdChunkMemoryT<T>* const buf, uint32 element) NO_EXCEPT
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
template <typename T>
inline
int64 chunk_dump(const ThrdChunkMemoryT<T>* const buf, byte* data) NO_EXCEPT
{
    LOG_1("[INFO] Dump ChunkMemoryT");
    const byte* const start = data;

    data = write_le(data, buf->capacity);
    data = write_le(data, buf->last_pos.load(memory_order_relaxed));

    const uint32 free_offset = (uint32) ((uintptr_t) buf->free - (uintptr_t) buf->memory);
    data = write_le(data, free_offset);

    // @todo also store completeness
    const size_t size = buf->capacity * sizeof(T)
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

template <typename T>
FORCE_INLINE
T* chunk_memory_get(ThrdChunkMemoryT<T>* const buf, uint32 elements) NO_EXCEPT
{
    const int32 element = chunk_reserve(buf, elements);

    return chunk_element_get(buf, element);
}

template <typename T>
inline HOT_CODE
T* memory_get(ThrdChunkMemoryT<T>* const buf, size_t size) NO_EXCEPT
{
    return chunk_memory_get(buf, (size + sizeof(T) - 1) / sizeof(T));
}

template <typename T>
inline HOT_CODE
T* memory_get_temp(ThrdChunkMemoryT<T>* const buf, size_t size) NO_EXCEPT
{
    const uint32 element_count = (uint32) (size + sizeof(T) - 1) / sizeof(T);
    T* data = chunk_memory_get(buf, element_count);
    chunk_free_elements(buf, data, element_count);

    return data;
}

template <typename T>
FORCE_INLINE
T* chunk_memory_get_one(ThrdChunkMemoryT<T>* const buf) NO_EXCEPT
{
    const int32 element = chunk_reserve_one(buf);

    return chunk_element_get(buf, element);
}

template <typename T>
inline
int64 chunk_load(ThrdChunkMemoryT<T>* const buf, const byte* data) NO_EXCEPT
{
    LOG_1("[INFO] Loading ChunkMemoryT");

    const byte* const start = data;

    data = read_le(data, &buf->capacity);

    int32 last_pos;
    data = read_le(data, &last_pos);
    buf->last_pos.store(last_pos, memory_order_relaxed);

    uint32 free_offset;
    data = read_le(data, &free_offset);

    // @todo also load completeness
    const size_t size = buf->capacity * sizeof(T)
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

#endif
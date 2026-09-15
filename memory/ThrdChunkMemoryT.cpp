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

    const size_t array_count = ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(capacity);
    MAYBE_UNUSED const size_t size = capacity * sizeof(T)
        + sizeof(size_t) * array_count
        + sizeof(size_t) * array_count
        + sizeof(size_t) * 2;

    buf->memory = (T *) ALIGN_UP((uintptr_t) data, alignment);

    buf->capacity = capacity;
    buf->last_pos.store(-1, memory_order_relaxed);

    buf->free = (atomic<size_t> *) ALIGN_UP(
        (size_t) ((uintptr_t) (buf->memory + capacity)),
        (size_t) alignof(size_t)
    );
    buf->completeness = (atomic<size_t> *) ALIGN_UP(
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

    const size_t array_count = ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(capacity);
    const size_t memory_size = capacity * sizeof(T)
        + sizeof(size_t) * array_count
        + sizeof(size_t) * array_count
        + sizeof(size_t) * 2;

    const size_t max_array_count = ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(max_capacity);
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

    const size_t array_count = ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(capacity);
    const size_t memory_size = capacity * sizeof(T)
        + sizeof(size_t) * array_count
        + sizeof(size_t) * array_count
        + sizeof(size_t) * 2;

    const size_t max_array_count = ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(max_capacity);
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

    const size_t array_count = ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(capacity);
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
        sizeof(T) * buf->capacity + sizeof(size_t) * ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(buf->capacity)
    );

    platform_aligned_free((void **) &buf->memory);

    buf->capacity = 0;
    buf->memory = NULL;
}

template <typename T>
inline
void chunk_free(ThrdChunkMemoryT<T>* const buf, MemoryArena* const mem) NO_EXCEPT
{
    DEBUG_MEMORY_DELETE((uintptr_t) buf->memory, sizeof(T) * buf->capacity + sizeof(size_t) * ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(buf->capacity));

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
int32 chunk_reserve_one_from_hint(ThrdChunkMemoryT<T>* const buf, int32 hint) NO_EXCEPT
{
    CONSTEXPR int32 bits_per_word = (int32) (sizeof(size_t) * 8);
    const int32 word_count = ceil_div_pow2<bits_per_word>(buf->capacity);
    const int32 clamped_hint = hint < 0 ? 0 : hint;
    const int32 start_word = (clamped_hint / bits_per_word) % word_count;
    const int32 start_bit = clamped_hint % bits_per_word;

    // Bits [0, start_bit) of the start word are handled separately below,
    // only after everything from start_bit onward (this word, then every
    // other word) has come up empty. This keeps the common case searching
    // forward from the hint for cache locality, instead of possibly
    // returning an earlier element from the very first word we look at.
    const size_t low_bits_mask = (start_bit == 0) ? (size_t) 0 : ((OMS_UINT_ONE << start_bit) - 1);

    for (int32 offset = 0; offset < word_count; ++offset) {
        const int32 free_index = (start_word + offset) % word_count;
        const size_t disallowed_mask = (free_index == start_word) ? low_bits_mask : (size_t) 0;

        size_t word = buf->free[free_index].load(memory_order_relaxed);
        while (true) {
            const int32 bit_index = chunk_find_first_zero_bit(word | disallowed_mask);
            if (bit_index < 0) {
                // word full (or nothing eligible here), try next word
                break;
            }

            const int32 element = free_index * bits_per_word + bit_index;
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
            // look for another eligible zero bit before moving on
        }
    }

    // Last resort: the bits before start_bit in the start word,
    // which were deliberately skipped above.
    if (low_bits_mask != 0) {
        size_t word = buf->free[start_word].load(memory_order_relaxed);
        while (true) {
            const int32 bit_index = chunk_find_first_zero_bit(word | ~low_bits_mask);
            if (bit_index < 0) {
                break;
            }

            const int32 element = start_word * bits_per_word + bit_index;
            if (element >= buf->capacity) {
                break;
            }

            const size_t new_word = word | (OMS_UINT_ONE << bit_index);
            if (buf->free[start_word].compare_exchange_weak(
                word, new_word, memory_order_acq_rel, memory_order_relaxed
            )) {
                buf->last_pos.store(element, memory_order_relaxed);
                DEBUG_MEMORY_WRITE((uintptr_t) &buf->memory[element], sizeof(T));

                return element;
            }
        }
    }

    return -1;
}

template <typename T>
inline
int32 chunk_reserve_one(ThrdChunkMemoryT<T>* const buf) NO_EXCEPT
{
    return chunk_reserve_one_from_hint(buf, buf->last_pos.load(memory_order_relaxed));
}

template <typename T>
inline
int32 chunk_reserve_one(
    ThrdChunkMemoryT<T>* const buf,
    const int32 hint
) NO_EXCEPT
{
    // Try to use hint as index
    {
        const int32 bits_per_word = (int32) (sizeof(size_t) * 8);
        const int32 free_index = hint / bits_per_word;
        const int32 bit_index  = hint % bits_per_word;

        size_t word = buf->free[free_index].load(memory_order_relaxed);

        // The hinted element is free.
        if ((word & (OMS_UINT_ONE << bit_index)) == 0) {
            const size_t new_word = word | (OMS_UINT_ONE << bit_index);

            if (buf->free[free_index].compare_exchange_weak(
                word, new_word, memory_order_acq_rel, memory_order_relaxed
            )) {
                buf->last_pos.store(hint, memory_order_relaxed);

                DEBUG_MEMORY_WRITE(
                    (uintptr_t)&buf->memory[hint],
                    sizeof(T)
                );

                return hint;
            }
        }
    }

    // chunk_reserve_one() function basically, but rooted at hint instead of
    // buf->last_pos
    return chunk_reserve_one_from_hint(buf, hint);
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
void chunk_free_element(ThrdChunkMemoryT<T>* const buf, void* data) NO_EXCEPT
{
    const uint32 element = (uint32) (((uintptr_t) data - (uintptr_t) buf->memory) / sizeof(T));
    chunk_free_element(buf, element);
}

template <typename T>
FORCE_INLINE
void chunk_free_elements(ThrdChunkMemoryT<T>* const buf, int32 element, uint32 element_count = 1) NO_EXCEPT
{
    chunk_clear_bit_range(buf->completeness, element, (int32) element_count);
    chunk_clear_bit_range(buf->free, element, (int32) element_count);
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
void chunk_mark_complete(ThrdChunkMemoryT<T>* const buf, void* data) NO_EXCEPT
{
    const uint32 element = (uint32) (((uintptr_t) data - (uintptr_t) buf->memory) / sizeof(T));
    chunk_mark_complete(buf, element);
}

template <typename T>
FORCE_INLINE
void chunk_element_insert(ThrdChunkMemoryT<T>* const __restrict buf, const T* const __restrict element) NO_EXCEPT
{
    const int32 element_id = chunk_reserve_one(buf);
    T* new_element = chunk_element_get(buf, element_id);
    memcpy(new_element, element, sizeof(T));

    chunk_mark_complete(buf, element_id);
}

template <typename T>
FORCE_INLINE
void chunk_element_insert(ThrdChunkMemoryT<T>* const buf, const T& element) NO_EXCEPT
{
    const int32 element_id = chunk_reserve_one(buf);
    T* new_element = chunk_element_get(buf, element_id);
    memcpy(new_element, &element, sizeof(T));

    chunk_mark_complete(buf, element_id);
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

template <typename T>
FORCE_INLINE
bool chunk_is_complete(const ThrdChunkMemoryT<T>* const buf, void* data) NO_EXCEPT
{
    const uint32 element = (uint32) (((uintptr_t) data - (uintptr_t) buf->memory) / sizeof(T));
    return chunk_is_complete(buf, element);
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
        + sizeof(size_t) * ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(buf->capacity)
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
        + sizeof(size_t) * ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(buf->capacity)
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
/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MEMORY_THREADED_CHUNK_MEMORY_H
#define COMS_MEMORY_THREADED_CHUNK_MEMORY_H

#include "../stdlib/Types.h"
#include "../thread/Thread.h"
#include "ChunkMemory.h"

struct ThreadedChunkMemory {
    byte* memory;

    uint64 size;
    uint32 last_pos;
    uint32 count;
    int32 chunk_size;
    int32 alignment;

    // length = count
    // free describes which locations are used and which are free
    atomic_64 uint64* free;

    // Chunk implementation ends here
    // The completeness indicates if the data is completely written to
    atomic_64 uint64* completeness;

    mutex lock;
};

// INFO: A chunk count of 2^n is recommended for maximum performance
inline
void thrd_chunk_alloc(ThreadedChunkMemory* const buf, uint32 count, int32 chunk_size, int32 alignment = 32)
{
    ASSERT_TRUE(chunk_size);
    ASSERT_TRUE(count);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    PROFILE(PROFILE_CHUNK_ALLOC, NULL, false, true);
    LOG_1("[INFO] Allocating ThreadedChunkMemory");

    chunk_size = OMS_ALIGN_UP(chunk_size, alignment);

    uint64 size = count * chunk_size
        + sizeof(uint64) * ceil_div(count, 64) // free
        + sizeof(uint64) * ceil_div(count, 64) // completeness
        + alignment * 3; // overhead for alignment

    size = OMS_ALIGN_UP(size, alignment);

    buf->memory = alignment < 2
        ? (byte *) platform_alloc(size)
        : (byte *) platform_alloc_aligned(size, alignment);

    buf->count = count;
    buf->size = size;
    buf->chunk_size = chunk_size;
    buf->last_pos = -1;
    buf->alignment = alignment;

    // @question Could it be beneficial to have this before the element data?
    buf->free = (uint64 *) OMS_ALIGN_UP((uintptr_t) (buf->memory + count * chunk_size), alignment);
    buf->completeness = (uint64 *) OMS_ALIGN_UP((uintptr_t) (buf->free + count), alignment);

    // @question Why is this even here?
    memset(buf->memory, 0, buf->size);
    mutex_init(&buf->lock, NULL);

    LOG_1("[INFO] Allocated ThreadedChunkMemory: %n B", {DATA_TYPE_UINT64, &buf->size});
}

inline
void thrd_chunk_init(ThreadedChunkMemory* const buf, BufferMemory* data, uint32 count, int32 chunk_size, int32 alignment = 32)
{
    ASSERT_TRUE(chunk_size);
    ASSERT_TRUE(count);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    chunk_size = OMS_ALIGN_UP(chunk_size, alignment);

    uint64 size = count * chunk_size
        + sizeof(uint64) * ceil_div(count, 64) // free
        + sizeof(uint64) * ceil_div(count, 64) // completeness
        + alignment * 3; // overhead for alignment

    buf->memory = buffer_get_memory(data, size);

    buf->count = count;
    buf->size = size;
    buf->chunk_size = chunk_size;
    buf->last_pos = -1;
    buf->alignment = alignment;

    // @question Could it be beneficial to have this before the element data?
    //  On the other hand the way we do it right now we never have to move past the free array since it is at the end
    //  On another hand we could by accident overwrite the values in free if we are not careful
    buf->free = (uint64 *) OMS_ALIGN_UP((uintptr_t) (buf->memory + count * chunk_size), alignment);
    buf->completeness = (uint64 *) OMS_ALIGN_UP((uintptr_t) (buf->free + count), alignment);

    mutex_init(&buf->lock, NULL);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

inline
void thrd_chunk_init(ThreadedChunkMemory* const buf, byte* data, uint32 count, int32 chunk_size, int32 alignment = 32)
{
    ASSERT_TRUE(chunk_size);
    ASSERT_TRUE(count);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    chunk_size = OMS_ALIGN_UP(chunk_size, alignment);

    uint64 size = count * chunk_size
        + sizeof(uint64) * ceil_div(count, 64) // free
        + sizeof(uint64) * ceil_div(count, 64) // completeness
        + alignment * 3; // overhead for alignment

    // @bug what if an alignment is defined?
    buf->memory = data;

    buf->count = count;
    buf->size = size;
    buf->chunk_size = chunk_size;
    buf->last_pos = -1;
    buf->alignment = alignment;

    // @question Could it be beneficial to have this before the element data?
    //  On the other hand the way we do it right now we never have to move past the free array since it is at the end
    //  On another hand we could by accident overwrite the values in free if we are not careful
    buf->free = (uint64 *) OMS_ALIGN_UP((uintptr_t) (buf->memory + count * chunk_size), alignment);
    buf->completeness = (uint64 *) OMS_ALIGN_UP((uintptr_t) (buf->free + count), alignment);

    mutex_init(&buf->lock, NULL);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

// @bug what if we used _init, we still need to destroy the mutex
// We have the same bug in other places as well
FORCE_INLINE
void thrd_chunk_free(ThreadedChunkMemory* const buf) NO_EXCEPT
{
    chunk_free((ChunkMemory *) buf);
    mutex_destroy(&buf->lock);
}

FORCE_INLINE
uint32 thrd_chunk_id_from_memory(const ThreadedChunkMemory* const buf, const byte* pos) NO_EXCEPT
{
    return chunk_id_from_memory((ChunkMemory *) buf, pos);
}

FORCE_INLINE
byte* thrd_chunk_get_element(ThreadedChunkMemory* const buf, uint32 element) NO_EXCEPT
{
    return chunk_get_element((ChunkMemory *) buf, element);
}

inline
void thrd_chunk_set_unset(uint32 element, atomic_64 uint64* state) NO_EXCEPT
{
    int32 free_index = element / 64;
    int32 bit_index = MODULO_2(element, 64);

    uint64 mask = ~(1ULL << bit_index);
    atomic_fetch_and_release(&state[free_index], mask);
}

int32 thrd_chunk_reserve_one(atomic_64 uint64* state, uint32 state_count, int32 start_index = 0) NO_EXCEPT
{
    if ((uint32) start_index >= state_count) {
        start_index = 0;
    }

    uint32 free_index = start_index / 64;
    uint32 bit_index = MODULO_2(start_index, 64);

    // Check standard simple solution
    uint64 current = atomic_get_acquire(&state[free_index]);
    if (!(current & (1ULL << bit_index))) {
        uint64_t desired = current | (1ULL << bit_index);
        if (atomic_compare_exchange_strong_acquire_release(&state[free_index], current, desired) == current) {
            return free_index * 64 + bit_index;
        }
    }

    for (uint32 i = 0; i < state_count; i += 64) {
        if (state[free_index] != 0xFFFFFFFFFFFFFFFF) {
            uint64 current_free = atomic_get_acquire(&state[free_index]);
            uint64 inverted = ~current_free;

            int32 bit_index;
            int32 j = 0; // We will only try 3 times to avoid infinite or long loops
            while (j < 3 && (bit_index = compiler_find_first_bit_r2l(inverted)) >= 0) {
                uint32 id = free_index * 64 + bit_index;
                if (id >= state_count) {
                    free_index = 0;

                    break;
                }

                uint64 new_free = current_free | (1ULL << bit_index);
                if ((new_free = atomic_compare_exchange_strong_acquire_release(&state[free_index], current_free, new_free)) == current_free) {
                    return id;
                }

                inverted = ~new_free;
                ++j;
            }
        } else {
            ++free_index;
            if (free_index * 64 >= state_count) {
                free_index = 0;
            }
        }
    }

    return -1;
}

FORCE_INLINE
int32 thrd_chunk_reserve(ThreadedChunkMemory* const buf, uint32 elements = 1) NO_EXCEPT
{
    MutexGuard _guard(&buf->lock);
    int32 free_element = chunk_reserve((ChunkMemory *) buf, elements);

    return free_element;
}

inline
void thrd_chunk_free_element(ThreadedChunkMemory* const buf, uint64 free_index, int32 bit_index) NO_EXCEPT
{
    atomic_64 uint64* target = &buf->free[free_index];
    uint64 old_value, new_value;

    do {
        old_value = atomic_get_relaxed(target);
        new_value = old_value | (1ULL << bit_index);

        if (old_value == new_value) {
            return;
        }
        // @bug Wrong use
    } while (!atomic_compare_exchange_strong_release(target, &old_value, new_value));

    DEBUG_MEMORY_DELETE((uintptr_t) (buf->memory + (free_index * 64 + bit_index) * buf->chunk_size), buf->chunk_size);
}

inline
void thrd_chunk_free_elements(ThreadedChunkMemory* const buf, uint64 element, uint32 element_count = 1) NO_EXCEPT
{
    uint64 free_index = element / 64;
    uint32 bit_index = MODULO_2(element, 64);

    if (element == 1) {
        thrd_chunk_free_element(buf, free_index, bit_index);
        return;
    }

    while (element_count > 0) {
        // Calculate the number of bits we can clear in the current 64-bit block
        uint32 bits_in_current_block = OMS_MIN(64 - bit_index, element_count);

        // Create a mask to clear the bits
        uint64 mask = ((1ULL << bits_in_current_block) - 1) << bit_index;

        uint64 old_value, new_value;
        atomic_64 uint64* target = &buf->free[free_index];

        do {
            old_value = atomic_get_relaxed(target);
            new_value = old_value & ~mask;

            if (old_value == new_value) {
                break;
            }
            // @bug Wrong use
        } while (!atomic_compare_exchange_strong_release(target, &old_value, new_value));

        // Update the counters and indices
        element_count -= bits_in_current_block;
        ++free_index;
        bit_index = 0;
    }

    DEBUG_MEMORY_DELETE((uintptr_t) (buf->memory + element * buf->chunk_size), buf->chunk_size);
}

// @performance We can optimize it by checking if we can just append additional chunks if they are free
inline
int32 thrd_chunk_resize(ThreadedChunkMemory* const buf, int32 element_id, uint32 elements_old, uint32 elements_new) NO_EXCEPT
{
    const byte* data = thrd_chunk_get_element(buf, element_id);

    int32 chunk_id = thrd_chunk_reserve(buf, elements_new);
    byte* data_new = thrd_chunk_get_element(buf, chunk_id);

    memcpy(data_new, data, buf->chunk_size * elements_old);

    // @see performance remark above
    //if (element_id != chunk_id) {
        thrd_chunk_free_elements(buf, element_id, elements_old);
    //}

    return chunk_id;
}

#endif
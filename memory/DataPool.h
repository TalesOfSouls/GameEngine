/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_DATA_POOL_H
#define COMS_MEMORY_DATA_POOL_H

#include "../stdlib/Stdlib.h"
#include "ChunkMemory.cpp"

// WARNING: Structure needs to be the same as ChunkMemory
// Opposite to the ChunkMemory we can see if someone currently has a pointer to the data
// This allows us to optimize the memory layout whenever data is unused
// @todo We need a HashMap so we can find elements
struct DataPool {
    byte* memory;

    // @question Do I really want to use uint?
    size_t size;
    int32 last_pos;
    uint32 capacity;
    int32 chunk_size;
    uint32 alignment;

    // length = count
    // free describes which locations are used and which are free
    size_t* free;

    // Chunk implementation ends here
    // This is a bit field that specifies which elements in the data pool are currently in use
    size_t* used;
};

// Number of size_t words needed to hold one bit per chunk
FORCE_INLINE CONSTEXPR
uint32 pool_bitmap_words(uint32 capacity) NO_EXCEPT
{
    return ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(capacity);
}

// INFO: A chunk count of 2^n is recommended for maximum performance
inline
void pool_alloc(DataPool* buf, uint32 capacity, int32 chunk_size, int32 alignment = sizeof(size_t))
{
    ASSERT_TRUE(chunk_size);
    ASSERT_TRUE(capacity);
    PROFILE_DEBUG(PROFILE_CHUNK_ALLOC, (char *) NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("[INFO] Allocating DataPool");

    const size_t size = capacity * chunk_size
        + sizeof(size_t) * ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(capacity) // free
        + sizeof(size_t) * ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(capacity) // used
        + alignof(size_t) * 3; // overhead for alignment

    buf->memory = (byte *) platform_alloc_aligned(size, size, alignment);

    buf->capacity = capacity;
    buf->size = size;
    buf->chunk_size = chunk_size;
    buf->last_pos = -1;
    buf->alignment = alignment;

    const uint32 bitmap_words = pool_bitmap_words(capacity);

    buf->free = (size_t *) ALIGN_UP((uintptr_t) (buf->memory + capacity * chunk_size), alignof(size_t));
    buf->used = buf->free + bitmap_words;

    // Only the two bitmaps have to be cleared, the data area comes zeroed from the OS
    memset(buf->free, 0, sizeof(size_t) * bitmap_words * 2);

    LOG_1("[INFO] Allocated DataPool: %n B", {DATA_TYPE_UINT64, &buf->size});
}

inline
void pool_init(DataPool* buf, BufferMemory* data, uint32 capacity, int32 chunk_size, int32 alignment = sizeof(size_t))
{
    ASSERT_TRUE(chunk_size);
    ASSERT_TRUE(capacity);

    chunk_size = ALIGN_UP(chunk_size, alignment);

    size_t size = capacity * chunk_size
        + sizeof(size_t) * ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(capacity) // free
        + sizeof(size_t) * ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(capacity) // used
        + alignof(size_t) * 3; // overhead for alignment

    buf->memory = memory_get(data, size);

    buf->capacity = capacity;
    buf->size = size;
    buf->chunk_size = chunk_size;
    buf->last_pos = -1;
    buf->alignment = alignment;

    const uint32 bitmap_words = pool_bitmap_words(capacity);

    buf->free = (size_t *) ALIGN_UP((uintptr_t) (buf->memory + capacity * chunk_size), alignof(size_t));
    buf->used = buf->free + bitmap_words;

    memset(buf->free, 0, sizeof(size_t) * bitmap_words * 2);
}

inline
void pool_init(DataPool* buf, byte* data, uint32 capacity, int32 chunk_size, int32 alignment = sizeof(size_t))
{
    ASSERT_TRUE(chunk_size);
    ASSERT_TRUE(capacity);

    chunk_size = ALIGN_UP(chunk_size, alignment);

    size_t size = capacity * chunk_size
        + sizeof(size_t) * ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(capacity) // free
        + sizeof(size_t) * ceil_div_pow2<(int32) (sizeof(size_t) * 8)>(capacity) // used
        + alignof(size_t) * 3; // overhead for alignment

    buf->memory = (byte *) ALIGN_UP((uintptr_t) data, (uintptr_t) alignment);

    buf->capacity = capacity;
    buf->size = size;
    buf->chunk_size = chunk_size;
    buf->last_pos = -1;
    buf->alignment = alignment;

    const uint32 bitmap_words = pool_bitmap_words(capacity);

    buf->free = (size_t *) ALIGN_UP((uintptr_t) (buf->memory + capacity * chunk_size), alignof(size_t));
    buf->used = buf->free + bitmap_words;

    memset(buf->free, 0, sizeof(size_t) * bitmap_words * 2);
}

FORCE_INLINE
void pool_free(DataPool* buf) NO_EXCEPT
{
    chunk_free((ChunkMemory *) buf);
}

FORCE_INLINE
int32 pool_reserve(DataPool* buf, uint32 elements = 1) NO_EXCEPT
{
    return chunk_reserve((ChunkMemory *) buf, elements);
}

FORCE_INLINE
byte* pool_element_get(DataPool* buf, uint32 element) NO_EXCEPT
{
    byte* data = chunk_element_get((ChunkMemory *) buf, element);
    if (!data) {
        return NULL;
    }

    OMS_BITARRAY_SET(buf->used, element);

    return data;
}

// Find a unused/unlocked element in the data pool
FORCE_INLINE
int32 pool_reserve_unused(DataPool* buf, int32 start_index = 0) NO_EXCEPT
{
    return chunk_reserve_one(buf->used, buf->capacity, start_index);
}

// Both memory_get variants hand out a pointer, so the element is marked as used
// and a subsequent pool_defragment() will not move it until pool_release()
FORCE_INLINE
byte* pool_memory_get(DataPool* const buf, uint32 elements) NO_EXCEPT
{
    byte* element = chunk_memory_get((ChunkMemory *) buf, elements);
    if (!element) {
        return NULL;
    }

    const uint32 id = chunk_id_from_memory(buf->memory, element, buf->chunk_size);
    OMS_BITARRAY_SET(buf->used, id);

    return element;
}

FORCE_INLINE
byte* pool_memory_get_one(DataPool* const buf) NO_EXCEPT
{
    byte* element = chunk_memory_get_one((ChunkMemory *) buf);
    if (!element) {
        return NULL;
    }

    const uint32 id = chunk_id_from_memory(buf->memory, element, buf->chunk_size);
    OMS_BITARRAY_SET(buf->used, id);

    return element;
}

// Release an element to be used by someone else
FORCE_INLINE
void pool_release(DataPool* buf, int32 element) NO_EXCEPT
{
    OMS_BITARRAY_CLEAR(buf->used, element);
}

FORCE_INLINE
void pool_defragment(DataPool* buf) NO_EXCEPT
{
    int32 dst = 0;
    int32 src = buf->capacity - 1;

    uint32 free_index = dst >> div_pow2(sizeof(size_t) * 8);
    uint32 bit_index = MODULO_2(dst, (sizeof(size_t) * 8));

    int32 free_src_index = src >> div_pow2(sizeof(size_t) * 8);
    int32 bit_src_index = MODULO_2(src, (sizeof(size_t) * 8));

    while (true) {
        // Find the next free slot
        while (dst < src && IS_BIT_SET_R2L(buf->free[free_index], bit_index)) {
            ++dst;

            ++bit_index;
            if (bit_index > (sizeof(size_t) * 8 - 1)) {
                bit_index = 0;
                ++free_index;
            }
        }

        // Find the last allocated but unused slot.
        while (dst < src) {
            if (IS_BIT_SET_R2L(buf->free[free_src_index], bit_src_index)
                && !IS_BIT_SET_R2L(buf->used[free_src_index], bit_src_index)
            ) {
                break;
            }

            --src;

            --bit_src_index;
            if (bit_src_index < 0) {
                bit_src_index = sizeof(size_t) * 8 - 1;
                --free_src_index;
            }
        }

        if (dst >= src) {
            break;
        }

        memcpy(
            buf->memory + dst * buf->chunk_size,
            buf->memory + src * buf->chunk_size,
            buf->chunk_size
        );

        buf->free[free_index] |= (OMS_UINT_ONE << bit_index);
        buf->used[free_index] &= ~(OMS_UINT_ONE << bit_index);

        buf->free[free_src_index] &= ~(OMS_UINT_ONE << bit_src_index);
        buf->used[free_src_index] &= ~(OMS_UINT_ONE << bit_src_index);

        ++dst;
        ++bit_index;
        if (bit_index > (sizeof(size_t) * 8 - 1)) {
            bit_index = 0;
            ++free_index;
        }

        --src;
        --bit_src_index;
        if (bit_src_index < 0) {
            bit_src_index = sizeof(size_t) * 8 - 1;
            --free_src_index;
        }
    }

    // Recompute last_pos = highest allocated chunk (64 chunks per word)
    buf->last_pos = -1;
    for (int32 word = ((int32) buf->capacity - 1) >> div_pow2(sizeof(size_t) * 8); word >= 0; --word) {
        if (!buf->free[word]) {
            continue;
        }

        const int32 pos = word * (int32) (sizeof(size_t) * 8)
            + compiler_find_first_bit_l2r(buf->free[word]);

        buf->last_pos = pos < (int32) buf->capacity ? pos : (int32) buf->capacity - 1;

        break;
    }
}

#endif
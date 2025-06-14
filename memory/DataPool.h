/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MEMORY_DATA_POOL_H
#define COMS_MEMORY_DATA_POOL_H

#include "../stdlib/Types.h"
#include "ChunkMemory.h"

// WARNING: Structure needs to be the same as RingMemory
struct DataPool {
    byte* memory;

    uint64 size;
    uint32 last_pos;
    uint32 count;
    uint32 chunk_size;
    int32 alignment;

    // length = count
    // free describes which locations are used and which are free
    alignas(8) uint64* free;

    // Chunk implementation ends here
    // This is a bit field that specifies which elements in the data pool are currently in use
    alignas(8) uint64* used;
};

// INFO: A chunk count of 2^n is recommended for maximum performance
inline
void pool_alloc(DataPool* buf, uint32 count, uint32 chunk_size, int32 alignment = 64)
{
    ASSERT_SIMPLE(chunk_size);
    ASSERT_SIMPLE(count);
    PROFILE(PROFILE_CHUNK_ALLOC, NULL, false, true);
    LOG_1("Allocating DataPool");

    chunk_size = ROUND_TO_NEAREST(chunk_size, alignment);

    uint64 size = count * chunk_size
        + sizeof(uint64) * CEIL_DIV(count, alignment) // free
        + sizeof(uint64) * CEIL_DIV(count, alignment) // used
        + alignment * 3; // overhead for alignment

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
    buf->used = (uint64 *) ROUND_TO_NEAREST((uintptr_t) (buf->free + count), 6alignment4);

    memset(buf->memory, 0, buf->size);

    LOG_1("Allocated DataPool: %n B", {{LOG_DATA_UINT64, &buf->size}});
}

inline
void pool_init(DataPool* buf, BufferMemory* data, uint32 count, uint32 chunk_size, int32 alignment = 64)
{
    ASSERT_SIMPLE(chunk_size);
    ASSERT_SIMPLE(count);

    chunk_size = ROUND_TO_NEAREST(chunk_size, alignment);

    uint64 size = count * chunk_size
        + sizeof(uint64) * CEIL_DIV(count, alignment) // free
        + sizeof(uint64) * CEIL_DIV(count, alignment) // used
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
    buf->free = (uint64 *) ROUND_TO_NEAREST((uintptr_t) (buf->memory + count * chunk_size), alignment);
    buf->used = (uint64 *) ROUND_TO_NEAREST((uintptr_t) (buf->free + count), alignment);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

inline
void pool_init(DataPool* buf, byte* data, uint32 count, uint32 chunk_size, int32 alignment = 64)
{
    ASSERT_SIMPLE(chunk_size);
    ASSERT_SIMPLE(count);

    chunk_size = ROUND_TO_NEAREST(chunk_size, alignment);

    uint64 size = count * chunk_size
        + sizeof(uint64) * CEIL_DIV(count, alignment) // free
        + sizeof(uint64) * CEIL_DIV(count, alignment) // used
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
    buf->free = (uint64 *) ROUND_TO_NEAREST((uintptr_t) (buf->memory + count * chunk_size), alignment);
    buf->used = (uint64 *) ROUND_TO_NEAREST((uintptr_t) (buf->free + count), alignment);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

FORCE_INLINE
void pool_free(DataPool* buf) noexcept
{
    chunk_free((ChunkMemory *) buf);
}

FORCE_INLINE
int32 pool_reserve(DataPool* buf, uint32 elements = 1) noexcept
{
    return chunk_reserve((ChunkMemory *) buf, elements);
}

FORCE_INLINE
byte* pool_get_element(DataPool* buf, uint64 element, bool zeroed = false) noexcept
{
    return chunk_get_element((ChunkMemory *) buf, element, zeroed);
}

// Find a unused/unlocked element in the data pool
FORCE_INLINE
int32 pool_get_unused(DataPool* buf, int32 start_index = 0) noexcept
{
    return chunk_get_unset(buf->used, buf->count, start_index);
}

// Release an element to be used by someone else
inline
void pool_release(DataPool* buf, int32 element) noexcept
{
    uint32 free_index = element / 64;
    uint32 bit_index = element & 63;
    buf->used[free_index] |= (1ULL << bit_index);
}

#endif
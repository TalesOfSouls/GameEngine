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

// WARNING: Structure needs to be the same as ChunkMemory
// Opposite to the ChunkMemory we can see if someone currently has a pointer to the data
// This allows us to optimize the memory layout whenever data is unused
// @todo We still need to implement this optimization by implementing a defragment function
//  ChunkMemory can not be defragmented because we don't know if a chunk is currently referenced or not
// @todo We also need a HashMap
struct DataPool {
    byte* memory;

    // @question Do I really want to use uint?
    uint64 size;
    int32 last_pos;
    uint32 count;
    uint32 chunk_size;
    uint32 alignment;

    // length = count
    // free describes which locations are used and which are free
    alignas(8) uint64* free;

    // Chunk implementation ends here
    // This is a bit field that specifies which elements in the data pool are currently in use
    alignas(8) uint64* used;
};

// INFO: A chunk count of 2^n is recommended for maximum performance
inline
void pool_alloc(DataPool* buf, uint32 count, uint32 chunk_size, int32 alignment = sizeof(size_t))
{
    ASSERT_TRUE(chunk_size);
    ASSERT_TRUE(count);
    PROFILE(PROFILE_CHUNK_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);
    LOG_1("Allocating DataPool");

    chunk_size = OMS_ALIGN_UP(chunk_size, alignment);

    uint64 size = count * chunk_size
        + sizeof(uint64) * CEIL_DIV(count, 64) // free
        + sizeof(uint64) * CEIL_DIV(count, 64) // used
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
    buf->free = (uint64 *) OMS_ALIGN_UP((uintptr_t) (buf->memory + count * chunk_size), alignment);
    buf->used = (uint64 *) OMS_ALIGN_UP((uintptr_t) (buf->free + count), alignment);

    memset(buf->memory, 0, buf->size);

    LOG_1("Allocated DataPool: %n B", {LOG_DATA_UINT64, &buf->size});
}

inline
void pool_init(DataPool* buf, BufferMemory* data, uint32 count, uint32 chunk_size, int32 alignment = sizeof(size_t))
{
    ASSERT_TRUE(chunk_size);
    ASSERT_TRUE(count);

    chunk_size = OMS_ALIGN_UP(chunk_size, alignment);

    uint64 size = count * chunk_size
        + sizeof(uint64) * CEIL_DIV(count, 64) // free
        + sizeof(uint64) * CEIL_DIV(count, 64) // used
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
    buf->used = (uint64 *) OMS_ALIGN_UP((uintptr_t) (buf->free + count), alignment);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

inline
void pool_init(DataPool* buf, byte* data, uint32 count, uint32 chunk_size, int32 alignment = sizeof(size_t))
{
    ASSERT_TRUE(chunk_size);
    ASSERT_TRUE(count);

    chunk_size = OMS_ALIGN_UP(chunk_size, alignment);

    uint64 size = count * chunk_size
        + sizeof(uint64) * CEIL_DIV(count, 64) // free
        + sizeof(uint64) * CEIL_DIV(count, 64) // used
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
    buf->used = (uint64 *) OMS_ALIGN_UP((uintptr_t) (buf->free + count), alignment);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
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
byte* pool_get_element(DataPool* buf, uint32 element) NO_EXCEPT
{
    return chunk_get_element((ChunkMemory *) buf, element);
}

// Find a unused/unlocked element in the data pool
FORCE_INLINE
int32 pool_reserve_unused(DataPool* buf, int32 start_index = 0) NO_EXCEPT
{
    return chunk_reserve_one(buf->used, buf->count, start_index);
}

// Release an element to be used by someone else
FORCE_INLINE
void pool_release(DataPool* buf, int32 element) NO_EXCEPT
{
    uint32 free_index = element / 64;
    uint32 bit_index = MODULO_2(element, 64);
    buf->used[free_index] |= (1ULL << bit_index);
}

#endif
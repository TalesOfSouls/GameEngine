/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MEMORY_THREADED_DATA_POOL_H
#define COMS_MEMORY_THREADED_DATA_POOL_H

#include "../stdlib/Types.h"
#include "DataPool.h"
#include "ThreadedChunkMemory.h"

/**
 * WARNING: This implementation assumes the initial setup (insertion of elements) is synchronous
 *          Only the retrieval of unused elements and the release are thread protected
 */

// WARNING: Structure needs to be the same as RingMemory
struct ThreadedDataPool {
    byte* memory;

    uint64 size;
    uint32 last_pos;
    uint32 count;
    uint32 chunk_size;
    int32 alignment;

    // length = count
    // free describes which locations are used and which are free
    alignas(8) atomic_64 uint64* free;

    // Chunk implementation ends here
    // This is a bit field that specifies which elements in the data pool are currently in use
    alignas(8) atomic_64 uint64* used;

    mutex mtx;
};

// INFO: A chunk count of 2^n is recommended for maximum performance
FORCE_INLINE
void thrd_pool_alloc(ThreadedDataPool* buf, uint32 count, uint32 chunk_size, int32 alignment = 64)
{
    pool_alloc((DataPool *) buf, count, chunk_size, alignment);
}

FORCE_INLINE
void thrd_pool_init(ThreadedDataPool* buf, BufferMemory* data, uint32 count, uint32 chunk_size, int32 alignment = 64)
{
    pool_init((DataPool *) buf, data, count, chunk_size, alignment);
}

FORCE_INLINE
void thrd_pool_init(ThreadedDataPool* buf, byte* data, uint32 count, uint32 chunk_size, int32 alignment = 64)
{
    pool_init((DataPool *) buf, data, count, chunk_size, alignment);
}

FORCE_INLINE
void thrd_pool_free(ThreadedDataPool* buf) noexcept
{
    chunk_free((ChunkMemory *) buf);
}

FORCE_INLINE
int32 thrd_pool_reserve(ThreadedDataPool* buf, uint32 elements = 1) noexcept
{
    return chunk_reserve((ChunkMemory *) buf, elements);
}

FORCE_INLINE
byte* thrd_pool_get_element(ThreadedDataPool* buf, uint64 element, bool zeroed = false) noexcept
{
    return chunk_get_element((ChunkMemory *) buf, element, zeroed);
}

// Find a unused/unlocked element in the data pool
FORCE_INLINE
int32 thrd_pool_get_unused(ThreadedDataPool* buf, int32 start_index = 0) noexcept
{
    return thrd_chunk_get_unset((ThreadedChunkMemory *) buf, buf->used, start_index);
}

// Release an element to be used by someone else
FORCE_INLINE
void thrd_pool_release(ThreadedDataPool* buf, int32 element) noexcept
{
    thrd_chunk_set_unset(element, buf->used);
}

#endif
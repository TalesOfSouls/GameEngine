/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_CHUNK_MEMORYT_H
#define COMS_MEMORY_CHUNK_MEMORYT_H

#include "../stdlib/Stdlib.h"
#include "../thread/ThreadDefines.h"
#include "../thread/Spinlock.h"

/**
 * This storage system is best used for fixed sized chunks
 * What I mean by that, every element has the same size.
 * Currently a caller could reserve multiple chunks to represent a single data entity
 * This can be fine in a single threaded application
 * However, this can lead to fragmentation which is hard to clean up because
 * we can't just defragment the memory since we don't know which chunks are currently in use
 * In use could mean by pointer of id. In use data isn't allowed to move or it would become "invalid"
 * If you need a data structure that can be defragmented use DataPool, which basically builds upon ChunkMemoryT
 * Fixed sized data structures that use this ChunkMemoryT can be:
 *      1. HashMap
 *      2. Queue
 * Carefull, both examples have alternative use cases which may require variable sized elements
 * WARNING: Changing this struct has effects on other data structures
 */
template <typename T>
struct ChunkMemoryT {
    T* memory;

    int32 capacity;
    int32 last_pos;

    // length = count
    // free describes which locations are used and which are free
    size_t* free;

    // Chunk implementation ends here
    // The completeness indicates if the data is completely written to
    size_t* completeness;

    spinlock32 lock;
};

#endif
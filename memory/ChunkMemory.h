/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_CHUNK_MEMORY_H
#define COMS_MEMORY_CHUNK_MEMORY_H

#include "../stdlib/Stdlib.h"
#include "../thread/ThreadDefines.h"

/**
 * This storage system is best used for fixed sized chunks
 * What I mean by that, every element has the same size.
 * Currently a caller could reserve multiple chunks to represent a single data entity
 * This can be fine in a single threaded application
 * However, this can lead to fragmentation which is hard to clean up because
 * we can't just defragment the memory since we don't know which chunks are currently in use
 * In use could mean by pointer of id. In use data isn't allowed to move or it would become "invalid"
 * If you need a data structure that can be defragmented use DataPool, which basically builds upon ChunkMemory
 * Fixed sized data structures that use this ChunkMemory can be:
 *      1. HashMap
 *      2. Queue
 * Carefull, both examples have alternative use cases which may require variable sized elements
 * WARNING: Changing this struct has effects on other data structures
 */
struct ChunkMemory {
    byte* memory;

    // @question Do I really want to use uint?
    uint_max size;
    atomic_32 int32 last_pos;
    uint32 capacity;
    int32 chunk_size;

    // WARNING: The alignment may increase the original chunk size e.g.
    // element_size = 14, alignment = sizeof(size_t) => chunk_size = 32
    uint32 alignment;

    // length = count
    // free describes which locations are used and which are free
    atomic_ptr uint_max* free;

    // Chunk implementation ends here
    // The completeness indicates if the data is completely written to
    atomic_ptr uint_max* completeness;

    mutex lock;
};

#endif
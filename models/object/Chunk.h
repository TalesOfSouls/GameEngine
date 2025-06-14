/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MODELS_OBJECT_CHUNK_H
#define COMS_MODELS_OBJECT_CHUNK_H

#include "../../stdlib/SIMD/SIMD_I32.h"
#include "../../../stdlib/Types.h"
#include "Block.h"
#include "Object.h"

struct Chunk {
    // 0 - x
    // 1 - y
    // 2 - z
    // 3 - size
    Stdlib::SIMD::int32_4_simd v;
};

struct ChunkDB {
    int x;
    int y;
    int z;

    int size;
};

struct BlockChunk {
    Chunk chunk;

    // Max size depends on CHUNK_SIZE (= CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE)
    Block *entities;
};

struct BlockObjectChunk {
    Chunk chunk;

    // Max size depends on CHUNK_SIZE (= CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE)
    BlockObject *entities;
};

struct ObjectChunk {
    Chunk chunk;

    // Max size is "unlimited" since multiple objects can be placed at any position
    Object *enteties;
};


#endif
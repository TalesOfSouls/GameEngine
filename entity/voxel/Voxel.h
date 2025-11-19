/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ENTITY_VOXEL_H
#define COMS_ENTITY_VOXEL_H

#include "../../stdlib/Types.h"
#include "../../stdlib/GameMathTypes.h"

enum VoxelRotation : byte {
    VOXEL_ROTATION_NONE = 0b00000000,

    VOXEL_ROTATION_X1 = 0b00000001,
    VOXEL_ROTATION_X2 = 0b00000010,
    VOXEL_ROTATION_X3 = 0b00000011,

    VOXEL_ROTATION_Y1 = 0b00000100,
    VOXEL_ROTATION_Y2 = 0b00001000,
    VOXEL_ROTATION_Y3 = 0b00001100,

    VOXEL_ROTATION_Z1 = 0b00010000,
    VOXEL_ROTATION_Z2 = 0b00100000,
    VOXEL_ROTATION_Z3 = 0b00110000,

    // we are not using 2 bits
    // @todo consider to use them for some smart optimization
    //      Maybe allow to flag "has neighbour" or something similar
    //      Or, is dynamic, updated, ....
};

enum CoordAxis {
    COORD_AXIS_X = 1 << 0,
    COORD_AXIS_Y = 1 << 1,
    COORD_AXIS_Z = 1 << 2
};

struct Voxel {
    uint16 type;
    uint8 rotation;
};

// For now the same as a Voxel but this might change
struct VoxelFace {
    uint16 type;
    uint8 rotation;
};

// Used for greedy meshing
// This holds temporary information about the Face
struct VoxelMaskCell {
    VoxelFace face;

    // @performance we probably want this to be outside as a bit field -> much more memory efficient
    bool is_filled;
};

// @todo Move to Voxel type enum
FORCE_INLINE
bool voxel_is_solid(int32 type) NO_EXCEPT {
    return type != 0;
}

// @question I remember that having a 30 voxel size is better in some situations
//          We could use a uint32 to define a bitfield that describes which field is set
//          30 because we could check if the neighbour chunk also has a block set
// WARNING: MUST be divisible by 2
#define VOXEL_CHUNK_SIZE 32

struct VoxelChunkMesh {
    // Interleaved vertex: position (f323), normal (packed int8x3),
    // type (uint16), rotation (uint8), padding (uint8)
    // Layout kept simple; adjust to your renderer.
    // @question Consider to change some of the types below vectors (vertices and normals at least)
    v3_f32 vertices[VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * 3]; // [num_vertices * 3]
    uint16 types[VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE]; // [num_vertices] // @question not sure i need this information here
    v3_byte normals[VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE]; // [num_vertices * 3]
    uint8 rotations[VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE];// [num_vertices] // @question not sure i need this information here
    uint32 indices[VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * 2]; // [num_vertices * 2]
    uint32 num_vertices;
    uint32 num_indices;
    uint32 cap_vertices;
    uint32 cap_indices;
};

enum VoxelChunkFlag : byte {
    VOXEL_CHUNK_FLAG_NONE = 0,
    VOXEL_CHUNK_FLAG_IS_NEW = 1 << 0,
    VOXEL_CHUNK_FLAG_IS_CHANGED = 1 << 1,
    VOXEL_CHUNK_FLAG_SHOULD_REMOVE = 1 << 2,
};

// CPU-side mesh buffers (triangulated greedy mesh)
// You can upload these to GPU when built.
// @performance The current implementation is incredibly space inefficient
//  We are using always the same chunk size even if it only contains one block
struct VoxelChunk {
    // This is the count of elements used in the DataPool memory
    // Strictly speaking this is not required because we could calculate the element count based on the data below
    // However, then we would have to calculate it every time.
    // We are exchanging memory for computational headroom
    int32 element_count;

    // The chunk coordinate in world space
    v3_int32 coord;

    // @todo This should be variable size and be positioned after the Chunk
    // The ChunkMemory which contains all the chunks then will hold multiple elements
    // 1 element = VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * sizeof(Voxel) + sizeof(VoxelChunk)
    Voxel vox[VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE];

    // Uses VoxelChunkFlag
    byte flag;

    // World-space AABB
    // Used to check if the chunk intersects with the view frustum planes
    AABB_int32 bounds;

    VoxelChunkMesh mesh;
};

FORCE_INLINE
VoxelChunk voxel_chunk_create(int32 x, int32 y, int32 z) NO_EXCEPT {
    VoxelChunk chunk = {};
    chunk.coord = {x, y, z};
    chunk.bounds.min = {x * VOXEL_CHUNK_SIZE, y * VOXEL_CHUNK_SIZE, z * VOXEL_CHUNK_SIZE};
    chunk.bounds.max = {(x + 1) * VOXEL_CHUNK_SIZE, (y + 1) * VOXEL_CHUNK_SIZE, (z + 1) * VOXEL_CHUNK_SIZE};

    return chunk;
}

// Calculates the index in a 1-dimensional array
// 1. Check depth coordinate (move an entire plane)
// 2. Then check height coordinate (move an entire row)
// 3. Lastly check width coordinate (move individual voxels)
static FORCE_INLINE
int32 voxel_index_get(int32 x, int32 y, int32 z) NO_EXCEPT {
    return x + VOXEL_CHUNK_SIZE * (y + VOXEL_CHUNK_SIZE * z);
}

FORCE_INLINE
Voxel voxel_chunk_get(const VoxelChunk* chunk, int32 x, int32 y, int32 z) NO_EXCEPT
{
    if((uint32) x >= (uint32) VOXEL_CHUNK_SIZE
        || (uint32) y >= (uint32) VOXEL_CHUNK_SIZE
        || (uint32) z >= (uint32) VOXEL_CHUNK_SIZE
    ) {
        return {0,0};
    }

    return chunk->vox[voxel_index_get(x, y, z)];
}

FORCE_INLINE
void voxel_chunk_set(VoxelChunk* chunk, int32 x, int32 y, int32 z, Voxel v) NO_EXCEPT
{
    if((uint32) x >= (uint32) VOXEL_CHUNK_SIZE
        || (uint32) y >= (uint32) VOXEL_CHUNK_SIZE
        || (uint32) z >= (uint32) VOXEL_CHUNK_SIZE
    ) {
        return;
    }

    chunk->vox[voxel_index_get(x, y, z)] = v;
    chunk->flag |= VOXEL_CHUNK_FLAG_IS_CHANGED;
}

inline
void voxel_chunk_vertex_push(
    VoxelChunk* __restrict chunk,
    v3_f32 coord,
    v3_byte normal,
    const VoxelFace* __restrict face
) NO_EXCEPT {
    // We currently don't support growing chunks
    ASSERT_TRUE(chunk->mesh.num_vertices <= VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * 3);

    uint32 i = chunk->mesh.num_vertices++;

    chunk->mesh.vertices[i] = coord;

    chunk->mesh.normals[i].x = normal.x * 127 + 127;
    chunk->mesh.normals[i].y = normal.y * 127 + 127;
    chunk->mesh.normals[i].z = normal.z * 127 + 127;

    chunk->mesh.types[i] = face->type;
    chunk->mesh.rotations[i] = face->rotation;
}

inline
void voxel_chunk_quad_push(
    VoxelChunk* chunk,
    uint32 v0, uint32 v1, uint32 v2, uint32 v3
) NO_EXCEPT {
    ASSERT_TRUE(chunk->mesh.num_indices <= chunk->mesh.cap_indices);

    chunk->mesh.indices[chunk->mesh.num_indices + 0] = v0;
    chunk->mesh.indices[chunk->mesh.num_indices + 1] = v2;
    chunk->mesh.indices[chunk->mesh.num_indices + 2] = v1;

    chunk->mesh.indices[chunk->mesh.num_indices + 3] = v0;
    chunk->mesh.indices[chunk->mesh.num_indices + 4] = v3;
    chunk->mesh.indices[chunk->mesh.num_indices + 5] = v2;

    chunk->mesh.num_indices += 6;
}

#endif
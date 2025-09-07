/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ENTITY_VOXEL_MAP_C
#define COMS_ENTITY_VOXEL_MAP_C

#include "../stdlib/Types.h"
#include "../stdlib/GameMathTypes.h"
#include "../stdlib/HashMap.h"

struct Voxel {
    uint16 type;
    uint8 rotation;
};

// For now the same as a Voxel but this might change
struct VoxelFace {
    uint16 type;
    uint8 rotation;
};

#define VOXEL_CHUNK_SIZE 32

struct VoxelChunkMesh {
    // Interleaved vertex: position (float3), normal (packed int8x3),
    // type (uint16), rotation (uint8), padding (uint8)
    // Layout kept simple; adjust to your renderer.
    f32 vertices[VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * 3 * sizeof(f32)]; // [num_vertices * 3]
    uint16 types[VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * sizeof(uint16)]; // [num_vertices] // @question not sure i need this information here
    uint8 normals[VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * 3 * sizeof(uint8)]; // [num_vertices * 3]
    uint8 rotations[VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * sizeof(uint8)];// [num_vertices] // @question not sure i need this information here
    uint32 indices[VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * 2 *sizeof(uint32)]; // [num_vertices * 2]
    uint32 num_vertices;
    uint32 num_indices;
    uint32 cap_vertices;
    uint32 cap_indices;
};

// CPU-side mesh buffers (triangulated greedy mesh)
// You can upload these to GPU when built.
// @performance The current implementation is incredibly space inefficient
//  We are using always the same chunk size even if it only contains one block
struct VoxelChunk {
    v3_int32 coord;

    Voxel vox[VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE];

    // Needs remesh
    bool dirty;

    // World-space AABB
    // Used to check if the chunk intersects with the view frustum planes
    AABB bounds;

    VoxelChunkMesh mesh;
}

static inline
uint64 voxel_chunk_coord_pack(int32 x, int32 y, int32 z) NO_EXCEPT {
    // We check if only the lowest 21 bits are set
    ASSERT_STRICT((((uint32)(x)) & ~((1u << 21) - 1u)) == 0);
    ASSERT_STRICT((((uint32)(y)) & ~((1u << 21) - 1u)) == 0);
    ASSERT_STRICT((((uint32)(z)) & ~((1u << 21) - 1u)) == 0);

    // 21 bits per axis signed (fits +/- 1,048,575)
    uint64 ux = ((uint64) (uint32) x) & 0x1FFFFF;
    uint64 uy = ((uint64) (uint32) y) & 0x1FFFFF;
    uint64 uz = ((uint64) (uint32) z) & 0x1FFFFF;

    return (ux) | (uy << 21) | (uz << 42);
}

HashEntry* hashmap_insert(HashMap* hm, int32 x, int32 y, int32 z, VoxelChunk* chunk) NO_EXCEPT {
    // @question Do we really want to do it like this or should the data be variable length?
    // This means we would have to reserve n chunks based on the size of the VoxelChunk
    int32 element = chunk_reserve_one(&hm->buf);
    if (element < 0) {
        return NULL;
    }

    HashEntry* entry = (HashEntry *) chunk_get_element(&hm->buf, element, true);

    entry->value = (byte *) entry + sizeof(HashEntry);

    uint64 key = voxel_chunk_coord_pack(x, y, z);
    *((uint64 *) entry->key) = key;

    // @bug this doesn't fully copy the Chunk since the chunk itself also has pointers
    // We need to memcpy that data as well, we should assume that it is stored at the end of the Chunk
    memcpy(entry->value, value, sizeof(*chunk));

    uint64 index = key % hm->buf.count;
    uint16* target = &hm->table[index];
    while (*target) {
        HashEntry* tmp = (HashEntry*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);

    return entry;
}

// @performance If we had a doubly linked list we could delete keys much easier
// However that would make insertion slower
// Maybe we create a nother hashmap that is doubly linked
void hashmap_remove(HashMap* hm, int32 x, int32 y, int32 z) NO_EXCEPT {
    uint64 key = voxel_chunk_coord_pack(x, y, z);
    uint64 index = key % hm->buf.count;
    HashEntry* entry = (HashEntry *) chunk_get_element(&hm->buf, hm->table[index] - 1, false);
    HashEntry* prev = NULL;

    uint32 element_id = hm->table[index];

    while (entry != NULL) {
         if (*((uint64 *) entry->key) == key) {
            if (prev == NULL) {
                hm->table[index] = entry->next;
            } else {
                prev->next = entry->next;
            }

            chunk_free_elements(&hm->buf, element_id - 1);

            return;
        }

        element_id = entry->next;
        prev = entry;
        entry = (HashEntry *) chunk_get_element(&hm->buf, entry->next - 1, false);
    }
}

inline
HashEntry* hashmap_get_entry(HashMap* hm, int32 x, int32 y, int32 z) NO_EXCEPT {
    uint64 key = voxel_chunk_coord_pack(x, y, z);
    uint64 index = key % hm->buf.count;
    HashEntry* entry = (HashEntry *) chunk_get_element(&hm->buf, hm->table[index] - 1, false);

    while (entry != NULL) {
        if (*((uint64 *) entry->key) == key) {
            DEBUG_MEMORY_READ((uintptr_t) entry, sizeof(HashEntry));
            return entry;
        }

        entry = (HashEntry *) chunk_get_element(&hm->buf, entry->next - 1, false);
    }

    return NULL;
}

inline
VoxelChunk* hashmap_get_value(HashMap* hm, int32 x, int32 y, int32 z) NO_EXCEPT {
    HashEntryVoidP* entry = (HashEntryVoidP *) hashmap_get_entry(hm, x, y, z);

    return entry ? (VoxelChunk *) entry->value : NULL;
}

// By using this we can directly fill the hash map without copying data over
HashEntry* hashmap_reserve_value(HashMap* hm, const char* key) NO_EXCEPT {
    int32 element = chunk_reserve_one(&hm->buf);
    if (element < 0) {
        return NULL;
    }

    HashEntry* entry = (HashEntry *) chunk_get_element(&hm->buf, element, true);

    entry->value = (byte *) entry + sizeof(HashEntry);

    *((uint64 *) entry->key) = voxel_chunk_coord_pack(x, y, z);

    entry->next = 0;

    uint16* target = &hm->table[index];
    while (*target) {
        HashEntry* tmp = (HashEntry*) chunk_get_element(&hm->buf, *target - 1, false);
        target = &tmp->next;
    }
    *target = (uint16) (element + 1);

    return entry;
}

FORCE_INLINE
void voxel_chunk_create(VoxelChunk* chunk, int32 x, int32 y, int32 z) {
    chunk->bounds.min = {x * VOXEL_CHUNK_SIZE, y * VOXEL_CHUNK_SIZE, z * VOXEL_CHUNK_SIZE};
    chunk->bounds.max = {(x + 1) * VOXEL_CHUNK_SIZE, (y + 1) * VOXEL_CHUNK_SIZE, (z + 1) * VOXEL_CHUNK_SIZE};
    chunk->cap_vertices = 1024;
    chunk->cap_indices = 1024 * 2;
}

// Calculates the index in a 1-dimensional array
// 1. Check depth coordinate (move an entire plane)
// 2. Then check height coordinate (move an entire row)
// 3. Lastly check width coordinate (move individual voxels)
static FORCE_INLINE
int32 voxel_index_get(int32 x, int32 y, int32 z) {
    return x + VOXEL_CHUNK_SIZE * (y + VOXEL_CHUNK_SIZE * z);
}

FORCE_INLINE
Voxel voxel_chunk_get(const VoxelChunk* chunk, int32 x, int32 y, int32 z)
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
void voxel_chunk_set(VoxelChunk* chunk, int32 x, int32 y, int32 z, Voxel v)
{
    if((uint32) x >= (uint32) VOXEL_CHUNK_SIZE
        || (uint32) y >= (uint32) VOXEL_CHUNK_SIZE
        || (uint32) z >= (uint32) VOXEL_CHUNK_SIZE
    ) {
        return;
    }

    chunk->vox[voxel_index_get(x, y, z)] = v;
    chunk->dirty = true;
}

inline
void voxel_chunk_vertex_push(
    VoxelChunk* chunk,
    f32 x, f32 y, f32 z,
    int32 nx, int32 ny, int32 nz, // Normal
    VoxelFace a
) {
    // We currently don't support growing chunks
    ASSERT_TRUE(chunk->mesh.num_vertices <= VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE);

    uint32 i = chunk->mesh.num_vertices++;

    chunk->mesh.vertices[i * 3 + 0] = x;
    chunk->mesh.vertices[i * 3 + 1] = y;
    chunk->mesh.vertices[i * 3 + 2] = z;

    c->mesh.normals[i * 3 + 0] = (uint8) (nx * 127 + 127);
    c->mesh.normals[i * 3 + 1] = (uint8) (ny * 127 + 127);
    c->mesh.normals[i * 3 + 2] = (uint8) (nz * 127 + 127);

    c->mesh.types[i] = a.type;
    c->mesh.rotations[i] = a.rotation;
}

inline
void voxel_chunk_quad_push(
    VoxelChunk* chunk,
    uint32 v0, uint32 v1, uint32 v2, uint32 v3
) {
    ASSERT_TRUE(chunk->mesh.num_indices <= chunk->mesh.cap_indices);

    chunk->mesh.indices[chunk->mesh.num_indices + 0] = v0;
    chunk->mesh.indices[chunk->mesh.num_indices + 1] = v2;
    chunk->mesh.indices[chunk->mesh.num_indices + 2] = v1;

    chunk->mesh.indices[chunk->mesh.num_indices + 3] = v0;
    chunk->mesh.indices[chunk->mesh.num_indices + 4] = v3;
    chunk->mesh.indices[chunk->mesh.num_indices + 5] = v2;

    chunk->mesh.num_indices += 6;
}

// Gets a voxel based on global/world map coordinates
inline
Voxel voxel_world_map_get(
    HashMap* map,
    int32 chunk_x, int32 chunk_y, int32 chunk_z,
    int32 x, int32 y, int32 z,
) {
    while(x < 0) { x += VOXEL_CHUNK_SIZE; --chunk_x; }
    while(y < 0) { y += VOXEL_CHUNK_SIZE; --chunk_y; }
    while(z < 0) { z += VOXEL_CHUNK_SIZE; --chunk_z; }
    while(x >= VOXEL_CHUNK_SIZE) { x -= VOXEL_CHUNK_SIZE; ++chunk_x; }
    while(y >= VOXEL_CHUNK_SIZE) { y -= VOXEL_CHUNK_SIZE; ++chunk_y; }
    while(z >= VOXEL_CHUNK_SIZE) { z -= VOXEL_CHUNK_SIZE; ++chunk_z; }

    HashEntry* entry = hashmap_get_entry(map, chunk_x, chunk_y, chunk_z);
    if (!entry) {
        // Is air
        return {0, 0};
    }

    VoxelChunk* chunk = (VoxelChunk *) entry->value;

    return voxel_get(chunk, x, y, z);
}

// Builds the greedy mash of a chunk
// Requires the neighboring chunks
void voxel_chunk_mesh_build(HashMap* map, VoxelChunk* chunk)
{
    chunk->mesh.num_vertices = 0;
    chunk->mesh.num_indices = 0;

    // Build quads between solid/air slabs
    // x,y,z are the axis 0-2
    for (int32 axis = 0; axis < 3; ++axis) {
        int32 u = (axis + 1) % 3;
        int32 v = (axis + 2) % 3;

        int32 du[3] = {}; du[axis] = 1;
        int32 dv[3] = {}; dv[v] = 1;
        int32 duu[3] = {}; duu[u] = 1;

        for (int32 d = -1; d < VOXEL_CHUNK_SIZE; ++d) {
            int32 x_offset = axis == 0 ? d : 0;
            int32 y_offset = axis == 1 ? d : 0;
            int32 z_offset = axis == 2 ? d : 0;

            for (int32 j = 0; j < VOXEL_CHUNK_SIZE; ++j) {
                int32 dv_x = x_offset + j * dv[0];
                int32 dv_y = y_offset + j * dv[1];
                int32 dv_z = z_offset + j * dv[2];

                for (int32 i = 0; i < VOXEL_CHUNK_SIZE; ++i) {
                    int32 x = dv_x + i * duu[0];
                    int32 y = dv_y + i * duu[1];
                    int32 z = dv_z + i * duu[2];

                    Voxel a;
                    if (d >= 0) {
                        a = voxel_world_map_get(map, chunk->coord.x, chunk->coord.y, chunk->coord.z, x, y, z);
                    } else {
                        a = {0, 0};
                    }

                    Voxel b;
                    if (d < VOXEL_CHUNK_SIZE - 1) {
                        b = voxel_world_map_get(map, chunk->coord.x, chunk->coord.y, chunk->coord.z, x + du[0], y + du[1], z + du[2]);
                    } else {
                        b = {0, 0};
                    }

                    // If one voxel is air and the other is solid we need to create a face
                    // @todo This needs to be expanded to semi-transparent voxels as well
                    if ((a.type == 0) != (b.type == 0)) {

                    } else {

                    }
                }
            }
        }
    }
}

struct VoxelOctNode {
    AABB box; // loose bounds
    VoxelOctNode* child[8]; // children or NULL
    VoxelChunk** chunks; // dynamic array of chunks overlapping this node
    uint32 count;
    uint32 cap;
};

struct VoxelOctree {
    VoxelOctNode* root;
    int32 max_depth; // e.g., 8
    f32 looseness; // >1.0 (e.g., 1.5)
};

static
void voxel_octree_rebuild(Octree* t, const ChunkMap* map)
{
    // destroy and rebuild from chunk map (simple approach; can be made incremental)
    octnode_free(t->root);

    // compute bounds from map contents, fallback to origin cube
    aabb wb = { v3(-512,-512,-512), v3(512,512,512) };
    t->root = octnode_create(aabb_loosen(wb, t->looseness));

    // iterate map slots
    for(uint32_t i = 0; i < map->cap; ++i) {
        Chunk* c = map->vals[i];

        if(!c) {
            continue;
        }

        octnode_insert(t->root, c, 0, t->max_depth);
    }
}

struct VoxelWorld {
    HashMap map; // sparse chunks
    VoxelOctree oct; // spatial index (optional)
    bool use_octree;
};

static inline
void voxel_world_remesh_dirty(VoxelWorld* vw) NO_EXCEPT {
    for(uint32 i = 0; i < vw->map.buf.count; ++i) {
        VoxelChunk* chunk = w->map.vals[i];
        if(!chunk) {
            continue
        };

        if(chunk->dirty) {
            voxel_chunk_mesh_build(chunk, &vw->map);
        }
    }

    if(vw->use_octree) {
        voxel_octree_rebuild(&vw->oct, &vw->map);
    }
}

static inline
VoxelChunk* voxel_world_get_or_create_chunk(VoxelWorld* vw, int32 cx, int32 cy, int32 cz) NO_EXCEPT
{
    HashEntry* entry = hashmap_get_entry(&vw->map, cx, cy, cz);
    if(entry) {
        return (VoxelChunk *) entry->value;
    }

    // @performance Maybe use hashmap_reserve and directly fill it instead of copying it over
    VoxelChunk chunk = {};
    voxel_chunk_create(&chunk, cx, cy, cz);

    entry = hashmap_insert(&vw->map, cx, cy, cz, &chunk);

    return (VoxelChunk *) entry->value;
}

static inline
void voxel_world_voxel_set(VoxelWorld* vw, int32 world_x, int32 world_y, int32 world_z, Voxel v) NO_EXCEPT
{
    int32 cx = (int32) FLOORF((f32) world_x / VOXEL_CHUNK_SIZE);
    int32 cy = (int32) FLOORF((f32) world_y / VOXEL_CHUNK_SIZE);
    int32 cz = (int32) FLOORF((f32) world_z / VOXEL_CHUNK_SIZE);

    int32 lx = world_x - cx * VOXEL_CHUNK_SIZE;
    int32 ly = world_y - cy * VOXEL_CHUNK_SIZE;
    int32 lz = world_z - cz * VOXEL_CHUNK_SIZE;

    VoxelChunk* chunk = voxel_world_get_or_create_chunk(vw, cx, cy, cz);
    voxel_chunk_set(chunk, lx, ly, lz, v);
}

inline
voxel_world_create(VoxelWorld* vw) {
    hashmap_create(&vw->map, 128, sizeof(HashEntry) + sizeof(VoxelChunk), 64);

    for (int32 x = 0; x < 128; ++x) {
        for (int32 z = 0; z < 128; ++z) {
            voxel_world_voxel_set(vw, x, 0, z, {1, 0});
        }
    }

    voxel_world_remash_dirty(vw);
}

#endif
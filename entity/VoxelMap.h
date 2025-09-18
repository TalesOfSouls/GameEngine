/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ENTITY_VOXEL_MAP_H
#define COMS_ENTITY_VOXEL_MAP_H

#include "../stdlib/Types.h"
#include "../stdlib/GameMathTypes.h"
#include "../stdlib/HashMap.h"
#include "../math/matrix/MatrixFloat32.h"
#include "../math/matrix/MatrixInt32.h"

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
    bool is_filled;
};

// @todo Move to Voxel type enum
inline
bool voxel_is_solid(int32 type) {
    return type != 0;
}

#define VOXEL_CHUNK_SIZE 32

struct VoxelChunkMesh {
    // Interleaved vertex: position (float3), normal (packed int8x3),
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

    // Needs remesh
    bool is_dirty;

    // World-space AABB
    // Used to check if the chunk intersects with the view frustum planes
    AABB_int32 bounds;

    VoxelChunkMesh mesh;
}

static FORCE_INLINE
uint64 voxel_chunk_coord_pack(int32 x, int32 y, int32 z) NO_EXCEPT {
    // We check if only the lowest 21 bits are set
    ASSERT_STRICT((((uint32) (x)) & ~((1u << 21) - 1u)) == 0);
    ASSERT_STRICT((((uint32) (y)) & ~((1u << 21) - 1u)) == 0);
    ASSERT_STRICT((((uint32) (z)) & ~((1u << 21) - 1u)) == 0);

    // 21 bits per axis signed (fits +/- 1,048,575)
    uint64 ux = ((uint64) (uint32) x) & 0x1FFFFF;
    uint64 uy = ((uint64) (uint32) y) & 0x1FFFFF;
    uint64 uz = ((uint64) (uint32) z) & 0x1FFFFF;

    return (ux) | (uy << 21) | (uz << 42);
}

FORCE_INLINE
HashEntry* hashmap_insert(HashMap* hm, int32 x, int32 y, int32 z, VoxelChunk* chunk) NO_EXCEPT {
    uint64 key = voxel_chunk_coord_pack(x, y, z);
    return hashmap_insert(hm, key, (void *) chunk);
}

FORCE_INLINE
void hashmap_remove(HashMap* hm, int32 x, int32 y, int32 z) NO_EXCEPT {
    uint64 key = voxel_chunk_coord_pack(x, y, z);
    hashmap_remove(hm, key);
}

FORCE_INLINE
HashEntry* hashmap_get_entry(HashMap* hm, int32 x, int32 y, int32 z) NO_EXCEPT {
    uint64 key = voxel_chunk_coord_pack(x, y, z);
    return hashmap_get_entry(hm, key);
}

FORCE_INLINE
VoxelChunk* hashmap_get_value(HashMap* hm, int32 x, int32 y, int32 z) NO_EXCEPT {
    HashEntryVoidP* entry = (HashEntryVoidP *) hashmap_get_entry(hm, x, y, z);

    return entry ? (VoxelChunk *) entry->value : NULL;
}

FORCE_INLINE
void voxel_chunk_create(VoxelChunk* chunk, int32 x, int32 y, int32 z) {
    chunk->bounds.min = {x * VOXEL_CHUNK_SIZE, y * VOXEL_CHUNK_SIZE, z * VOXEL_CHUNK_SIZE};
    chunk->bounds.max = {(x + 1) * VOXEL_CHUNK_SIZE, (y + 1) * VOXEL_CHUNK_SIZE, (z + 1) * VOXEL_CHUNK_SIZE};
    chunk->cap_vertices = VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE;
    chunk->cap_indices = VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * 2;
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
    chunk->is_dirty = true;
}

inline
void voxel_chunk_vertex_push(
    VoxelChunk* chunk,
    v3_f32 coord,
    v3_byte normal,
    const VoxelFace* face
) {
    // We currently don't support growing chunks
    ASSERT_TRUE(chunk->mesh.num_vertices <= VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * 3);

    uint32 i = chunk->mesh.num_vertices++;

    chunk->mesh.vertices[i] = coord;

    c->mesh.normals[i].x = normal.x * 127 + 127;
    c->mesh.normals[i].y = normal.y * 127 + 127;
    c->mesh.normals[i].z = normal.z * 127 + 127;

    c->mesh.types[i] = face->type;
    c->mesh.rotations[i] = face->rotation;
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
    v3_int32 chunk_coord,
    int32 x, int32 y, int32 z,
) {
    while(x < 0) { x += VOXEL_CHUNK_SIZE; --chunk_coord.x; }
    while(y < 0) { y += VOXEL_CHUNK_SIZE; --chunk_coord.y; }
    while(z < 0) { z += VOXEL_CHUNK_SIZE; --chunk_coord.z; }
    while(x >= VOXEL_CHUNK_SIZE) { x -= VOXEL_CHUNK_SIZE; ++chunk_coord.x; }
    while(y >= VOXEL_CHUNK_SIZE) { y -= VOXEL_CHUNK_SIZE; ++chunk_coord.y; }
    while(z >= VOXEL_CHUNK_SIZE) { z -= VOXEL_CHUNK_SIZE; ++chunk_coord.z; }

    HashEntry* entry = hashmap_get_entry(map, chunk_coord.x, chunk_coord.y, chunk_coord.z);
    if (!entry) {
        // Is air
        return {0, 0};
    }

    VoxelChunk* chunk = (VoxelChunk *) entry->value;

    return voxel_chunk_get(chunk, x, y, z);
}

// Builds the greedy mash of a chunk
// Requires the neighboring chunks
void voxel_chunk_mesh_build(HashMap* map, VoxelChunk* chunk)
{
    chunk->mesh.num_vertices = 0;
    chunk->mesh.num_indices = 0;

    // @bug This is probably using up all our stack memory once we multithread it
    VoxelMaskCell mask[VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE];

    // Build quads between solid/air slabs
    // x,y,z are the axis 0-2
    for (int32 axis = 0; axis < 3; ++axis) {
        int32 u = (axis + 1) % 3;
        int32 v = (axis + 2) % 3;

        v3_int32 du = {}; du.vec[axis] = 1;
        v3_int32 dv = {}; dv.vec[v] = 1;
        v3_int32 duu = {}; duu.vec[u] = 1;

        for (int32 d = -1; d < VOXEL_CHUNK_SIZE; ++d) {
            v3_int32 offset = {
                axis == 0 ? d : 0,
                axis == 1 ? d : 0,
                axis == 2 ? d : 0
            };

            // Fill mask with faces between slices d and d + 1 along the axis
            for (int32 j = 0; j < VOXEL_CHUNK_SIZE; ++j) {
                v3_int32 dv_offset;
                vec3_muladd(&dv_offset, &offset, &dv, j);

                for (int32 i = 0; i < VOXEL_CHUNK_SIZE; ++i) {
                    v3_int32 vec;
                    vec3_muladd(&vec, &dv_offset, &duu, i);

                    Voxel a = (d >= 0)
                        ? voxel_world_map_get(map, chunk->coord, vec.x, vec.y, vec.z)
                        : {0, 0};

                    vec3_add(vec, du);
                    Voxel b = (d < VOXEL_CHUNK_SIZE - 1)
                        ? voxel_world_map_get(map, chunk->coord, vec.x, vec.y, vec.z)
                        : {0, 0};

                    VoxelMaskCell* mask_temp = &mask[j * VOXEL_CHUNK_SIZE + i];

                    // If one voxel is air and the other is solid we need to create a face
                    mask_temp->is_filled = voxel_is_solid(a.type) != voxel_is_solid(b.type)
                    if (mask_temp->is_filled) {
                        // Face is oriented towards a solid voxel
                        bool front = voxel_is_solid(b.type);
                        mask_temp->face.type = front ? b.type : a.type;
                        mask_temp->face.rotation = front ? b.rotation : a.rotation;
                    }
                }
            }

            // Greedy merge rectangles in the mask
            for (int32 j = 0; j < VOXEL_CHUNK_SIZE;) {
                for (int32 i = 0; i < VOXEL_CHUNK_SIZE;) {
                    if (!mask[j * VOXEL_CHUNK_SIZE + i].is_filled) {
                        ++i;
                        continue;
                    }

                    VoxelFace* face = &mask[j * VOXEL_CHUNK_SIZE + i].face;

                    // Calculate width
                    int32 width = 1;
                    while (i + width < VOXEL_CHUNK_SIZE
                        && mask[j * VOXEL_CHUNK_SIZE + i + width].is_filled
                        && memcmp(&mask[j * VOXEL_CHUNK_SIZE + i + width].face, face, sizeof(VoxelFace)) == 0
                    ) {
                        ++width;
                    }

                    // Calculate height
                    int32 height = 1;
                    bool done = false;
                    while (j + h < VOXEL_CHUNK_SIZE && !done) {
                        for (int32 k = 0; k < width; ++k) {
                            VoxelMaskCell* mask_temp = &mask[(j + width) * VOXEL_CHUNK_SIZE + i + k];
                            if (!(mask_temp->filled
                                && memcmp(&mask_temp->face, face, sizeof(VoxelFace)) == 0)
                            ) {
                                done = true;
                                break;
                            }
                        }

                        // @performance Consider to replace with jump in if above
                        if (done) {
                            break;
                        }

                        ++height;
                    }

                    // Create Quad of size width + height
                    // The origin is in voxel space at i,j between slabs d and d+1
                    // Build the oriented vertices
                    v3_int32 normal = {};
                    if (axis == 0) {
                        if (d >= 0
                            && d < VOXEL_CHUNK_SIZE - 1
                            && voxel_is_solid(voxel_world_map_get(map, chunk->coord, d + 1, j, i))
                        ) {
                            normal.x = -1;
                        } else if (voxel_is_solid(voxel_world_map_get(map, chunk->coord, d, j, i)) || d == -1) {
                            normal.x = 1;
                        } else {
                            normal.x = -1;
                        }
                    } else if (axis == 1) {
                        if (d >= 0
                            && d < VOXEL_CHUNK_SIZE - 1
                            && voxel_is_solid(voxel_world_map_get(map, chunk->coord, i, d + 1, j))
                        ) {
                            normal.y = -1;
                        } else if (voxel_is_solid(voxel_world_map_get(map, chunk->coord, i, d, j)) || d == -1) {
                            normal.y = 1;
                        } else {
                            normal.y = -1;
                        }
                    } else if (axis == 2) {
                        if (d >= 0
                            && d < VOXEL_CHUNK_SIZE - 1
                            && voxel_is_solid(voxel_world_map_get(map, chunk->coord, j, i, d + 1))
                        ) {
                            normal.z = -1;
                        } else if (voxel_is_solid(voxel_world_map_get(map, chunk->coord, j, i, d)) || d == -1) {
                            normal.z = 1;
                        } else {
                            normal.z = -1;
                        }
                    }

                    v3_f32 base = {
                        (f32) chunk->coord.x * VOXEL_CHUNK_SIZE,
                        (f32) chunk->coord.y * VOXEL_CHUNK_SIZE,
                        (f32) chunk->coord.z * VOXEL_CHUNK_SIZE
                    };
                    v3_f32 x = {};
                    v3_f32 y = {};
                    v3_f32 z = {};

                    x[axis] = (f32) ((d + 1) * (normal.x > 0) + d * (normal.x <= 0));

                    y[u] = (f32) i;
                    y[v] = (f32) j;

                    z[u] = (f32) width;

                    v3_f32 world = {};
                    world[v] = (f32) height;

                    uint32 vbase = chunk->mesh.num_vertices;

                    v3_f32 base_temp = base;
                    vec3_add(base_temp, x);
                    vec3_add(base_temp, y);
                    v3_f32 base_temp2 = base_temp;

                    voxel_chunk_vertex_push(chunk, base_temp, normal, face);

                    vec3_add(base_temp, z);
                    voxel_chunk_vertex_push(chunk, base_temp, normal, face);

                    vec3_add(base_temp, world);
                    voxel_chunk_vertex_push(chunk, base_temp, normal, face);

                    vec3_add(base_temp2, world);
                    voxel_chunk_vertex_push(chunk, base_temp2, normal, face);

                    if (vec3_sum(&normal) >= 0) {
                        voxel_chunk_quad_push(chunk, vbase, vbase + 1, vbase + 2, vbase + 3);
                    } else {
                        voxel_chunk_quad_push(chunk, vbase, vbase + 3, vbase + 2, vbase + 1);
                    }

                    for (int32 jj = 0; jj < height; ++jj) {
                        for (int32 ii = 0; ii < width; ++ii) {
                            mask[(j + jj) * VOXEL_CHUNK_SIZE + i + ii].is_filled = false;
                        }
                    }

                    i += width;
                }

                int32 nextj = j + 1;
                for (; nextj < VOXEL_CHUNK_SIZE; ++nextj) {
                    bool any = false;

                    for (int32 ii = 0; ii < VOXEL_CHUNK_SIZE; ++ii) {
                        if (mask[nextj * VOXEL_CHUNK_SIZE + ii].is_filled) {
                            any = true;
                            break;
                        }
                    }

                    // @performance Consider to replace with jump in if above
                    if (any) {
                        break;
                    }
                }

                j = nextj;
            }
        }
    }

    chunk->is_dirty = false;
}

struct VoxelDrawChunk {
    VoxelChunk* chunk;

    // Distance squared to the camera
    // Used for sorting
    f32 dist2;
};

struct VoxelChunkDrawQueue {
    uint32 size;
    uint32 count;
    VoxelDrawChunk* elements;
};

struct VoxelOctNode {
    AABB_int32 box; // loose bounds
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

static inline
voxel_octnode_create(VoxelOctNode* node, const AABB_int32* box){
    node->box = *box;

    for(int32 i = 0; i < 8; ++i) {
        node->child[i] = NULL;
    }

    node->chunks=NULL;
    node->count=0;
    node->cap=0;
}

static
void voxel_octnode_insert(VoxelOctNode* node, const VoxelChunk* chunk, int32 depth, int32 max_depth) {
    if(depth == max_depth){
        if(node->count == node->cap){
            node->cap = node->cap
                ? node->cap * 2
                : 4;

            node->chunks=(VoxelChunk**)realloc(node->chunks, sizeof(VoxelChunk *) * node->cap);
        }

        node->chunks[node->count++] = chunk;

        return;
    }

    // subdivide bounds
    v3_int32 cmin = node->box.min
    v3_int32 cmax = node->box.max;
    v3_int32 half = vec3_scale(vec3_sub(cmax,cmin), 0.5f);
    v3_int32 ctr = vec3_add(cmin, half);

    AABB_int32 child_box[8];
    for (int32 i = 0; i < 8; ++i) {
        v3_int32 mn = {
            (i & 1) ? ctr.x : cmin.x,
            (i & 2) ? ctr.y : cmin.y,
            (i & 4) ? ctr.z : cmin.z
        };

        v3_int32 mx = {
            (i & 1) ? cmax.x: ctr.x ,
            (i & 2) ? cmax.y: ctr.y ,
            (i & 4) ? cmax.z: ctr.z
        };

        child_box[i] = (AABB_int32){ mn, mx };
    }

    // attempt to place chunk in a single child; if overlaps multiple, store here
    int32 single = -1;
    for (int32 i = 0; i < 8; ++i) {
        if (aabb_overlap(child_box[i], c->bounds)){
            if (single == -1) {
                single = i;
            } else {
                single = -2;
                break;
            }
        }
    }

    if (single >= 0) {
        if(!node->child[single]) {
            voxel_octnode_create(&node->child[single], child_box[single]);
        }

        voxel_octnode_insert(node->child[single], chunk, depth+1, max_depth);

        return;
    }

    // overlap multiple children -> keep at this node
    if (node->count==node->cap) {
        node->cap = node->cap
            ? node->cap * 2
            : 4;

        node->chunks=(VoxelChunk**)realloc(node->chunks, sizeof(VoxelChunk *) * node->cap);
    }

    node->chunks[node->count++] = (VoxelChunk*) chunk;
}

static
void voxel_octree_rebuild(VoxelOctree* tree, const ChunkMap* map)
{
    // destroy and rebuild from chunk map (simple approach; can be made incremental)
    // @todo instead of free set to 0
    voxel_octnode_free(t->root);

    // compute bounds from map contents, fallback to origin cube
    AABB_int32 wb = { {-512,-512,-512}, {512,512,512} };
    tree->root = voxel_octnode_create(node, aabb_loosen(wb, tree->looseness));

    // iterate map slots
    // @bug this won't work for our map implementation
    for(uint32 i = 0; i < map->cap; ++i) {
        Chunk* chunk = map->vals[i];

        if(!chunk) {
            continue;
        }

        voxel_octnode_insert(tree->root, chunk, 0, tree->max_depth);
    }
}

struct VoxelWorld {
    // This contains the actual chunk data
    // The element size is not a full chunk but sizeof(VoxelChunk) + length*length*sizeof(Voxel)
    DataPool mem;

    // This contains the pointer to the VoxelChunk stored in the chunk memory (HashEntryVoidPKeyInt64)
    // We don't directly store the chunks in here because then we would have to support dynamic size hashmap entries
    // This wouldn't be impossible but make the memory handling more complex
    // @todo we need to implement a defragment function that allows us to defragment DataPool
    //  -> optimize free space because highly fragmented data will make new allocation difficult
    //  -> this needs to be implemented here because the HashMap also needs to update it's reference
    //  -> For that we need to iterate every element in the hashmap and try to find a better position and update the value
    //  -> However, this might not be needed at all depending on how much overhead memory we allow for DataPool
    HashMap map;

    // Octree used for the rendering
    VoxelOctree oct;

    bool use_octree;
};

static inline
void voxel_world_remesh_dirty(VoxelWorld* vw) NO_EXCEPT {
    for(uint32 i = 0; i < vw->map.buf.count; ++i) {
        VoxelChunk* chunk = w->map.vals[i];
        if(!chunk) {
            continue
        };

        if(chunk->is_dirty) {
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

    voxel_world_remesh_dirty(vw);
}

#endif
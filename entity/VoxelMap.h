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
#include "../memory/Queue.h"
#include "../math/matrix/MatrixFloat32.h"
#include "../math/matrix/MatrixInt32.h"

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
HashEntry* hashmap_insert(HashMap* __restrict hm, int32 x, int32 y, int32 z, VoxelChunk* __restrict chunk) NO_EXCEPT {
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
Chunk voxel_chunk_create(int32 x, int32 y, int32 z) NO_EXCEPT {
    Chunk chunk = {};
    chunk.bounds.min = {x * VOXEL_CHUNK_SIZE, y * VOXEL_CHUNK_SIZE, z * VOXEL_CHUNK_SIZE};
    chunk.bounds.max = {(x + 1) * VOXEL_CHUNK_SIZE, (y + 1) * VOXEL_CHUNK_SIZE, (z + 1) * VOXEL_CHUNK_SIZE};
    chunk.cap_vertices = VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE;
    chunk.cap_indices = VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE * 2;

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

// Gets a voxel based on global/world map coordinates
inline
Voxel voxel_world_map_get(
    HashMap* map,
    v3_int32 chunk_coord,
    int32 x, int32 y, int32 z,
) NO_EXCEPT {
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
void voxel_chunk_mesh_build(HashMap* map, VoxelChunk* chunk) NO_EXCEPT
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

                    if (vec3_sum(normal) >= 0) {
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

    chunk->is_dirty &= ~VOXEL_CHUNK_FLAG_IS_CHANGED;
}

struct VoxelDrawChunk {
    VoxelChunk* chunk;

    // Distance squared to the camera
    // Used for sorting
    f32 dist2;
};

struct VoxelChunkDrawArray {
    // Maximum size
    uint32 size;

    // Currently inserted elements
    uint32 count;
    VoxelDrawChunk* elements;
};

static inline
void voxel_octnode_collect_visible(const VoxelOctNode* n, const Frustum* f, VoxelChunkDrawArray* q) {
    if (!n) {
        return;
    }

    if (!aabb_intersects_frustum(&n->box, f)) {
        return;
    }

    if (n->is_leaf) {
        if (n->chunk) {
            q->elements[q->count++] = n->chunk;
        }

        return;
    }

    for (int32 i = 0; i < 8; ++i) {
        voxel_octnode_collect_visible(n->child[i], f, q);
    }
}

static inline
int32 sort_draw_array_compare(const void* a, const void* b) NO_EXCEPT {
    const VoxelDrawChunk* c1 = (VoxelDrawChunk *) a;
    const VoxelDrawChunk* c2 = (VoxelDrawChunk *) b;

    return (c1->dist2 < c2->dist2) ? -1 : (int32) (c1->dist2 > c2->dist2);
}

static inline
void draw_array_sort(VoxelChunkDrawArray* draw_array) NO_EXCEPT {
    qsort(draw_array->elements, draw_array->count, sizeof(VoxelDrawChunk), sort_draw_array_compare);
}

void vw_build_draw_queue(VoxelWorld* vw, const Camera* camera) {
    frustum f = frustum_from_vp(viewproj);
    DrawQueue q={0};
    if(w->use_octree){
    octnode_collect_visible(w->oct.root, &f, &q, cam_pos);
    } else {
    for(uint32_t i=0;i<w->map.cap;++i){ Chunk* c=w->map.vals[i]; if(!c) continue; if(aabb_intersects_frustum(&c->bounds,&f)) dq_push(&q,c,cam_pos); }
    }
    dq_sort(&q);
    return q; // caller owns q.items (free with free())
}

struct VoxelOctNode {
    AABB_int32 box; // loose bounds
    bool is_leaf;
    bool has_data;
    VoxelOctNode* child[8]; // children or NULL
    //VoxelOctNode* parent;
    const VoxelChunk* chunk;
};

struct VoxelOctree {
    VoxelOctNode* root;

    // Last added node
    VoxelOctNode* last;
    int32 max_depth;
    //f32 looseness; // >1.0 (e.g., 1.5) used to handle objects overlapping a chunk boundary
};

static inline
VoxelOctNode* voxel_octnode_child_create(VoxelOctree* tree)
{
    VoxelOctNode* node = ++tree->last;
    memset(node, 0, sizeof(VoxelOctNode));

    node->has_data = true;

    return node;
}

static inline
int32 voxel_octree_child_index_for_chunk(const OctNode* node, const VoxelChunk* chunk) {
    v3_int32 center = aabb_center(&node->bounds);

    // We'll decide which side of the center the chunk lies on in each axis
    int32 index = 0;
    if (chunk->x >= center.x) {
        index |= COORD_AXIS_X;
    }

    if (chunk->y >= center.y) {
        index |= COORD_AXIS_Y;
    }

    if (chunk->z >= center.z) {
        index |= COORD_AXIS_Z;
    }

    return index;
}

static inline
void voxel_octnode_child_aabb_compute(AABB_int32* out, const AABB_int32* parent, int32 child_index)
{
    v3_int32 center = aabb_center(parent);
    *out = *parent;

    // Split along X
    if (child_index & COORD_AXIS_X) {
        out->min.x = center.x;
    } else {
        out->max.x = center.x;
    }

    // Split along Y
    if (child_index & COORD_AXIS_Y) {
        out->min.y = center.y;
    } else {
        out->max.y = center.y;
    }

    // Split along Z
    if (child_index & COORD_AXIS_Z) {
        out->min.z = center.z;
    } else {
        out->max.z = center.z;
    }
}

static inline
void voxel_octnode_insert(VoxelOctree* tree, const VoxelChunk* chunk) {
    VoxelOctNode* node = tree->root;
    while (!node->is_leaf) {
        node->has_data = true;

        int32 child_index = voxel_octree_child_index_for_chunk(node, chunk);
        if (node->child[child_index] == NULL) {
            node->child[child_index] = voxel_octnode_child_create(node, child_index);
            voxel_octnode_child_aabb_compute(&node->child[child_index]->box, node->box, child_index);

            // Check if this is a leaf
            // Leafs have a aabb box width of VOXEL_CHUNK_SIZE in our specific implementation
            node->child[child_index]->is_leaf = (node->child[child_index]->box.max.x - node->child[child_index]->box.min.x) <= VOXEL_CHUNK_SIZE;
        }

        node = node->child[child_index];
    }

    node->chunk = chunk;
    node->is_leaf = true;
    node->has_data = true;
}

static inline
void voxel_octree_create(VoxelOctree* tree, v3_int32 pos)
{
    // 8^n == 2^(3*n)
    // @performance Do we really need that many chunks? That is the maximum amount of chunks but most of them will be empty
    //          Which also means very often we will not go the full depth
    //          This is potentially a huge amount of unused reserved nodes
    //          On the other hand the nodes are very small and the actual data is in the chunks
    int32 chunk_count = OMS_POW2(3 * (tree->max_depth - 1));
    //memset(tree->root, 0, sizeof(VoxelOctNode) * chunk_count);

    VoxelOctNode* node = tree->root;
    node->is_leaf = tree->max_depth == 1;

    // Center of the chunk the player is currently standing on
    // calculate amount of chunks towards x, y, z (rounded down)
    // then walk along those axis
    // then add another halve chunk size to go to the center
    //      if negative we need to subtract because int rounding rounds up if negative
    v3_int32 ctr = {
        IFLOORI_POS_DIV_32(pos.x, VOXEL_CHUNK_SIZE) * VOXEL_CHUNK_SIZE + OMS_DIV2_I32(VOXEL_CHUNK_SIZE),
        IFLOORI_POS_DIV_32(pos.y, VOXEL_CHUNK_SIZE) * VOXEL_CHUNK_SIZE + OMS_DIV2_I32(VOXEL_CHUNK_SIZE),
        IFLOORI_POS_DIV_32(pos.z, VOXEL_CHUNK_SIZE) * VOXEL_CHUNK_SIZE + OMS_DIV2_I32(VOXEL_CHUNK_SIZE)
    };

    const int32 half_size = OMS_DIV2_I32(VOXEL_CHUNK_SIZE) * chunk_count;

    node->box = {
        { ctr.x - half_size, ctr.y - half_size, ctr.z - half_size },
        { ctr.x + half_size, ctr.y + half_size, ctr.z + half_size },
    };
}

struct VoxelWorld {
    // This contains the actual chunk data
    // The element size is not a full chunk but sizeof(VoxelChunk) + length*length*sizeof(Voxel)
    // @todo we need to implement a defragment function that allows us to defragment DataPool
    //  -> optimize free space because highly fragmented data will make new allocation difficult
    //  -> this needs to be implemented here because the HashMap also needs to update it's reference
    //  -> For that we need to iterate every element in the hashmap and try to find a better position and update the value
    //  -> However, this might not be needed at all depending on how much overhead memory we allow for DataPool
    DataPool chunks;

    // This contains the pointer to the VoxelChunk stored in the chunk memory (HashEntryVoidPKeyInt64)
    // We don't directly store the chunks in here because then we would have to support dynamic size hashmap entries
    // This wouldn't be impossible but make the memory handling more complex
    HashMap map;

    // Octree used for the rendering
    // Yes, we hardly need a full octree in the real world. Most of the time we need horizontal map data.
    // This makes it seem like we are wasting/not using vertical data as much.
    // That is true but we can simply not fill/use that much horizontal data.
    // In other words we have to pre-select which chunks we want to put in our octree
    VoxelOctree oct_old;
    VoxelOctree oct_new;

    VoxelChunkDrawArray* draw_array;
};

// Updates the voxel world after a position update
// Careful this function doesn't load new chunks after the position update
// @todo implement logic somewhere that loads the new chunks (e.g. from database, file etc)
//      this may happen in here or outside
static inline
void voxel_world_update_pos(VoxelWorld* vw, v3_int32 pos) {
    // After a position update we have to reset the octree
    voxel_octree_create(&vw->oct_new, pos);

    // Now we need to add every chunk to the octree that is within the aabb box
    // Not all chunks currently in memory may fulfill that (we remove those)
    uint32 chunk_id = 0;
    chunk_iterate_start(&vw->map.buf, chunk_id) {
        HashEntry* entry = (HashEntry *) chunk_get_element(&vw->map.buf, chunk_id);
        VoxelChunk* chunk = (VoxelChunk *) entry->value;

        // Don't add chunks to be removed or outside of our bounding box
        if (aabb_overlap(vw->oct_new.root->box, chunk->bounds)) {
            // @question Do we really want to do this? We might want to keep a chunk recently out of bounding box
            //      This way if a player returns to the previous chunk this chunk is still in memory
            chunk->flag |= VOXEL_CHUNK_FLAG_SHOULD_REMOVE;
            chunk_iterate_large_skip(chunk->element_count);
        }

        if (chunk->flag & VOXEL_CHUNK_FLAG_IS_CHANGED) {
            voxel_chunk_mesh_build(chunk, &vw->map);
            chunk->flag &= ~VOXEL_CHUNK_FLAG_IS_CHANGED;
        }

        // We don't have to check if the chunk is new because upon player position update
        // we MUST recreate the octree. This is different to voxel_world_update_state
        // --------------------------------------------------------------------------------

        // Make sure the chunk doesn't get removed
        chunk->flag &= ~VOXEL_CHUNK_FLAG_SHOULD_REMOVE;
        voxel_octnode_insert(&vw->oct_new, chunk);
    } chunk_iterate_end;
}

// Updates the voxel world after voxel changes
static inline
void voxel_world_update_state(VoxelWorld* vw) {
    // Iterate all elements in the hash map (only the active ones of course)
    uint32 chunk_id = 0;
    chunk_iterate_start(&vw->map.buf, chunk_id) {
        HashEntry* entry = (HashEntry *) chunk_get_element(&vw->map.buf, chunk_id);
        VoxelChunk* chunk = (VoxelChunk *) entry->value;

        if (chunk->flag & VOXEL_CHUNK_FLAG_SHOULD_REMOVE) {
            // Remove chunk from octree
            voxel_octnode_remove(vw, chunk);

            // Remove chunk from hash map
            // @bug This is not how you remove an element from the hash map
            chunk_free_elements(&vw->map.buf, chunk_id, chunk->element_count);

            // This continues the iteration and skips the code below
            chunk_iterate_large_skip(chunk->element_count);
        }

        if (chunk->flag & VOXEL_CHUNK_FLAG_IS_CHANGED) {
            // Rebuild mesh
            voxel_chunk_mesh_build(chunk, &vw->map);
            chunk->flag &= ~VOXEL_CHUNK_FLAG_IS_CHANGED;
        }

        if (chunk->flag & VOXEL_CHUNK_FLAG_IS_NEW) {
            // Add new chunk to octree
            voxel_octnode_insert(vw, chunk);
            chunk->flag &= ~VOXEL_CHUNK_FLAG_IS_NEW;
        }

        chunk_iterate_large_skip(chunk->element_count);
    } chunk_iterate_end;
}

// This function cleans up the chunks
inline
void voxel_world_chunk_update(VoxelWorld* vw) {
    // Consider to handle all the chunk removal and adds to the octree here instead of
    //      voxel_world_update_state
    //      and voxel_world_update_pos

    // Iterate all elements in the hash map (only the active ones of course)
    uint32 chunk_id = 0;
    chunk_iterate_start(&vw->chunks.buf, chunk_id) {
        VoxelChunk* chunk = (VoxelChunk *) chunk_get_element(&vw->chunks.buf, chunk_id);

        if (chunk->flag & VOXEL_CHUNK_FLAG_SHOULD_REMOVE) {
            // Remove chunk from octree
            voxel_octnode_remove(vw, chunk);

            // Remove chunk from hash map
            // @bug This is not how you remove an element from the hash map
            chunk_free_elements(&vw->map.buf, chunk_id, chunk->element_count);

            // This continues the iteration and skips the code below
            chunk_iterate_large_skip(chunk->element_count);
        }

        if (chunk->flag & VOXEL_CHUNK_FLAG_IS_CHANGED) {
            // Rebuild mesh
            voxel_chunk_mesh_build(chunk, &vw->map);
            chunk->flag &= ~VOXEL_CHUNK_FLAG_IS_CHANGED;
        }

        if (chunk->flag & VOXEL_CHUNK_FLAG_IS_NEW) {
            // Add new chunk to octree
            voxel_octnode_insert(vw, chunk);
            chunk->flag &= ~VOXEL_CHUNK_FLAG_IS_NEW;
        }

        chunk_iterate_large_skip(chunk->element_count);
    } chunk_iterate_end;
}

static inline
VoxelChunk* voxel_world_chunk_get_or_create(VoxelWorld* vw, int32 cx, int32 cy, int32 cz) NO_EXCEPT
{
    HashEntry* entry = hashmap_get_entry(&vw->map, cx, cy, cz);
    if(entry) {
        return (VoxelChunk *) entry->value;
    }

    // @performance Maybe use hashmap_reserve and directly fill it instead of copying it over
    VoxelChunk chunk = voxel_chunk_create(cx, cy, cz);

    entry = hashmap_insert(&vw->map, cx, cy, cz, &chunk);

    return (VoxelChunk *) entry->value;
}

static inline
void voxel_world_voxel_set(VoxelWorld* vw, int32 world_x, int32 world_y, int32 world_z, Voxel v) NO_EXCEPT
{
    int32 cx = IFLOORI_POS_DIV_32(world_x, VOXEL_CHUNK_SIZE);
    int32 cy = IFLOORI_POS_DIV_32(world_y, VOXEL_CHUNK_SIZE);
    int32 cz = IFLOORI_POS_DIV_32(world_z, VOXEL_CHUNK_SIZE);

    // Transform global coordinates to local/chunk coordinates
    int32 lx = world_x - cx * VOXEL_CHUNK_SIZE;
    int32 ly = world_y - cy * VOXEL_CHUNK_SIZE;
    int32 lz = world_z - cz * VOXEL_CHUNK_SIZE;

    VoxelChunk* chunk = voxel_world_chunk_get_or_create(vw, cx, cy, cz);
    voxel_chunk_set(chunk, lx, ly, lz, v);
}

inline
void voxel_world_init(VoxelWorld* vw, v3_int32 pos, int32 max_depth, BufferMemory* buf) {
    // 8^n == 2^(3*n)
    int32 chunk_count = OMS_POW2(3 * (max_depth - 1));

    pool_create(&vw->mem, chunk_count, buf);

    // We want a hashmap with 2* the amount of chunks for reduced hash collisions
    hashmap_create(&vw->map, chunk_count * 2, sizeof(HashEntry) + sizeof(VoxelChunk), 32);

    // Reserve max amount of node memory space
    vw->oct_old->max_depth = max_depth;
    vw->oct_old->root = (VoxelOctNode *) buffer_get_memory(buf, sizeof(VoxelOctNode) * chunk_count, 8);
    vw->oct_old->last = vw->oct_old->root;
    vw->oct_old->has_data = true;

    vw->oct_new->max_depth = max_depth;
    // @bug This is a problem for when we change the max_depth
    //  We then need to grow this buffer, which is currently not possible
    //  Sure we could allocate a new one but then we would basically waste the old one one as unused memory
    vw->oct_new->root = (VoxelOctNode *) buffer_get_memory(buf, sizeof(VoxelOctNode) * chunk_count, 8);
    vw->oct_new->last = vw->oct_new->root;
    vw->oct_new->has_data = true;

    /*
    for (int32 x = 0; x < 128; ++x) {
        for (int32 z = 0; z < 128; ++z) {
            voxel_world_voxel_set(vw, x, 0, z, {1, 0});
        }
    }
    */

    voxel_octree_create(&vw->oct_old, pos, max_depth, buf);
    voxel_octree_create(&vw->oct_new, pos, max_depth, buf);
    // This doesn't happen because we first need to load the respective chunks for the pos into the hash map
    // voxel_world_update_pos(vw, pos);

    // Actual usage:
    //      1. voxel_world_init with player spawn position
    //      2. load all voxel close to player position
    //      2. load all chunks via voxel_world_chunk_add(vw, chunk)
    //      3. voxel_world_update_state()

    // Per frame:
    //      Player moves and IFF crossed chunk?
    //          voxel_world_chunk_add(), add new chunks
    //          voxel world_chunk_remesh(), remesh newly added chunks manually
    //          voxel_world_update_pos()
    //      IFF voxel changed?
    //          voxel_world_update_state()

    // Performance ideas:
    //      1. perform remesh in another thread and just replace when done
    //      2. perform entire logic above in another thread and replace entire vw once done
    //          BEST option, we can even pass the current version to avoid loading chunks or meshes again

    // Things to consider:
    //      Maybe consider to store a larger area in memory than on gpu
    //      This way we don't have to rebuild the tree every time and wait for it but continuously do it in another thread
    //      Even if the thread is not done yet, we still have a buffer of at least N chunks
    //      We could even decide to only rebuild after N/2 chunk crossings
}

#endif
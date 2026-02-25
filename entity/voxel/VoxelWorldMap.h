/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ENTITY_VOXEL_WORLD_MAP_H
#define COMS_ENTITY_VOXEL_WORLD_MAP_H

#include "../../stdlib/Stdlib.h"
#include "../../stdlib/GameMathTypes.h"
#include "../../stdlib/HashMap.h"
#include "../../stdlib/Octree.h"
#include "../../memory/DataPool.h"

#include "Voxel.h"
#include "VoxelHashMap.h"

// Gets a voxel based on global/world map coordinates
inline
Voxel voxel_world_map_get(
    HashMap* map,
    v3_int32 chunk_coord,
    int32 x, int32 y, int32 z
) NO_EXCEPT
{
    while(x < 0) { x += VOXEL_CHUNK_SIZE; --chunk_coord.x; }
    while(y < 0) { y += VOXEL_CHUNK_SIZE; --chunk_coord.y; }
    while(z < 0) { z += VOXEL_CHUNK_SIZE; --chunk_coord.z; }
    while(x >= VOXEL_CHUNK_SIZE) { x -= VOXEL_CHUNK_SIZE; ++chunk_coord.x; }
    while(y >= VOXEL_CHUNK_SIZE) { y -= VOXEL_CHUNK_SIZE; ++chunk_coord.y; }
    while(z >= VOXEL_CHUNK_SIZE) { z -= VOXEL_CHUNK_SIZE; ++chunk_coord.z; }

    HashEntryVoidPKeyInt64* entry = voxel_hashmap_get_entry(map, chunk_coord.x, chunk_coord.y, chunk_coord.z);
    if (!entry) {
        // Is air
        return {0, 0};
    }

    VoxelChunk* chunk = (VoxelChunk *) entry->value;

    return voxel_chunk_get(chunk, x, y, z);
}

// Builds the greedy mash of a chunk
// Requires the neighboring chunks
void voxel_chunk_mesh_build(HashMap* const map, VoxelChunk* const chunk) NO_EXCEPT
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

        v3_int32 du = {0};
        du.vec[axis] = 1;

        v3_int32 dv = {0};
        dv.vec[v] = 1;

        v3_int32 duu = {0};
        duu.vec[u] = 1;

        for (int32 d = -1; d < VOXEL_CHUNK_SIZE; ++d) {
            // Fill mask with faces between slices d and d + 1 along the axis
            for (int32 j = 0; j < VOXEL_CHUNK_SIZE; ++j) {
                for (int32 i = 0; i < VOXEL_CHUNK_SIZE; ++i) {
                    v3_int32 coordA = {0};
                    v3_int32 coordB = {0};

                    // set u and v components
                    coordA.vec[u] = i;
                    coordA.vec[v] = j;
                    coordB.vec[u] = i;
                    coordB.vec[v] = j;

                    coordA.vec[axis] = d - 1;
                    coordB.vec[axis] = d;

                    Voxel a = {0,0}; // default: air
                    Voxel b = {0,0};
                    bool a_valid = (coordA.vec[axis] >= 0 && coordA.vec[axis] < VOXEL_CHUNK_SIZE);
                    bool b_valid = (coordB.vec[axis] >= 0 && coordB.vec[axis] < VOXEL_CHUNK_SIZE);

                    if (a_valid) {
                        a = voxel_world_map_get(map, chunk->coord, coordA.x, coordA.y, coordA.z);
                    }
                    if (b_valid) {
                        b = voxel_world_map_get(map, chunk->coord, coordB.x, coordB.y, coordB.z);
                    }

                    bool solidA = voxel_is_solid(a.type);
                    bool solidB = voxel_is_solid(b.type);

                    VoxelMaskCell* m = &mask[j * VOXEL_CHUNK_SIZE + i];
                    m->is_filled = (solidA != solidB);
                    if (m->is_filled) {
                        // face should point toward the solid voxel (front == true => face oriented toward B)
                        bool front = solidB;
                        m->face.type = front ? b.type : a.type;
                        m->face.rotation = front ? b.rotation : a.rotation;
                    } else {
                        // Clear face contents to avoid accidental memcmp matches
                        // @performance we can maybe optimize the size of face through padding and then do:
                        // MEMSET_ZERO_32(&m->face)
                        memset(&m->face, 0, sizeof(m->face));
                    }
                }
            }

            // Greedy merge rectangles in the mask
            for (int32 j = 0; j < VOXEL_CHUNK_SIZE;) {
                for (int32 i = 0; i < VOXEL_CHUNK_SIZE;) {
                    VoxelMaskCell* cell = &mask[j * VOXEL_CHUNK_SIZE + i];
                    if (!cell->is_filled) {
                        ++i;
                        continue;
                    }

                    VoxelFace face = cell->face; // copy for comparison

                    // compute width
                    int32 width = 1;
                    while (i + width < VOXEL_CHUNK_SIZE) {
                        VoxelMaskCell* next = &mask[j * VOXEL_CHUNK_SIZE + (i + width)];
                        if (!next->is_filled) break;
                        if (memcmp(&next->face, &face, sizeof(VoxelFace)) != 0) break;
                        ++width;
                    }

                    // compute height
                    int32 height = 1;
                    bool height_done = false;
                    while (j + height < VOXEL_CHUNK_SIZE && !height_done) {
                        for (int32 k = 0; k < width; ++k) {
                            VoxelMaskCell* check = &mask[(j + height) * VOXEL_CHUNK_SIZE + (i + k)];
                            if (!check->is_filled || memcmp(&check->face, &face, sizeof(VoxelFace)) != 0) {
                                height_done = true;
                                break;
                            }
                        }
                        if (!height_done) ++height;
                    }

                    // Build a quad covering [i..i+width-1] x [j..j+height-1] at boundary d
                    // Determine face normal: axis direction * (+1 or -1).
                    // If B (d) is solid -> normal points +axis. If A (d-1) is solid -> normal points -axis.
                    v3_byte normal_int = {0};

                    bool front = false;
                    if (d >= 0) {
                        v3_int32 coordB = {0};
                        coordB.vec[u] = i;
                        coordB.vec[v] = j;
                        coordB.vec[axis] = d;

                        Voxel b = voxel_world_map_get(map, chunk->coord, coordB.x, coordB.y, coordB.z);
                        front = voxel_is_solid(b.type);
                    }

                    byte normal_sign = front ? 1 : -1;
                    normal_int.vec[axis] = normal_sign;

                    // world base (chunk origin in world coordinates)
                    // @question why do I even need this variable? it is only used once
                    const v3_f32 base = {
                        (f32) chunk->coord.x * (f32)VOXEL_CHUNK_SIZE,
                        (f32) chunk->coord.y * (f32)VOXEL_CHUNK_SIZE,
                        (f32) chunk->coord.z * (f32)VOXEL_CHUNK_SIZE
                    };

                    // compute the 3 axes vectors for quad positioning in f32:
                    v3_f32 axisVec = {0.0f, 0.0f, 0.0f};       // offset along axis to the plane
                    v3_f32 uVec = {0.0f, 0.0f, 0.0f};          // across width (u direction)
                    v3_f32 vVec = {0.0f, 0.0f, 0.0f};          // across height (v direction)

                    // axisVec: position of the face along axis direction:
                    // if normal points +axis, face lies at d (the B side), else at d-1 (A side)
                    f32 planePos = (f32)( (normal_sign > 0) ? d : (d - 1) );
                    axisVec.vec[axis] = planePos;

                    // uVec is along u (width), vVec along v (height)
                    uVec.vec[u] = (f32) width;
                    vVec.vec[v] = (f32) height;

                    // origin point for quad (lower-left corner in chunk-local coordinates):
                    // for coordinate mapping: we want the vertex at (u = i, v = j)
                    v3_f32 origin = base;
                    origin.vec[u] += (f32) i;
                    origin.vec[v] += (f32) j;
                    // origin already accounts for chunk base; add axis position
                    origin.vec[axis] += axisVec.vec[axis];

                    // push quad vertices (4 verts). We'll follow a consistent winding based on normal
                    uint32 vbase = chunk->mesh.num_vertices;

                    // vertex 0: origin
                    voxel_chunk_vertex_push(chunk, origin, normal_int, &face);

                    // vertex 1: origin + uVec (width direction)
                    v3_f32 v1 = origin;
                    v1.vec[u] += uVec.vec[u];
                    voxel_chunk_vertex_push(chunk, v1, normal_int, &face);

                    // vertex 2: origin + uVec + vVec (width + height)
                    v3_f32 v2 = v1;
                    v2.vec[v] += vVec.vec[v];
                    voxel_chunk_vertex_push(chunk, v2, normal_int, &face);

                    // vertex 3: origin + vVec
                    v3_f32 v3 = origin;
                    v3.vec[v] += vVec.vec[v];
                    voxel_chunk_vertex_push(chunk, v3, normal_int, &face);

                    // Create indices with correct winding: if normal points towards positive axis, winding is 0,1,2,3 otherwise flip
                    if (normal_sign > 0) {
                        voxel_chunk_quad_push(chunk, vbase + 0, vbase + 1, vbase + 2, vbase + 3);
                    } else {
                        voxel_chunk_quad_push(chunk, vbase + 0, vbase + 3, vbase + 2, vbase + 1);
                    }

                    // clear mask cells covered by this quad
                    for (int32 jj = 0; jj < height; ++jj) {
                        for (int32 ii = 0; ii < width; ++ii) {
                            mask[(j + jj) * VOXEL_CHUNK_SIZE + (i + ii)].is_filled = false;
                        }
                    }

                    // advance i by width
                    i += width;
                }

                // Find next j that has any filled cell (skip empty rows)
                int32 nextj = j + 1;
                for (; nextj < VOXEL_CHUNK_SIZE; ++nextj) {
                    bool any = false;
                    for (int32 ii = 0; ii < VOXEL_CHUNK_SIZE; ++ii) {
                        if (mask[nextj * VOXEL_CHUNK_SIZE + ii].is_filled) {
                            any = true;
                            break;
                        }
                    }
                    if (any) break;
                }

                j = nextj;
            }
        }
    }

    chunk->flag &= ~VOXEL_CHUNK_FLAG_IS_CHANGED;
}

struct VoxelDrawChunk {
    const void* data;

    // Distance squared to the camera
    // Used for sorting
    f32 dist2;
};

struct VoxelChunkDrawArray {
    // Maximum size
    int32 size;

    // Currently inserted elements
    int32 count;
    VoxelDrawChunk* elements;
};

struct VoxelWorld {
    BufferMemory mem;

    // This contains the actual chunk data
    // The element size is not a full chunk but sizeof(VoxelChunk) + length*length*sizeof(Voxel)
    // @todo we need to implement a defragment function that allows us to defragment DataPool
    //  -> optimize free space because highly fragmented data will make new allocation difficult
    //  -> this needs to be implemented here because the HashMap also needs to update it's reference
    //  -> For that we need to iterate every element in the hashmap and try to find a better position and update the value
    //  -> However, this might not be needed at all depending on how much overhead memory we allow for DataPool
    // @question Consider to use the ReserveMemory instead of DataPool?
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
    Octree oct_old;
    Octree oct_new;

    VoxelChunkDrawArray draw_array;
};

static inline
void voxel_octnode_collect_visible(const OctNode* node, const Frustum* frustum, VoxelChunkDrawArray* draw_array) NO_EXCEPT
{
    if (!aabb_intersects_frustum(&node->bounds, frustum)) {
        return;
    }

    if (node->is_leaf) {
        if (node->data) {
            draw_array->elements[draw_array->count++].data = node->data;
        }

        return;
    }

    for (int i = 0; i < ARRAY_COUNT(node->child); ++i) {
        if (node->child[i]) {
            voxel_octnode_collect_visible(node->child[i], frustum, draw_array);
        }
    }
}

static inline
int32 voxel_sort_draw_array_compare(const void* a, const void* b) NO_EXCEPT
{
    const VoxelDrawChunk* c1 = (VoxelDrawChunk *) a;
    const VoxelDrawChunk* c2 = (VoxelDrawChunk *) b;

    return (c1->dist2 < c2->dist2) ? -1 : (int32) (c1->dist2 > c2->dist2);
}

inline
void voxel_draw_array_build(VoxelWorld* const vw, const Camera* const camera) NO_EXCEPT
{
    vw->draw_array.count = 0;

    voxel_octnode_collect_visible(vw->oct_old.root, &camera->frustum, &vw->draw_array);

    // Sort the draw array
    // @performance Compare to sort_quicksort
    sort_introsort_small(vw->draw_array.elements, vw->draw_array.count, sizeof(VoxelDrawChunk), voxel_sort_draw_array_compare);
}

// Updates the voxel world after a position update
// Careful this function doesn't load new chunks after the position update
// @todo implement logic somewhere that loads the new chunks (e.g. from database, file etc)
//      this may happen in here or outside
static inline
void voxel_world_update_pos(VoxelWorld* const vw, v3_int32 pos) NO_EXCEPT
{
    // After a position update we have to reset the octree
    octree_create(&vw->oct_new, pos, vw->chunks.capacity * VOXEL_CHUNK_SIZE);

    // Now we need to add every chunk to the octree that is within the aabb box
    // Not all chunks currently in memory may fulfill that (we remove those)
    uint32 chunk_id = 0;
    chunk_iterate_start(&vw->map.buf, chunk_id) {
        HashEntry* entry = (HashEntry *) chunk_get_element(&vw->map.buf, chunk_id);
        VoxelChunk* chunk = (VoxelChunk *) entry->value;

        // Don't add chunks to be removed or outside of our bounding box
        if (aabb_overlap(vw->oct_new.root->bounds, chunk->bounds)) {
            // @question Do we really want to do this? We might want to keep a chunk recently out of bounding box
            //      This way if a player returns to the previous chunk this chunk is still in memory
            chunk->flag |= VOXEL_CHUNK_FLAG_SHOULD_REMOVE;
            chunk_iterate_continue_n(chunk->element_count);
        }

        if (chunk->flag & VOXEL_CHUNK_FLAG_IS_CHANGED) {
            voxel_chunk_mesh_build(&vw->map, chunk);
            chunk->flag &= ~VOXEL_CHUNK_FLAG_IS_CHANGED;
        }

        // We don't have to check if the chunk is new because upon player position update
        // we MUST recreate the octree. This is different to voxel_world_chunk_update
        // --------------------------------------------------------------------------------

        // Make sure the chunk doesn't get removed
        chunk->flag &= ~VOXEL_CHUNK_FLAG_SHOULD_REMOVE;
        octnode_insert(&vw->oct_new, chunk, chunk->coord);
    } chunk_iterate_end;
}

// Updates the voxel world after voxel changes
// This function cleans up the chunks
inline
void voxel_world_chunk_update(VoxelWorld* vw) NO_EXCEPT
{
    // Consider to handle all the chunk removal and adds to the octree here instead of
    //      voxel_world_chunk_update
    //      and voxel_world_update_pos

    // Iterate all elements in the hash map (only the active ones of course)
    // @bug do We need a lock? we are not locking the chunk when doing this,
    // if we have a thread that defragments the datapool we are screwed
    uint32 chunk_id = 0;
    chunk_iterate_start(&vw->chunks, chunk_id) {
        VoxelChunk* chunk = (VoxelChunk *) chunk_get_element((ChunkMemory *) &vw->chunks, chunk_id);

        if ((chunk->flag & VOXEL_CHUNK_FLAG_SHOULD_REMOVE)
            || (chunk->flag & VOXEL_CHUNK_FLAG_IS_INACTIVE)
        ) {
            // Remove chunk from octree
            octnode_remove(&vw->oct_new, chunk->coord);

            // Remove chunk from hash map
            voxel_hashmap_remove(&vw->map, chunk->coord.x, chunk->coord.y, chunk->coord.z);

            if (chunk->flag & VOXEL_CHUNK_FLAG_SHOULD_REMOVE) {
                pool_release(&vw->chunks, chunk_id);
            }

            // This continues the iteration and skips the code below
            chunk_iterate_continue_n(chunk->element_count);
        }

        if (chunk->flag & VOXEL_CHUNK_FLAG_IS_NEW) {
            // Add new chunk to octree
            octnode_insert(&vw->oct_new, chunk, chunk->coord);
            chunk->flag &= ~VOXEL_CHUNK_FLAG_IS_NEW;
            chunk->flag |= VOXEL_CHUNK_FLAG_IS_CHANGED;
        }

        if (chunk->flag & VOXEL_CHUNK_FLAG_IS_CHANGED) {
            // Rebuild mesh
            voxel_chunk_mesh_build(&vw->map, chunk);
            chunk->flag &= ~VOXEL_CHUNK_FLAG_IS_CHANGED;
        }

        chunk_iterate_continue_n(chunk->element_count);
    } chunk_iterate_end;
}

inline
VoxelChunk* voxel_world_chunk_get(HashMap* map, int32 cx, int32 cy, int32 cz) NO_EXCEPT
{
    HashEntryVoidPKeyInt64* entry = voxel_hashmap_get_entry(map, cx, cy, cz);
    return entry ? (VoxelChunk *) entry->value : NULL;
}

// @question do we need separate get and create functions?
static inline
VoxelChunk* voxel_world_chunk_get_or_create(VoxelWorld* vw, int32 cx, int32 cy, int32 cz) NO_EXCEPT
{
    HashEntryVoidPKeyInt64* entry = voxel_hashmap_get_entry(&vw->map, cx, cy, cz);
    if(entry) {
        return (VoxelChunk *) entry->value;
    }

    VoxelChunk* chunk = (VoxelChunk *) pool_get_memory_one(&vw->chunks);
    voxel_chunk_init(chunk, cx, cy, cz);

    entry = voxel_hashmap_insert(&vw->map, cx, cy, cz, chunk);

    return (VoxelChunk *) entry->value;
}

inline
void voxel_world_voxel_set(VoxelWorld* vw, int32 world_x, int32 world_y, int32 world_z, Voxel v) NO_EXCEPT
{
    int32 cx = floor_div(world_x, VOXEL_CHUNK_SIZE);
    int32 cy = floor_div(world_y, VOXEL_CHUNK_SIZE);
    int32 cz = floor_div(world_z, VOXEL_CHUNK_SIZE);

    // Transform global coordinates to local/chunk coordinates
    int32 lx = world_x - cx * VOXEL_CHUNK_SIZE;
    int32 ly = world_y - cy * VOXEL_CHUNK_SIZE;
    int32 lz = world_z - cz * VOXEL_CHUNK_SIZE;

    VoxelChunk* chunk = voxel_world_chunk_get_or_create(vw, cx, cy, cz);
    voxel_chunk_set(chunk, lx, ly, lz, v);
}

// voxel_coord inside the chunk
FORCE_INLINE
void voxel_world_voxel_set(VoxelWorld* vw, v3_int32 chunk_coord, v3_int32 voxel_coord, Voxel v) NO_EXCEPT
{
    VoxelChunk* chunk = voxel_world_chunk_get_or_create(vw, chunk_coord.x, chunk_coord.y, chunk_coord.z);
    voxel_chunk_set(chunk, voxel_coord.x, voxel_coord.y, voxel_coord.z, v);
}

// max_depth represents the distance in chunks
// @bug using depth for this is bad because of how it is calculated 8^n. I think defining the max amount of visible chunks as input is much better
inline
void voxel_world_alloc(VoxelWorld* const vw, v3_int32 pos, int chunk_count) NO_EXCEPT
{
    const int node_count = chunk_count * 8;

    // @todo calculate required size based on depth
    buffer_alloc(&vw->mem, 1 * GIGABYTE, 1 * GIGABYTE, ASSUMED_CACHE_LINE_SIZE);
    pool_init(&vw->chunks, &vw->mem, chunk_count, sizeof(VoxelChunk), ASSUMED_CACHE_LINE_SIZE);

    // We want a hashmap with 2* the amount of chunks for reduced hash collisions
    hashmap_create(&vw->map, node_count, sizeof(HashEntryVoidP), &vw->mem, 32);
    vw->map.hash_function = hash_int64;

    // We expect at most chunk_count elements in our draw_array.
    // @performance Maybe chunk_count is too large, this is the max number which probably never is reached?
    vw->draw_array.elements = (VoxelDrawChunk  *) buffer_get_memory(&vw->mem, chunk_count * sizeof(VoxelDrawChunk), sizeof(size_t));
    vw->draw_array.size = chunk_count;

    // Reserve max amount of node memory space
    // @performance Depending on the optimization maybe we want a different data structure compared to an array?
    vw->oct_old.root = (OctNode *) buffer_get_memory(&vw->mem, sizeof(OctNode) * node_count, sizeof(size_t));
    vw->oct_old.root->has_data = true;
    vw->oct_old.last = vw->oct_old.root;

    // @bug This is a problem for when we change the max_depth
    //  We then need to grow this buffer, which is currently not possible
    //  Sure we could allocate a new one but then we would basically waste the old one one as unused memory
    vw->oct_new.root = (OctNode *) buffer_get_memory(&vw->mem, sizeof(OctNode) * node_count, sizeof(size_t));
    vw->oct_new.root->has_data = true;
    vw->oct_new.last = vw->oct_new.root;

    /*
    for (int32 x = 0; x < 128; ++x) {
        for (int32 z = 0; z < 128; ++z) {
            voxel_world_voxel_set(vw, x, 0, z, {1, 0});
        }
    }
    */

    octree_create(&vw->oct_old, pos, vw->chunks.capacity * VOXEL_CHUNK_SIZE);
    octree_create(&vw->oct_new, pos, vw->chunks.capacity * VOXEL_CHUNK_SIZE);
    // This doesn't happen because we first need to load the respective chunks for the pos into the hash map
    // voxel_world_update_pos(vw, pos);

    // Actual usage:
    //      1. voxel_world_init with player spawn position
    //      2. load all voxel close to player position
    //      2. load all chunks via voxel_world_chunk_add(vw, chunk)
    //      3. voxel_world_chunk_update()

    // Per frame:
    //      Player moves and IFF crossed chunk?
    //          voxel_world_chunk_add(), add new chunks
    //          voxel_world_chunk_remesh(), remesh newly added chunks manually
    //          voxel_world_update_pos()
    //      IFF voxel changed?
    //          voxel_world_chunk_update() WARNING update_pos might have alread updated, sometimes useless
    //      IFF movement or rotation, can be maybe optimized further if the view rotation or movement follows some constraints (e.g. looking down, moving along the same vector and no chunk crossing)
    //          voxel_draw_array_build()

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

FORCE_INLINE
void voxel_world_free(VoxelWorld* const vw)
{
    buffer_free(&vw->mem);
}

#endif
/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_ENTITY_VOXEL_OCTREE_H
#define COMS_ENTITY_VOXEL_OCTREE_H

#include "../../stdlib/Stdlib.h"
#include "../../stdlib/GameMathTypes.h"
#include "Voxel.h"

// @question I think we could easily make this a general purpose octree without much compromising. This way we wouldn't need two different versions
//          The biggest problem is probably that we are using the bounding box to identify the chunk level node

/**
 * The octree itself doesn't hold any data, it just references it
 * Therefore removing data from the octree doesn't free the memory of the data
 */
struct VoxelOctNode {
    // @question Shouldn't we make this floats?
    AABB_int32 bounds;

    // @question Shouldn't we make this a bitfield instead of individual variables.
    // At the moment it doesn't provide any padding benefits because of the other data sizes
    bool is_leaf;
    bool has_data;

    VoxelOctNode* child[8]; // children or NULL

    // The data can be an entire chunk, an object, a collection of objects, a single voxel, a single vertex etc.
    // On higher levels this can contain similar information as a leaf node but with less detail (LOD implementation)
    // The actual data might be dynamically generated or loaded from file depending on your implementation
    // The programmer is responsible to provide and handle that data
    const void* data;
};

struct VoxelOctree {
    VoxelOctNode* root;

    // Last added node
    VoxelOctNode* last;
    int32 max_depth;
    //f32 looseness; // >1.0 (e.g., 1.5) used to handle objects overlapping a chunk boundary
};

// @performance Instead of doing this and setting pointers maybe it would be better to just always have a fully setup tree
//              We shouldn't have to set pointers all the time whenever we setup a new tree, just the has_data and data should be setup
//              Maybe we can keep an empty tree cached which we then just copy over. Problem with that would be the pointers which don't carry over.
static inline
VoxelOctNode* voxel_octnode_child_create(VoxelOctree* const tree) NO_EXCEPT
{
    VoxelOctNode* const node = ++tree->last;
    memset(node, 0, sizeof(VoxelOctNode));

    node->has_data = true;

    return node;
}

static inline
int32 voxel_octree_child_index_from_coord(const VoxelOctNode* const node, v3_int32 coord) NO_EXCEPT
{
    const v3_int32 center = aabb_center(node->bounds);

    // We'll decide which side of the center the chunk lies on in each axis
    int32 index = 0;
    if (coord.x >= center.x) {
        index |= COORD_AXIS_X;
    }

    if (coord.y >= center.y) {
        index |= COORD_AXIS_Y;
    }

    if (coord.z >= center.z) {
        index |= COORD_AXIS_Z;
    }

    return index;
}

static inline
void voxel_octnode_child_aabb_compute(AABB_int32* out, const AABB_int32* const parent, int32 child_index) NO_EXCEPT
{
    v3_int32 center = aabb_center(*parent);
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

// This inserts a voxel into the tree
// If we have a parent, we can skip some iterations
static inline
void voxel_octnode_insert(VoxelOctree* const tree, const VoxelChunk* chunk, v3_int32 voxel_coord, VoxelOctNode* const parent = NULL) NO_EXCEPT
{
    VoxelOctNode* node = parent ? parent : tree->root;
    while (!node->is_leaf) {
        node->has_data = true;

        int32 child_index = voxel_octree_child_index_from_coord(node, voxel_coord);
        if (node->child[child_index] == NULL) {
            node->child[child_index] = voxel_octnode_child_create(tree);
            voxel_octnode_child_aabb_compute(&node->child[child_index]->bounds, &node->bounds, child_index);

            // Check if this is a leaf
            // Leafs have a aabb box width of VOXEL_CHUNK_SIZE in our specific implementation
            node->child[child_index]->is_leaf = (node->child[child_index]->bounds.max.x - node->child[child_index]->bounds.min.x) <= 1;
        }

        // @performance Do we need this here or can we move this inside the if above?
        if (node->child[child_index]->bounds.max.x - node->child[child_index]->bounds.min.x <= VOXEL_CHUNK_SIZE) {
            node->data = chunk;
        }

        node = node->child[child_index];
    }

    node->is_leaf = true;
    node->has_data = true;
}

// this inserts a whole chunk into the octree
// If we have a parent, we can skip some computations
static inline
void voxel_octnode_insert(VoxelOctree* const tree, const VoxelChunk* chunk, VoxelOctNode* const parent = NULL) NO_EXCEPT
{
    VoxelOctNode* node = parent ? parent : tree->root;

    while (node->bounds.max.x - node->bounds.min.x > VOXEL_CHUNK_SIZE) {
        int32 child_index = voxel_octree_child_index_from_coord(node, chunk->coord);

        node->has_data = true;
        if (node->child[child_index] == NULL) {
            node->child[child_index] = voxel_octnode_child_create(tree);
            voxel_octnode_child_aabb_compute(&node->child[child_index]->bounds, &node->bounds, child_index);
        }

        node = node->child[child_index];
    }

    node->data = chunk;
    node->is_leaf = false;
    node->has_data = true;

    // We now need to insert the individual voxels of the chunk
    for (int i = 0; i < ARRAY_COUNT(chunk->vox); ++i) {
        int32 x = i % VOXEL_CHUNK_SIZE;
        int32 y = (i / VOXEL_CHUNK_SIZE) % VOXEL_CHUNK_SIZE;
        int32 z = i / (VOXEL_CHUNK_SIZE * VOXEL_CHUNK_SIZE);

        if (voxel_is_solid(chunk->vox[i].type)) {
            voxel_octnode_insert(tree, chunk, {x, y, z}, node);
        }
    }
}

// Removes node and all children
static
bool voxel_octnode_remove_recursive(VoxelOctNode* const node) NO_EXCEPT
{
    // @performance Do I even need this check? - Can't we handle this at the calling place?
    if (!node) {
        return false;
    }

    if (node->is_leaf) {
        for (int i = 0; i < ARRAY_COUNT(node->child); ++i) {
            if (node->child[i] && node->child[i]->has_data) {
                voxel_octnode_remove_recursive(node->child[i]);
            }
        }
    }

    node->has_data = false;
    node->data = NULL;

    return true;
}

static
bool voxel_octnode_remove_recursive(VoxelOctNode* const node, v3_int32 coord) NO_EXCEPT
{
    if (!node) {
        return false;
    }

    if (node->is_leaf) {
        node->data = NULL;
        node->has_data = false;

        return true;
    }

    // Find which child to descend into
    int32 child_index = voxel_octree_child_index_from_coord(node, coord);
    VoxelOctNode* child = node->child[child_index];
    if (!child) {
        return false;
    }

    // Recurse down
    bool removed = voxel_octnode_remove_recursive(child, coord);
    if (!removed) {
        return false;
    }

    // @bug What if we didn't iterate through the entire tree but started at the correct node as function parameter?
    //          In that case we would never get here and we would actually need a parent pointer to go back and then check from the parent if it has children
    // After child removal, recompute parent has_data
    bool any_child_has_data = false;
    for (int i = 0; i < 8; ++i) {
        if (node->child[i] && node->child[i]->has_data) {
            any_child_has_data = true;
            break;
        }
    }
    node->has_data = any_child_has_data;

    return true;
}

// Removes a chunk and therefor also all its sub-nodes which are part of the chunk
static
bool voxel_octnode_remove_recursive(VoxelOctNode* const node, const VoxelChunk* const chunk) NO_EXCEPT
{
    // @performance Do I even need this check? - Can't we handle this at the calling place?
    if (!node) {
        return false;
    }

    if (node->bounds.max.x - node->bounds.min.x == VOXEL_CHUNK_SIZE) {
        if (node->data != chunk) {
            return false;
        }

        // Removes a node from the octree
        return voxel_octnode_remove_recursive(node);
    }

    // Find which child to descend into
    int32 child_index = voxel_octree_child_index_from_coord(node, chunk->coord);
    VoxelOctNode* child = node->child[child_index];
    if (!child) {
        return false;
    }

    // Recurse down
    bool removed = voxel_octnode_remove_recursive(child, chunk);
    if (!removed) {
        return false;
    }

    // @bug What if we didn't iterate through the entire tree but started at the correct node as function parameter?
    //          In that case we would never get here and we would actually need a parent pointer to go back and then check from the parent if it has children
    // After child removal, recompute parent has_data
    bool any_child_has_data = false;
    for (int i = 0; i < 8; ++i) {
        if (node->child[i] && node->child[i]->has_data) {
            any_child_has_data = true;
            break;
        }
    }
    node->has_data = any_child_has_data;

    return true;
}

static inline
void voxel_octnode_remove(VoxelOctree* tree, const VoxelChunk* chunk) NO_EXCEPT
{
    // @performance Do I even need this check? - Can't we handle this at the calling place?
    if (!tree || !tree->root || !chunk) {
        return;
    }

    voxel_octnode_remove_recursive(tree->root, chunk);
}

static inline
void voxel_octnode_remove(VoxelOctree* tree, v3_int32 coord) NO_EXCEPT
{
    // @performance Do I even need this check? - Can't we handle this at the calling place?
    if (!tree || !tree->root) {
        return;
    }

    voxel_octnode_remove_recursive(tree->root, coord);
}

static inline
void voxel_octree_create(VoxelOctree* const tree, v3_int32 pos) NO_EXCEPT
{
    // 8^n == 2^(3*n)
    // @performance Do we really need that many chunks? That is the maximum amount of chunks but most of them will be empty
    //          Which also means very often we will not go the full depth
    //          This is potentially a huge amount of unused reserved nodes
    //          On the other hand the nodes are very small and the actual data is in the chunks
    const int32 chunk_count = OMS_POW2_I32(3 * (tree->max_depth - 1));
    //memset(tree->root, 0, sizeof(VoxelOctNode) * chunk_count);

    VoxelOctNode* root = tree->root;
    root->is_leaf = tree->max_depth == 1;

    // Center of the chunk the player is currently standing on
    // calculate amount of chunks towards x, y, z (rounded down)
    // then walk along those axis
    // then add another halve chunk size to go to the center
    //      if negative we need to subtract because int32 rounding rounds up if negative
    const v3_int32 ctr = {
        floor_div(pos.x, VOXEL_CHUNK_SIZE) * VOXEL_CHUNK_SIZE + OMS_DIV2_I32(VOXEL_CHUNK_SIZE),
        floor_div(pos.y, VOXEL_CHUNK_SIZE) * VOXEL_CHUNK_SIZE + OMS_DIV2_I32(VOXEL_CHUNK_SIZE),
        floor_div(pos.z, VOXEL_CHUNK_SIZE) * VOXEL_CHUNK_SIZE + OMS_DIV2_I32(VOXEL_CHUNK_SIZE)
    };

    const int32 half_size = OMS_DIV2_I32(VOXEL_CHUNK_SIZE) * chunk_count;

    root->bounds = {
        { ctr.x - half_size, ctr.y - half_size, ctr.z - half_size },
        { ctr.x + half_size, ctr.y + half_size, ctr.z + half_size },
    };
}

#endif
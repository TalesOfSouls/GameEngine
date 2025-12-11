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

#include "../../stdlib/Types.h"
#include "../../stdlib/GameMathTypes.h"
#include "Voxel.h"

struct VoxelOctNode {
    AABB_int32 bounds;
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
int32 voxel_octree_child_index_for_chunk(const VoxelOctNode* node, const VoxelChunk* chunk) {
    v3_int32 center = aabb_center(node->bounds);

    // We'll decide which side of the center the chunk lies on in each axis
    int32 index = 0;
    if (chunk->coord.x >= center.x) {
        index |= COORD_AXIS_X;
    }

    if (chunk->coord.y >= center.y) {
        index |= COORD_AXIS_Y;
    }

    if (chunk->coord.z >= center.z) {
        index |= COORD_AXIS_Z;
    }

    return index;
}

static inline
void voxel_octnode_child_aabb_compute(AABB_int32* out, const AABB_int32* parent, int32 child_index)
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

static inline
void voxel_octnode_insert(VoxelOctree* tree, const VoxelChunk* chunk) {
    VoxelOctNode* node = tree->root;
    while (!node->is_leaf) {
        node->has_data = true;

        int32 child_index = voxel_octree_child_index_for_chunk(node, chunk);
        if (node->child[child_index] == NULL) {
            node->child[child_index] = voxel_octnode_child_create(tree);
            voxel_octnode_child_aabb_compute(&node->child[child_index]->bounds, &node->bounds, child_index);

            // Check if this is a leaf
            // Leafs have a aabb box width of VOXEL_CHUNK_SIZE in our specific implementation
            node->child[child_index]->is_leaf = (node->child[child_index]->bounds.max.x - node->child[child_index]->bounds.min.x) <= VOXEL_CHUNK_SIZE;
        }

        node = node->child[child_index];
    }

    node->chunk = chunk;
    node->is_leaf = true;
    node->has_data = true;
}

static
bool voxel_octnode_remove_recursive(VoxelOctNode* node, const VoxelChunk* chunk) {
    if (!node) {
        return false;
    }

    if (node->is_leaf) {
        if (node->chunk == chunk) {
            node->chunk = NULL;
            node->has_data = false;
            // stays a leaf
            return true; // removed
        }
        return false; // not found
    }

    // Find which child to descend into
    int32 child_index = voxel_octree_child_index_for_chunk(node, chunk);
    VoxelOctNode* child = node->child[child_index];
    if (!child) {
        return false;
    }

    // Recurse down
    bool removed = voxel_octnode_remove_recursive(child, chunk);
    if (!removed) {
        return false;
    }

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
void voxel_octnode_remove(VoxelOctree* tree, const VoxelChunk* chunk) {
    if (!tree || !tree->root || !chunk) {
        return;
    }

    voxel_octnode_remove_recursive(tree->root, chunk);
}

static inline
void voxel_octree_create(VoxelOctree* tree, v3_int32 pos)
{
    // 8^n == 2^(3*n)
    // @performance Do we really need that many chunks? That is the maximum amount of chunks but most of them will be empty
    //          Which also means very often we will not go the full depth
    //          This is potentially a huge amount of unused reserved nodes
    //          On the other hand the nodes are very small and the actual data is in the chunks
    int32 chunk_count = OMS_POW2_I32(3 * (tree->max_depth - 1));
    //memset(tree->root, 0, sizeof(VoxelOctNode) * chunk_count);

    VoxelOctNode* root = tree->root;
    root->is_leaf = tree->max_depth == 1;

    // Center of the chunk the player is currently standing on
    // calculate amount of chunks towards x, y, z (rounded down)
    // then walk along those axis
    // then add another halve chunk size to go to the center
    //      if negative we need to subtract because int32 rounding rounds up if negative
    v3_int32 ctr = {
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
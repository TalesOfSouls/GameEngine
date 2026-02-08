/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_OCTREE_H
#define COMS_STDLIB_OCTREE_H

#include "Stdlib.h"
#include "GameMathTypes.h"

enum TreeCoordAxis {
    TREE_COORD_AXIS_X = 1 << 0,
    TREE_COORD_AXIS_Y = 1 << 1,
    TREE_COORD_AXIS_Z = 1 << 2
};

/**
 * The octree itself doesn't hold any data, it just references it
 * Therefore removing data from the octree doesn't free the memory of the data
 */
struct OctNode {
    // @question Shouldn't we make this floats?
    //          There is an argument that ints is fine since nodes will always be chunks
    //          and in this case we just say the smallest chunk size is of length = 1
    //          What a length of 1 actually represents can be arbitrary,
    //          we may even say that a length of 1 represents a length unit of 0.25 in our rendering
    v3_int32 coord;
    AABB_int32 bounds;

    // @question Shouldn't we make this a bitfield instead of individual variables.
    //          At the moment it doesn't provide any padding benefits because of the other data sizes
    bool is_leaf;
    bool has_data;

    // Child or NULL if no child available
    OctNode* child[8];

    // The data can be an entire chunk, an object, a collection of objects, a single voxel, a single vertex etc.
    // On higher node levels this can contain similar information as a leaf node but with less detail (LOD implementation)
    // The actual data might be dynamically generated or loaded from file depending on your implementation
    // The programmer is responsible to provide and handle that data
    const void* data;
};

struct Octree {
    OctNode* root;

    // Last added node
    OctNode* last;

    //f32 looseness; // >1.0 (e.g., 1.5) used to handle objects overlapping a chunk boundary

    int32 leaf_size;
};

static inline
OctNode* octnode_child_create(Octree* const tree) NO_EXCEPT
{
    OctNode* const node = ++tree->last;
    memset(node, 0, sizeof(OctNode));

    // @question Do I really want this to be true by default?
    node->has_data = true;

    return node;
}

static inline
int32 octree_child_index_from_coord(v3_int32 anchor, v3_int32 coord) NO_EXCEPT
{
    // We'll decide which side of the anchor the chunk lies on in each axis
    int32 index = 0;
    if (coord.x >= anchor.x) {
        index |= TREE_COORD_AXIS_X;
    }

    if (coord.y >= anchor.y) {
        index |= TREE_COORD_AXIS_Y;
    }

    if (coord.z >= anchor.z) {
        index |= TREE_COORD_AXIS_Z;
    }

    return index;
}

static inline
AABB_int32 octnode_child_aabb_compute(
    AABB_int32 parent,
    v3_int32 parent_center,
    int32 child_index
) NO_EXCEPT
{
    AABB_int32 out = parent;

    // Split along X
    if (child_index & TREE_COORD_AXIS_X) {
        out.min.x = parent_center.x;
    } else {
        out.max.x = parent_center.x;
    }

    // Split along Y
    if (child_index & TREE_COORD_AXIS_Y) {
        out.min.y = parent_center.y;
    } else {
        out.max.y = parent_center.y;
    }

    // Split along Z
    if (child_index & TREE_COORD_AXIS_Z) {
        out.min.z = parent_center.z;
    } else {
        out.max.z = parent_center.z;
    }

    return out;
}

static inline
v3_int32 octnode_child_anchor_compute(
    v3_int32 parent_min,
    v3_int32 center,
    int32 child_index
) NO_EXCEPT
{
    v3_int32 anchor;

    anchor.x = (child_index & TREE_COORD_AXIS_X)
        ? center.x
        : parent_min.x;

    anchor.y = (child_index & TREE_COORD_AXIS_Y)
        ? center.y
        : parent_min.y;

    anchor.z = (child_index & TREE_COORD_AXIS_Z)
        ? center.z
        : parent_min.z;

    return anchor;
}

// This inserts a voxel into the tree
// If we have a parent, we can skip some iterations
static inline
void octnode_insert(Octree* const tree, const void* data, v3_int32 node_coord, OctNode* const parent = NULL) NO_EXCEPT
{
    OctNode* node = parent ? parent : tree->root;
    while (!node->is_leaf) {
        node->has_data = true;

        const int32 child_index = octree_child_index_from_coord(node->coord, node_coord);
        if (node->child[child_index] == NULL) {
            const v3_int32 center = aabb_center(node->bounds);
            node->child[child_index] = octnode_child_create(tree);
            node->child[child_index]->bounds = octnode_child_aabb_compute(node->bounds, center, child_index);
            node->child[child_index]->coord = octnode_child_anchor_compute(node->bounds.min, center, child_index);
        }

        if (node->child[child_index]->bounds.max.x - node->child[child_index]->bounds.min.x <= tree->leaf_size) {
            node->data = data;
        }

        node = node->child[child_index];
    }

    node->is_leaf = true;
    node->has_data = true;
}

// Removes node and all children
static inline
bool octnode_remove_recursive(OctNode* const node) NO_EXCEPT
{
    // @performance Do I even need this check? - Can't we handle this at the calling place?
    if (!node) {
        return false;
    }

    for (int i = 0; i < ARRAY_COUNT(node->child); ++i) {
        if (node->child[i] && node->child[i]->has_data) {
            octnode_remove_recursive(node->child[i]);
        }
    }

    node->has_data = false;
    node->data = NULL;

    return true;
}

static inline
bool octnode_has_data_recalculate(OctNode* const node) NO_EXCEPT
{
    if (node->data) {
        return true;
    }

    bool any_child_has_data = false;
    for (int i = 0; i < 8; ++i) {
        if (node->child[i] && node->child[i]->has_data) {
            any_child_has_data = true;
            break;
        }
    }

    return any_child_has_data;
}

static inline
bool octnode_remove_recursive(OctNode* const node, v3_int32 coord) NO_EXCEPT
{
    // Is leaf -> has to be a match
    if (node->is_leaf) {
        node->data = NULL;
        node->has_data = false;

        return true;
    }

    // No leaf but coord matches -> remove this node and all child nodes
    if (memcmp(&node->coord, &coord, sizeof(coord)) == 0) {
        octnode_remove_recursive(node);

        node->data = NULL;
        node->has_data = false;

        return true;
    }

    // Find which child to descend into, if the current node is not the correct final destination
    const int32 child_index = octree_child_index_from_coord(node->coord, coord);
    OctNode* child = node->child[child_index];
    if (!child) {
        return false;
    }

    // Recurse down based on child_index
    bool removed = octnode_remove_recursive(child, coord);
    if (!removed) {
        return false;
    }

    // @bug What if we didn't iterate through the entire tree but started at the correct node as function parameter?
    //          In that case we would never get here and we would actually need a parent pointer to go back and then check from the parent if it has children
    // After child removal, recompute parent has_data
    node->has_data = octnode_has_data_recalculate(node);

    return true;
}

// Same as version without data, it just validates if the data pointer matches as additional check
static inline
bool octnode_remove_recursive(OctNode* const node, const void* const data, v3_int32 data_coord) NO_EXCEPT
{
    // Is leaf -> has to be a match
    if (node->is_leaf) {
        if (node->data != data) {
            return false;
        }

        node->data = NULL;
        node->has_data = false;

        return true;
    }

    // No leaf but coord matches -> remove this node and all child nodes
    if (memcmp(&node->coord, &data_coord, sizeof(data_coord)) == 0) {
        if (node->data != data) {
            return false;
        }

        octnode_remove_recursive(node);

        node->data = NULL;
        node->has_data = false;

        return true;
    }

    // Find which child to descend into, if the current node is not the correct final destination
    const int32 child_index = octree_child_index_from_coord(node->coord, data_coord);
    OctNode* child = node->child[child_index];
    if (!child) {
        return false;
    }

    // Recurse down based on child_index
    bool removed = octnode_remove_recursive(child, data_coord);
    if (!removed) {
        return false;
    }

    // @bug What if we didn't iterate through the entire tree but started at the correct node as function parameter?
    //          In that case we would never get here and we would actually need a parent pointer to go back and then check from the parent if it has children
    // After child removal, recompute parent has_data
    node->has_data = octnode_has_data_recalculate(node);

    return true;
}

// Removes node if coord and data match
FORCE_INLINE
void octnode_remove(Octree* tree, const void* data, v3_int32 data_coord) NO_EXCEPT
{
    octnode_remove_recursive(tree->root, data, data_coord);
}

// Removes node if coord matches
FORCE_INLINE
void octnode_remove(Octree* tree, v3_int32 coord) NO_EXCEPT
{
    octnode_remove_recursive(tree->root, coord);
}

static inline
void octree_create(Octree* const tree, v3_int32 anchor, int root_dimension) NO_EXCEPT
{
    OctNode* root = tree->root;

    if (tree->leaf_size == 0) {
        tree->leaf_size = 1;
    }

    root->coord = anchor;
    root->bounds = {
        anchor,
        { anchor.x + root_dimension, anchor.y + root_dimension, anchor.z + root_dimension }
    };
}

#endif
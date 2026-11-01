#include "../TestFramework.h"
#include "../../stdlib/Octree.h"

/**
 * Octree tests
 *
 * The octree is an intrusive structure: the caller provides the OctNode memory and the
 * tree only references the data. The tests therefore build the node storage manually.
 *
 * NOTE (@see report): octnode_insert() cannot be tested with an empty tree, the
 * `while (!node->is_leaf)` loop never terminates because freshly created nodes are
 * zero initialized (is_leaf == false) and nothing inside the loop ever sets is_leaf.
 * The insert test below therefore uses a pre-built child chain.
 */

static void octree_test_create_node(OctNode* node, const v3_int32& min, const v3_int32& max, const v3_int32& coord) {
    memset(node, 0, sizeof(OctNode));
    node->bounds.min = min;
    node->bounds.max = max;
    node->coord = coord;
}

static v3_int32 octree_test_v3(int32 x, int32 y, int32 z) {
    v3_int32 value;
    value.x = x;
    value.y = y;
    value.z = z;

    return value;
}

static void test_octree_create() {
    OctNode nodes[8] = {0};
    Octree tree = {0};
    tree.root = &nodes[0];
    tree.last = &nodes[0];

    const v3_int32 anchor = octree_test_v3(-8, -8, -8);
    octree_create(&tree, anchor, 16);

    TEST_EQUALS(tree.leaf_size, 1);
    TEST_EQUALS(tree.root, &nodes[0]);
    TEST_EQUALS(tree.last, &nodes[0]);

    TEST_EQUALS(tree.root->coord.x, -8);
    TEST_EQUALS(tree.root->coord.y, -8);
    TEST_EQUALS(tree.root->coord.z, -8);

    TEST_EQUALS(tree.root->bounds.min.x, -8);
    TEST_EQUALS(tree.root->bounds.min.z, -8);
    TEST_EQUALS(tree.root->bounds.max.x, 8);
    TEST_EQUALS(tree.root->bounds.max.y, 8);
    TEST_EQUALS(tree.root->bounds.max.z, 8);

    // The root has to be split, so it can't be a leaf
    TEST_FALSE(tree.root->is_leaf);
    TEST_FALSE(tree.root->has_data);
    TEST_EQUALS(tree.root->data, NULL);

    // An explicit leaf size is preserved
    tree.leaf_size = 4;
    octree_create(&tree, octree_test_v3(0, 0, 0), 32);
    TEST_EQUALS(tree.leaf_size, 4);
    TEST_EQUALS(tree.root->bounds.max.x, 32);
}

static void test_octree_child_index() {
    const v3_int32 anchor = octree_test_v3(0, 0, 0);

    TEST_EQUALS(octree_child_index_from_coord(anchor, octree_test_v3(-1, -1, -1)), 0);
    TEST_EQUALS(octree_child_index_from_coord(anchor, octree_test_v3(1, -1, -1)), TREE_COORD_AXIS_X);
    TEST_EQUALS(octree_child_index_from_coord(anchor, octree_test_v3(-1, 1, -1)), TREE_COORD_AXIS_Y);
    TEST_EQUALS(octree_child_index_from_coord(anchor, octree_test_v3(-1, -1, 1)), TREE_COORD_AXIS_Z);
    TEST_EQUALS(octree_child_index_from_coord(anchor, octree_test_v3(1, 1, -1)), TREE_COORD_AXIS_X | TREE_COORD_AXIS_Y);
    TEST_EQUALS(octree_child_index_from_coord(anchor, octree_test_v3(1, 1, 1)), 7);

    // A coordinate equal to the anchor belongs to the positive half
    TEST_EQUALS(octree_child_index_from_coord(anchor, anchor), 7);

    const v3_int32 other_anchor = octree_test_v3(10, 10, 10);
    TEST_EQUALS(octree_child_index_from_coord(other_anchor, octree_test_v3(9, 9, 9)), 0);
    TEST_EQUALS(octree_child_index_from_coord(other_anchor, octree_test_v3(11, 9, 11)), TREE_COORD_AXIS_X | TREE_COORD_AXIS_Z);
}

static void test_octree_child_aabb() {
    const v3_int32 min = octree_test_v3(0, 0, 0);
    const v3_int32 max = octree_test_v3(8, 8, 8);
    AABB_int32 parent;
    parent.min = min;
    parent.max = max;

    const v3_int32 center = aabb_center(parent);
    TEST_EQUALS(center.x, 4);
    TEST_EQUALS(center.y, 4);
    TEST_EQUALS(center.z, 4);

    int64 volume_sum = 0;
    for (int32 i = 0; i < 8; ++i) {
        const AABB_int32 child = octnode_child_aabb_compute(parent, center, i);

        // Every child is inside the parent
        TEST_TRUE(child.min.x >= parent.min.x && child.max.x <= parent.max.x);
        TEST_TRUE(child.min.y >= parent.min.y && child.max.y <= parent.max.y);
        TEST_TRUE(child.min.z >= parent.min.z && child.max.z <= parent.max.z);

        // The split axis moves exactly one side to the center
        TEST_EQUALS((i & TREE_COORD_AXIS_X) ? child.min.x : child.max.x, 4);
        TEST_EQUALS((i & TREE_COORD_AXIS_Y) ? child.min.y : child.max.y, 4);
        TEST_EQUALS((i & TREE_COORD_AXIS_Z) ? child.min.z : child.max.z, 4);

        volume_sum += (int64) (child.max.x - child.min.x)
            * (child.max.y - child.min.y)
            * (child.max.z - child.min.z);
    }

    // The eight children have to fill the parent completely
    TEST_EQUALS(volume_sum, 8 * 8 * 8);

    const AABB_int32 first = octnode_child_aabb_compute(parent, center, 0);
    TEST_EQUALS(first.min.x, 0);
    TEST_EQUALS(first.max.x, 4);
    TEST_EQUALS(first.max.y, 4);
    TEST_EQUALS(first.max.z, 4);

    const AABB_int32 last = octnode_child_aabb_compute(parent, center, 7);
    TEST_EQUALS(last.min.x, 4);
    TEST_EQUALS(last.min.y, 4);
    TEST_EQUALS(last.min.z, 4);
    TEST_EQUALS(last.max.x, 8);
    TEST_EQUALS(last.max.y, 8);
    TEST_EQUALS(last.max.z, 8);

    const AABB_int32 x_only = octnode_child_aabb_compute(parent, center, TREE_COORD_AXIS_X);
    TEST_EQUALS(x_only.min.x, 4);
    TEST_EQUALS(x_only.max.x, 8);
    TEST_EQUALS(x_only.max.y, 4);
    TEST_EQUALS(x_only.max.z, 4);
}

static void test_octree_child_anchor() {
    const v3_int32 parent_min = octree_test_v3(0, 0, 0);
    const v3_int32 center = octree_test_v3(4, 4, 4);

    const v3_int32 child0 = octnode_child_anchor_compute(parent_min, center, 0);
    TEST_EQUALS(child0.x, 0);
    TEST_EQUALS(child0.y, 0);
    TEST_EQUALS(child0.z, 0);

    const v3_int32 child7 = octnode_child_anchor_compute(parent_min, center, 7);
    TEST_EQUALS(child7.x, 4);
    TEST_EQUALS(child7.y, 4);
    TEST_EQUALS(child7.z, 4);

    const v3_int32 child_x = octnode_child_anchor_compute(parent_min, center, TREE_COORD_AXIS_X);
    TEST_EQUALS(child_x.x, 4);
    TEST_EQUALS(child_x.y, 0);
    TEST_EQUALS(child_x.z, 0);

    const v3_int32 child_yz = octnode_child_anchor_compute(parent_min, center, TREE_COORD_AXIS_Y | TREE_COORD_AXIS_Z);
    TEST_EQUALS(child_yz.x, 0);
    TEST_EQUALS(child_yz.y, 4);
    TEST_EQUALS(child_yz.z, 4);

    // The anchor of a child has to be inside the child AABB
    AABB_int32 parent;
    parent.min = parent_min;
    parent.max = octree_test_v3(8, 8, 8);
    const AABB_int32 child_bounds = octnode_child_aabb_compute(parent, center, 7);
    TEST_EQUALS(child7.x, child_bounds.min.x);
    TEST_EQUALS(child7.y, child_bounds.min.y);
    TEST_EQUALS(child7.z, child_bounds.min.z);
}

static void test_octree_node_create() {
    OctNode nodes[8] = {0};
    Octree tree = {0};
    tree.root = &nodes[0];
    tree.last = &nodes[0];

    // Dirty the storage so the zero initialization is provable
    memset(nodes, 0xFF, sizeof(nodes));
    nodes[0].is_leaf = false;

    OctNode* first = octnode_child_create(&tree);
    TEST_EQUALS(first, &nodes[1]);
    TEST_EQUALS(tree.last, &nodes[1]);
    TEST_FALSE(first->is_leaf);
    TEST_FALSE(first->has_data);
    TEST_EQUALS(first->data, NULL);
    TEST_EQUALS(first->child[0], NULL);
    TEST_EQUALS(first->coord.x, 0);
    TEST_EQUALS(first->bounds.max.x, 0);

    OctNode* second = octnode_child_create(&tree);
    TEST_EQUALS(second, &nodes[2]);
    TEST_EQUALS(tree.last, &nodes[2]);
    TEST_NOT_EQUALS(first, second);
}

static void test_octree_insert() {
    OctNode nodes[8] = {0};
    Octree tree = {0};
    tree.root = &nodes[0];
    tree.last = &nodes[0];
    tree.leaf_size = 4;

    const v3_int32 anchor = octree_test_v3(0, 0, 0);
    octree_create(&tree, anchor, 8);

    // Build the root -> child -> leaf chain manually (@see note at the top)
    const v3_int32 root_center = aabb_center(tree.root->bounds);
    OctNode* child = octnode_child_create(&tree);
    child->bounds = octnode_child_aabb_compute(tree.root->bounds, root_center, 7);
    child->coord = octnode_child_anchor_compute(tree.root->bounds.min, root_center, 7);
    tree.root->child[7] = child;

    const v3_int32 child_center = aabb_center(child->bounds);
    OctNode* leaf = octnode_child_create(&tree);
    leaf->bounds = octnode_child_aabb_compute(child->bounds, child_center, 7);
    leaf->coord = octnode_child_anchor_compute(child->bounds.min, child_center, 7);
    leaf->is_leaf = true;
    child->child[7] = leaf;

    const int32 node_count_before = (int32) (tree.last - tree.root + 1);
    TEST_EQUALS(node_count_before, 3);

    int32 payload = 42;
    octnode_insert(&tree, &payload, octree_test_v3(5, 5, 5));

    // Insert stops at the pre-built leaf, no additional nodes are created
    TEST_EQUALS((int32) (tree.last - tree.root + 1), node_count_before);

    // The path to the leaf is marked as containing data
    TEST_TRUE(tree.root->has_data);
    TEST_TRUE(child->has_data);
    TEST_TRUE(leaf->is_leaf);
    TEST_TRUE(leaf->has_data);
    TEST_EQUALS(leaf->coord.x, 6);
    TEST_EQUALS(leaf->coord.y, 6);
    TEST_EQUALS(leaf->coord.z, 6);
    TEST_EQUALS(leaf->bounds.min.x, 6);
    TEST_EQUALS(leaf->bounds.max.x, 8);

    // The parent of the leaf is a leaf_size wide chunk -> it carries the data
    TEST_EQUALS(child->data, &payload);
}

static void test_octree_remove() {
    OctNode nodes[8] = {0};
    Octree tree = {0};
    tree.root = &nodes[0];
    tree.last = &nodes[5];
    tree.leaf_size = 4;

    const v3_int32 origin = octree_test_v3(0, 0, 0);
    octree_create(&tree, origin, 8);

    int32 data_a = 1;
    int32 data_b = 2;

    // root -> child0 (leaf, data_a), child7 (leaf, data_b)
    OctNode* child0 = &nodes[1];
    octree_test_create_node(child0, octree_test_v3(0, 0, 0), octree_test_v3(4, 4, 4), octree_test_v3(0, 0, 0));
    child0->is_leaf = true;
    child0->has_data = true;
    child0->data = &data_a;

    OctNode* child7 = &nodes[2];
    octree_test_create_node(child7, octree_test_v3(4, 4, 4), octree_test_v3(8, 8, 8), octree_test_v3(4, 4, 4));
    child7->is_leaf = true;
    child7->has_data = true;
    child7->data = &data_b;

    tree.root->child[0] = child0;
    tree.root->child[7] = child7;
    tree.root->has_data = true;

    // Removing a leaf by coordinate only touches that leaf
    octnode_remove(&tree, octree_test_v3(4, 4, 4));
    TEST_FALSE(child7->has_data);
    TEST_EQUALS(child7->data, NULL);
    TEST_TRUE(child0->has_data);
    TEST_EQUALS(child0->data, &data_a);

    // The parent still has data through the sibling
    TEST_TRUE(tree.root->has_data);

    // Removing a coordinate that matches the node removes the whole subtree
    octnode_remove(&tree, origin);
    TEST_FALSE(tree.root->has_data);
    TEST_EQUALS(tree.root->data, NULL);
    TEST_FALSE(child0->has_data);
    TEST_EQUALS(child0->data, NULL);

    // The children are not deallocated (the octree only references the data)
    TEST_NOT_EQUALS(tree.root->child[0], NULL);
    TEST_NOT_EQUALS(tree.root->child[7], NULL);

    // Recursive removal of a single node clears the children as well
    child0->has_data = true;
    child0->data = &data_a;
    child7->has_data = true;
    child7->data = &data_b;
    TEST_TRUE(octnode_remove_recursive(tree.root));
    TEST_FALSE(tree.root->has_data);
    TEST_FALSE(child0->has_data);
    TEST_FALSE(child7->has_data);

    // A null node is handled
    TEST_FALSE(octnode_remove_recursive((OctNode*) NULL));
}

static void test_octree_remove_missing() {
    OctNode nodes[4] = {0};
    Octree tree = {0};
    tree.root = &nodes[0];
    tree.last = &nodes[3];

    octree_create(&tree, octree_test_v3(0, 0, 0), 8);

    // The root has no children, so the descent can't find the coordinate
    octnode_remove(&tree, octree_test_v3(5, 5, 5));
    TEST_EQUALS(tree.root->child[7], NULL);
    TEST_FALSE(tree.root->has_data);

    // A leaf root is cleared regardless of the coordinate
    tree.root->is_leaf = true;
    tree.root->has_data = true;
    int32 payload = 5;
    tree.root->data = &payload;

    octnode_remove(&tree, octree_test_v3(5, 5, 5));
    TEST_FALSE(tree.root->has_data);
    TEST_EQUALS(tree.root->data, NULL);
}

static void test_octree_has_data_recalculate() {
    OctNode node = {0};

    // No data, no children
    TEST_FALSE(octnode_has_data_recalculate(&node));

    // Own data
    int32 payload = 7;
    node.data = &payload;
    TEST_TRUE(octnode_has_data_recalculate(&node));

    // Child data only
    node.data = NULL;
    TEST_FALSE(octnode_has_data_recalculate(&node));

    OctNode child = {0};
    child.has_data = true;
    node.child[3] = &child;
    TEST_TRUE(octnode_has_data_recalculate(&node));

    child.has_data = false;
    TEST_FALSE(octnode_has_data_recalculate(&node));

    // Last child index is checked as well
    node.child[3] = NULL;
    node.child[7] = &child;
    child.has_data = true;
    TEST_TRUE(octnode_has_data_recalculate(&node));

    node.child[7] = NULL;
    TEST_FALSE(octnode_has_data_recalculate(&node));
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main StdlibOctreeTest
#endif

int main() {
    TEST_INIT(128);

    TEST_RUN(test_octree_create);
    TEST_RUN(test_octree_child_index);
    TEST_RUN(test_octree_child_aabb);
    TEST_RUN(test_octree_child_anchor);
    TEST_RUN(test_octree_node_create);
    TEST_RUN(test_octree_insert);
    TEST_RUN(test_octree_remove);
    TEST_RUN(test_octree_remove_missing);
    TEST_RUN(test_octree_has_data_recalculate);

    TEST_FINALIZE();

    return 0;
}

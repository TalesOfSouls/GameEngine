#include "../../TestFramework.h"
#include "../../../entity/voxel/VoxelWorldMap.h"

static void test_voxel_world_map() {
    VoxelWorld vw = {0};
    v3_int32 pos = {0};

    const int width = 10;
    const int depth = 25;
    const int height = 2;
    voxel_world_alloc(&vw, pos, height * width * depth);

    Voxel voxel = {
        1,
        0
    };

    ////////////////////////////////////////////////////////////////
    // Test adding a voxel at the center chunk
    ////////////////////////////////////////////////////////////////
    voxel_world_voxel_set(&vw, 1, 1, 1, voxel);
    voxel_world_chunk_update(&vw);

    v3_int32 chunk_coord = {0, 0, 0};
    VoxelChunk* chunk_inserted = voxel_world_chunk_get(&vw.map, chunk_coord.x, chunk_coord.y, chunk_coord.z);
    TEST_EQUALS(chunk_inserted->coord.x, chunk_coord.x);
    TEST_EQUALS(chunk_inserted->coord.y, chunk_coord.y);
    TEST_EQUALS(chunk_inserted->coord.z, chunk_coord.z);

    Voxel voxel_inserted = voxel_world_map_get(&vw.map, chunk_coord, 1, 1, 1);
    TEST_EQUALS(voxel_inserted.type, 1);

    ////////////////////////////////////////////////////////////////
    // Test adding a voxel at an offset from the center
    ////////////////////////////////////////////////////////////////
    voxel_world_voxel_set(&vw, -42, -42, -42, voxel);
    voxel_world_chunk_update(&vw);

    // -42 is 2 chunk offsets if a chunk is 32 voxels (+42 would only be one)
    // The center of the center chunk is not at 0,0,0 (that is the bottom/left/front corner)
    chunk_coord = {-2, -2, -2};
    chunk_inserted = voxel_world_chunk_get(&vw.map, chunk_coord.x, chunk_coord.y, chunk_coord.z);
    TEST_EQUALS(chunk_inserted->coord.x, chunk_coord.x);
    TEST_EQUALS(chunk_inserted->coord.y, chunk_coord.y);
    TEST_EQUALS(chunk_inserted->coord.z, chunk_coord.z);

    // Negative coordinates need to be handled in a special way
    voxel_inserted = voxel_world_map_get(
        &vw.map, chunk_coord,
        VOXEL_CHUNK_SIZE + -42 + VOXEL_CHUNK_SIZE,
        VOXEL_CHUNK_SIZE + -42 + VOXEL_CHUNK_SIZE,
        VOXEL_CHUNK_SIZE + -42 + VOXEL_CHUNK_SIZE
    );
    TEST_EQUALS(voxel_inserted.type, 1);

    ////////////////////////////////////////////////////////////////
    // Test invalid voxel
    ////////////////////////////////////////////////////////////////
    Voxel voxel_invalid = voxel_world_map_get(&vw.map, chunk_coord, 0, 0, 0);
    TEST_NOT_EQUALS(voxel_invalid.type, 1);

    voxel_world_free(&vw);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main VoxelWorldMapTest
#endif

int main() {
    TEST_INIT(5);

    TEST_RUN(test_voxel_world_map);

    TEST_FINALIZE();

    return 0;
}
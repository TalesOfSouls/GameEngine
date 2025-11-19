#include "../../TestFramework.h"
#include "../../../entity/voxel/VoxelWorldMap.h"

static void test_voxel_world_map() {
    HashMap hm = {};
    hashmap_alloc(&hm, 3, sizeof(HashEntryInt32));

    TEST_TRUE(hm.buf.count > 0);

    hashmap_free(&hm);
    TEST_EQUALS(hm.buf.size, 0);
    TEST_EQUALS(hm.buf.memory, NULL);
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
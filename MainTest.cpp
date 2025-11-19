#define UBER_TEST 1
#define PERFORMANCE_TEST 1

//#include "math/EvaluatorTest.cpp"
#include "tests/memory/ChunkMemoryTest.cpp"
#include "tests/memory/RingMemoryTest.cpp"
#include "tests/stdlib/HashMapTest.cpp"
#include "tests/ui/UILayoutTest.cpp"
#include "tests/ui/UIThemeTest.cpp"
#include "tests/utils/BitUtilsTest.cpp"
#include "tests/utils/EndianUtilsTest.cpp"
#include "tests/utils/StringUtilsTest.cpp"
#include "tests/utils/MathUtilsTest.cpp"
#include "tests/utils/UtilsTest.cpp"
#include "tests/utils/TimeUtilsTest.cpp"
#include "tests/entity/voxel/VoxelWorldMapTest.cpp"

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
#endif

int main() {
    TEST_HEADER();

    _rng_state_32 = (int32) time_index();
    _rng_state_64 = time_index();

    //MathEvaluatorTest();
    MemoryChunkMemoryTest();
    MemoryRingMemoryTest();
    StdlibHashMapTest();
    UIUILayoutTest();
    UIUIThemeTest();
    UtilsBitUtilsTest();
    UtilsEndianUtilsTest();
    UtilsStringUtilsTest();
    UtilsMathUtilsTest();
    UtilsUtilsTest();
    UtilsTimeUtilsTest();
    VoxelWorldMapTest();

    TEST_FOOTER();

    return _test_global_assert_error_count ? 1 : 0;
}
#define UBER_TEST 1
#define PERFORMANCE_TEST 0

//#include "math/EvaluatorTest.cpp"
#include "memory/ChunkMemoryTest.cpp"
#include "memory/RingMemoryTest.cpp"
#include "stdlib/HashMapTest.cpp"
//#include "ui/UILayoutTest.cpp"
//#include "ui/UIThemeTest.cpp"
#include "utils/BitUtilsTest.cpp"
#include "utils/EndianUtilsTest.cpp"
#include "utils/StringUtilsTest.cpp"
#include "utils/MathUtilsTest.cpp"
#include "utils/UtilsTest.cpp"
#include "utils/TimeUtilsTest.cpp"

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
#endif

int main() {
    TEST_HEADER();

    //MathEvaluatorTest();
    MemoryChunkMemoryTest();
    MemoryRingMemoryTest();
    StdlibHashMapTest();
    //UIUILayoutTest();
    //UIUIThemeTest();
    UtilsBitUtilsTest();
    UtilsEndianUtilsTest();
    UtilsStringUtilsTest();
    UtilsMathUtilsTest();
    UtilsUtilsTest();
    UtilsTimeUtilsTest();

    TEST_FOOTER();

    return _test_global_assert_error_count ? 1 : 0;
}
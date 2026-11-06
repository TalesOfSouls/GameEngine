#define UBER_TEST 1
#define PERFORMANCE_TEST 0

// The higher level systems (ui, asset, drm, image, localization) are being ported
// to the current APIs, the priorities are utilities, memory and stdlib
#define TEST_SYSTEMS 1

// VoxelWorldMap still uses the removed HashMap (asset/entity refactor), not testable yet
#define TEST_VOXEL 0

#include "stdlib/Stdlib.h"
#include "utils/RandomUtils.h"

/**
 * Test suite
 *
 * All tests are included into this file, every test file defines its own main()
 * which is renamed to the test function name via the UBER_TEST macro.
 *
 * The test order is: utilities, memory, stdlib (highest priority) followed by the
 * higher level systems
 *
 * Rules for new test files (@see tests/README.md):
 *      - the main macro is <Directory><FileName>Test, e.g. UtilsSimpleStringTest
 *      - every test function is prefixed with the module, e.g. test_simple_string_*
 *      - a test file must also compile and pass standalone (tests_iter.bat/.sh)
 */
#include "tests/math/EvaluatorTest.cpp"
#include "tests/hash/HashTest.cpp"

// Utilities
#include "tests/utils/BitUtilsTest.cpp"
#include "tests/utils/EndianUtilsTest.cpp"
#include "tests/utils/UtilsTest.cpp"
#include "tests/utils/MathUtilsTest.cpp"
#include "tests/utils/StringUtilsTest.cpp"
#include "tests/utils/TimeUtilsTest.cpp"
#include "tests/utils/SimpleStringTest.cpp"
#include "tests/utils/SimpleStringUtilsTest.cpp"
#include "tests/utils/RandomUtilsTest.cpp"
#include "tests/utils/RegexSimplifiedTest.cpp"
#include "tests/utils/ConstantTimeTest.cpp"
#include "tests/utils/SecureRandomTest.cpp"

// Memory
#include "tests/memory/BufferMemoryTest.cpp"
#include "tests/memory/RingMemoryTest.cpp"
#include "tests/memory/ChunkMemoryTest.cpp"
#include "tests/memory/ChunkMemoryTTest.cpp"
#include "tests/memory/QueueTest.cpp"
#include "tests/memory/MemoryArenaTest.cpp"
#include "tests/memory/HeapTest.cpp"
#include "tests/memory/PointerMemoryTest.cpp"
#include "tests/memory/FragmentMemoryTest.cpp"
#include "tests/memory/FragmentMemoryTTest.cpp"
#include "tests/memory/DataPoolTest.cpp"
#include "tests/memory/SPSCQueueTTest.cpp"
#include "tests/memory/MPSCQueueTTest.cpp"
#include "tests/memory/MPMCQueueTTest.cpp"
#include "tests/memory/ThrdChunkMemoryTest.cpp"
#include "tests/memory/ThrdChunkMemoryTTest.cpp"

// Stdlib
#include "tests/stdlib/HashMapTest.cpp"
#include "tests/stdlib/ArrayVectorTest.cpp"
#include "tests/stdlib/StdlibInternalTest.cpp"
#include "tests/stdlib/HelperTest.cpp"
#include "tests/stdlib/TypesTest.cpp"
#include "tests/stdlib/PerfectHashMapTest.cpp"
#include "tests/stdlib/ThrdHashMapTTest.cpp"
#include "tests/stdlib/FixedFloatTest.cpp"
#include "tests/stdlib/OctreeTest.cpp"
#include "tests/stdlib/SimdTest.cpp"

// Object
#include "tests/object/OmxTest.cpp"

#if TEST_SYSTEMS
    #include "tests/localization/LanguageTest.cpp"
    #include "tests/ui/UILayoutTest.cpp"
    #include "tests/ui/UIThemeTest.cpp"
    #include "tests/asset/AssetArchiveTest.cpp"
    #include "tests/system/DRMTest.cpp"
    #include "tests/system/FileUtilsTest.cpp"
    #include "tests/image/QoiTest.cpp"
#endif

#if TEST_VOXEL
    #include "tests/entity/voxel/VoxelWorldMapTest.cpp"
#endif

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
#endif

int main() {
    TEST_HEADER();

    rand_setup();

    // Math
    MathEvaluatorTest();

    // Hash
    HashUtilsTest();

    // Utilities
    UtilsBitUtilsTest();
    UtilsEndianUtilsTest();
    UtilsUtilsTest();
    UtilsMathUtilsTest();
    UtilsStringUtilsTest();
    UtilsTimeUtilsTest();
    UtilsSimpleStringTest();
    UtilsSimpleStringUtilsTest();
    UtilsRandomUtilsTest();
    UtilsRegexSimplifiedTest();
    UtilsConstantTimeTest();
    UtilsSecureRandomTest();

    // Memory
    MemoryBufferMemoryTest();
    MemoryRingMemoryTest();
    MemoryChunkMemoryTest();
    MemoryChunkMemoryTTest();
    QueueTest();
    MemoryMemoryArenaTest();
    MemoryHeapTest();
    MemoryPointerMemoryTest();
    MemoryFragmentMemoryTest();
    MemoryFragmentMemoryTTest();
    MemoryDataPoolTest();
    MemorySPSCQueueTTest();
    MemoryMPSCQueueTTest();
    MemoryMPMCQueueTTest();
    MemoryThrdChunkMemoryTest();
    MemoryThrdChunkMemoryTTest();

    // Stdlib
    StdlibHashMapTest();
    StdlibArrayVectorTest();
    StdlibStdlibInternalTest();
    StdlibHelperTest();
    StdlibTypesTest();
    StdlibPerfectHashMapTest();
    StdlibThrdHashMapTTest();
    StdlibFixedFloatTest();
    StdlibOctreeTest();
    StdlibSimdTest();

    // Object
    ObjectOmxTest();

    #if TEST_SYSTEMS
        // Systems
        LocalizationLanguageTest();
        UIUILayoutTest();
        UIUIThemeTest();
        AssetArchiveTest();
        DRMTest();
        SystemFileUtilsTest();
        QoiTest();
    #endif

    #if TEST_VOXEL
        VoxelWorldMapTest();
    #endif

    TEST_FOOTER();

    return _test_global_assert_error_count ? 1 : 0;
}

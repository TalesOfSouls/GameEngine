# Tests

All tests are plain C++ files that are `#include`d into a single translation unit
([MainTest.cpp](../MainTest.cpp)). There is no external test framework, everything is
in [TestFramework.h](TestFramework.h).

## Running

| Command | What it does |
|---------|--------------|
| `tests.bat` / `test.sh` | Compile and run the whole suite (uber build) |
| `tests_iter.bat` / `tests_iter.sh` | Compile and run every `*Test.cpp` standalone |

Both scripts build the executable inside `../build`. The suite returns the amount of
failed assertions as exit code (`0` = everything passed) and prints one line per test
file:

```
Stat Tests   Assert(OK/NG)    Time(s)   Details
[OK]    15   (   72/   72)       0.00   .../tests/memory/ChunkMemoryTest.cpp
```

## Writing a new test

```cpp
#include "../TestFramework.h"
#include "../../stdlib/ArrayVector.cpp"   // or .h if the module has no .cpp

static void test_array_vector_alloc() {
    ArrayVector<int32> vec = {0};
    array_vector_alloc(&vec, 10, 20);
    TEST_EQUALS(vec.capacity, 10);
    array_vector_free(&vec);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main StdlibArrayVectorTest   // <Directory><FileName>Test
#endif

int main() {
    TEST_INIT(64);                       // upper bound of assertions of this file

    TEST_RUN(test_array_vector_alloc);

    TEST_FINALIZE();

    return 0;
}
```

Rules:

* **The whole suite is one translation unit.** All test functions must be unique, so
  prefix them with the module (`test_chunk_memory_alloc`, not `test_alloc`).
* `main` is renamed through the `UBER_TEST` macro. The name has to be unique as well
  (`<Directory><FileName>Test`), because [MainTest.cpp](../MainTest.cpp) calls it.
* A test file must **also compile and pass standalone** (that is what
  `tests_iter.bat`/`tests_iter.sh` do), so it has to include everything it needs.
* Include the `.cpp` of a module if it has one, otherwise the `.h`.
* Guard assertions that intentionally crash (`ASSERT_*` in debug builds) with
  `#if !DEBUG`.
* Windows headers define `small`, `near`, `far`, `interface` and `min`/`max` macros —
  never use those as identifier names.
* Tests must be deterministic, must not depend on timing and must not leave files
  behind (write into `relative_to_absolute("./...")` next to the executable if a file
  is really needed, and delete it again).

## Assertions

| Macro | Purpose |
|-------|---------|
| `TEST_EQUALS(a, b)` / `TEST_NOT_EQUALS(a, b)` | Compares as `int64` (integers, pointers, bools as ints) |
| `TEST_EQUALS_WITH_DELTA(a, b, delta)` | Floating point comparison |
| `TEST_TRUE(a)` / `TEST_FALSE(a)` | Boolean expressions |
| `TEST_MEMORY_EQUALS(ptr1, ptr2, len)` | Byte wise memory comparison |
| `TEST_MEMORY_NOT_EQUALS(ptr1, ptr2, len)` | Byte wise memory difference |
| `TEST_GREATER_THAN(a, b)` / `TEST_LESSER_THAN(a, b)` | Order comparison |
| `TEST_CONTAINS(haystack, needle)` | Reverse logic: fails if `strstr` finds `needle` |
| `TEST_TRUE_CONST(a)` / `TEST_FALSE_CONST(a)` | Compile time assertions |

`TEST_EQUALS`, `TEST_NOT_EQUALS`, `TEST_TRUE` and `TEST_FALSE` evaluate their arguments
exactly once, so function calls with side effects are safe inside of them.

## Coverage overview

Priority of this project is utilities, memory and stdlib; the higher level systems are
covered as well but are still being ported to the current APIs.

| Area | Test files |
|------|------------|
| `utils/` | `BitUtils`, `EndianUtils`, `Utils`, `MathUtils`, `StringUtils`, `TimeUtils`, `SimpleString`, `SimpleStringUtils`, `RandomUtils`, `RegexSimplified`, `ConstantTime`, `SecureRandom` |
| `math/` | `Evaluator` |
| `hash/` | `Crc`, `GeneralHash`, `Sha1`, `Sha256`, `Hmac` (`HashTest.cpp`) |
| `memory/` | `BufferMemory`, `RingMemory`, `ChunkMemory`, `ChunkMemoryT`, `Queue`, `QueueT`, `MemoryArena`, `Heap`, `PointerMemory`, `FragmentMemory`, `FragmentMemoryT`, `DataPool`, `SPSCQueueT`, `MPSCQueueT`, `MPMCQueueT`, `ThrdChunkMemory`, `ThrdChunkMemoryT` |
| `stdlib/` | `HashMapT`, `ArrayVector`, `stdlib_internal`, `Helper`, `Types`/`Defines`, `PerfectHashMap`, `ThrdHashMapT`, `FixedFloat`, `Octree`, `Simd` |
| Systems | `localization/Language`, `ui/UILayout`, `ui/UITheme`, `asset/AssetArchive`, `system/DRM`, `system/FileUtils`, `image/Qoi` |

Current size (uber build, DEBUG): **47 files, 483 test suites, ~7900 assertions**.

Not tested / only partially tested:

* `entity/voxel/VoxelWorldMap.h` still includes the removed `stdlib/HashMap.h`, the test
  exists but is disabled through `TEST_VOXEL` in [MainTest.cpp](../MainTest.cpp).
* `utils/PerformanceProfiler.h` is a standalone profiling demo (`main()` + Linux only
  `__builtin_ia32_rdtsc`) and is not part of the engine build.
* `utils/RegexSimplified.h` only implements single element patterns, so only that subset
  is asserted (multi element patterns, `{n,m}` repetition and general alternation are
  not implemented yet).
* `gpuapi/*`, `platform/*` (except `FileUtils`), `thread/*`, `network/*`, `audio/*`,
  `compression/*`, `encoding/*` need a device/window or are still in flux.
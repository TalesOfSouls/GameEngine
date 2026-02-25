#include "../TestFramework.h"
#include "../../memory/BufferMemory.h"

static void test_buffer_alloc() {
    BufferMemory mem = {0};
    buffer_alloc(&mem, 50, 50);

    TEST_TRUE(memcmp(mem.memory, mem.memory + 1, 49) == 0);

    // Every element is aligned according to the default alignment -> size >= constructor size
    TEST_EQUALS(mem.size, 64);

    buffer_free(&mem);
    TEST_EQUALS(mem.size, 0);
    TEST_EQUALS(mem.memory, NULL);
}

static void test_buffer_get_memory() {
    BufferMemory mem = {0};
    buffer_alloc(&mem, 50, 50);

    TEST_EQUALS(buffer_get_memory(&mem, 20), mem.memory);
    TEST_EQUALS(mem.head, mem.memory + 24);

    buffer_free(&mem);
}

static void test_buffer_reset() {
    BufferMemory mem = {0};
    buffer_alloc(&mem, 50, 50);

    buffer_get_memory(&mem, 20);
    TEST_NOT_EQUALS(mem.head, mem.memory);

    buffer_reset(&mem);
    TEST_EQUALS(mem.head, mem.memory);

    buffer_free(&mem);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main MemoryBufferMemoryTest
#endif

int main() {
    TEST_INIT(25);

    TEST_RUN(test_buffer_alloc);
    TEST_RUN(test_buffer_get_memory);
    TEST_RUN(test_buffer_reset);

    TEST_FINALIZE();

    return 0;
}

#include "../TestFramework.h"
#include "../../memory/RingMemory.cpp"

static void test_ring_alloc() {
    RingMemory mem = {0};
    ring_alloc(&mem, 50, 50);

    TEST_TRUE(memcmp(mem.memory, mem.memory + 1, 49) == 0);

    // Every element is aligned according to the default alignment -> size >= constructor size
    TEST_EQUALS(mem.size, 64);

    ring_free(&mem);
    TEST_EQUALS(mem.size, 0);
    TEST_EQUALS(mem.memory, NULL);
}

static void test_ring_get_memory() {
    RingMemory mem = {0};
    ring_alloc(&mem, 50, 50);

    TEST_EQUALS(ring_get_memory(&mem, 20), mem.memory);
    TEST_EQUALS(mem.head, mem.memory + 24);

    ring_free(&mem);
}

static void test_ring_calculate_position() {
    RingMemory mem = {0};
    ring_alloc(&mem, 50, 50);

    ring_get_memory(&mem, 20);
    TEST_EQUALS(ring_calculate_position(&mem, 20), mem.memory + 24);

    ring_free(&mem);
}

static void test_ring_reset() {
    RingMemory mem = {0};
    ring_alloc(&mem, 50, 50);

    ring_get_memory(&mem, 20);
    TEST_NOT_EQUALS(mem.head, mem.memory);

    ring_reset(&mem);
    TEST_EQUALS(mem.head, mem.memory);

    ring_free(&mem);
}

static void test_ring_get_memory_nomove() {
    RingMemory mem = {0};
    ring_alloc(&mem, 50, 50);

    TEST_EQUALS(ring_get_memory_nomove(&mem, 20), mem.memory);
    TEST_EQUALS(mem.head, mem.memory);

    ring_free(&mem);
}

static void test_ring_move_pointer() {
    RingMemory mem = {0};
    ring_alloc(&mem, 50, 50);

    ring_move_pointer(&mem, &mem.head, 20);
    TEST_EQUALS(mem.head, mem.memory + 24);

    ring_free(&mem);
}

static void test_ring_commit_safe() {
    RingMemory mem = {0};
    ring_alloc(&mem, 50, 50);

    ring_get_memory(&mem, 24, 1);

    TEST_TRUE(ring_commit_safe(&mem, 24));

    // False because of alignment
    //TEST_FALSE(ring_commit_safe(&mem, 26));

    TEST_TRUE(ring_commit_safe(&mem, 30, 1));
    TEST_FALSE(ring_commit_safe(&mem, 45));
    TEST_FALSE(ring_commit_safe(&mem, 101));

    ring_free(&mem);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main MemoryRingMemoryTest
#endif

int main() {
    TEST_INIT(25);

    TEST_RUN(test_ring_alloc);
    TEST_RUN(test_ring_get_memory);
    TEST_RUN(test_ring_calculate_position);
    TEST_RUN(test_ring_reset);
    TEST_RUN(test_ring_get_memory_nomove);
    TEST_RUN(test_ring_move_pointer);
    TEST_RUN(test_ring_commit_safe);

    TEST_FINALIZE();

    return 0;
}

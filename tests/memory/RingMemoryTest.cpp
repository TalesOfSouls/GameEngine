#include "../TestFramework.h"
#include "../../memory/RingMemory.h"

static void test_ring_alloc() {
    RingMemory mem = {};
    ring_alloc(&mem, 50);

    ASSERT_TRUE(memcmp(mem.memory, mem.memory + 1, 49) == 0);
    ASSERT_EQUALS(mem.size, 50);

    ring_free(&mem);
    ASSERT_EQUALS(mem.size, 0);
    ASSERT_EQUALS(mem.memory, NULL);
}

static void test_ring_get_memory() {
    RingMemory mem = {};
    ring_alloc(&mem, 50);

    ASSERT_EQUALS(ring_get_memory(&mem, 20), mem.memory);
    ASSERT_EQUALS(mem.head, mem.memory + 20);

    ring_free(&mem);
}

static void test_ring_calculate_position() {
    RingMemory mem = {};
    ring_alloc(&mem, 50);

    ring_get_memory(&mem, 20);
    ASSERT_EQUALS(ring_calculate_position(&mem, 20), mem.memory + 20);

    ring_free(&mem);
}

static void test_ring_reset() {
    RingMemory mem = {};
    ring_alloc(&mem, 50);

    ring_get_memory(&mem, 20);
    ASSERT_NOT_EQUALS(mem.head, mem.memory);

    ring_reset(&mem);
    ASSERT_EQUALS(mem.head, mem.memory);

    ring_free(&mem);
}

static void test_ring_get_memory_nomove() {
    RingMemory mem = {};
    ring_alloc(&mem, 50);

    ASSERT_EQUALS(ring_get_memory_nomove(&mem, 20), mem.memory);
    ASSERT_EQUALS(mem.head, mem.memory);

    ring_free(&mem);
}

static void test_ring_move_pointer() {
    RingMemory mem = {};
    ring_alloc(&mem, 50);

    ring_move_pointer(&mem, &mem.head, 20);
    ASSERT_EQUALS(mem.head, mem.memory + 20);

    ring_free(&mem);
}

static void test_ring_commit_safe() {
    RingMemory mem = {};
    ring_alloc(&mem, 50);

    ring_get_memory(&mem, 20, 1);

    ASSERT_TRUE(ring_commit_safe(&mem, 20));

    // False because of alignment
    ASSERT_FALSE(ring_commit_safe(&mem, 30));

    ASSERT_TRUE(ring_commit_safe(&mem, 30, 1));
    ASSERT_FALSE(ring_commit_safe(&mem, 45));
    ASSERT_FALSE(ring_commit_safe(&mem, 101));

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

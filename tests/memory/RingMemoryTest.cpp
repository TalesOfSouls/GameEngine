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

static void test_ring_init() {
    byte buffer[128];
    RingMemory mem = {0};
    ring_init(&mem, buffer, 100);

    TEST_EQUALS(mem.size, 104); // ALIGN_UP(100, sizeof(size_t))
    TEST_EQUALS(mem.head, mem.memory);
    TEST_EQUALS(mem.tail, mem.memory);
    TEST_EQUALS(mem.end, mem.memory + mem.size);
    TEST_TRUE(mem.memory >= buffer);

    // Never frees memory that it doesn't own
    mem.memory = NULL;
    mem.size = 0;
}

static void test_ring_memory_get() {
    RingMemory mem = {0};
    ring_alloc(&mem, 50, 50);

    TEST_EQUALS(memory_get(&mem, 20), mem.memory);
    TEST_EQUALS(mem.head, mem.memory + 24);

    ring_free(&mem);
}

static void test_ring_calculate_position() {
    RingMemory mem = {0};
    ring_alloc(&mem, 50, 50);

    memory_get(&mem, 20);
    TEST_EQUALS(ring_calculate_position(&mem, 20), mem.memory + 24);

    ring_free(&mem);
}

static void test_ring_reset() {
    RingMemory mem = {0};
    ring_alloc(&mem, 50, 50);

    memory_get(&mem, 20);
    TEST_NOT_EQUALS(mem.head, mem.memory);

    ring_reset(&mem);
    TEST_EQUALS(mem.head, mem.memory);

    ring_free(&mem);
}

static void test_ring_memory_get_nomove() {
    RingMemory mem = {0};
    ring_alloc(&mem, 50, 50);

    TEST_EQUALS(ring_memory_get_nomove(&mem, 20), mem.memory);
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

static void test_ring_move_pointer_wrapping() {
    RingMemory mem = {0};
    ring_alloc(&mem, 50, 50);

    // 3 * 24 = 72 > 64 -> the pointer has to wrap around
    ring_move_pointer(&mem, &mem.head, 20);
    ring_move_pointer(&mem, &mem.head, 20);
    TEST_EQUALS(mem.head, mem.memory + 48);

    ring_move_pointer(&mem, &mem.head, 20);
    TEST_EQUALS(mem.head, mem.memory + 24);

    ring_free(&mem);
}

static void test_ring_get_element() {
    RingMemory mem = {0};
    ring_alloc(&mem, 50, 50);

    mem.memory[0] = 125;
    mem.memory[24] = 250;

    TEST_EQUALS(*ring_get_element(&mem, 0, 24), 125);
    TEST_EQUALS(*ring_get_element(&mem, 1, 24), 250);

    ring_free(&mem);
}

static void test_ring_commit_safe() {
    RingMemory mem = {0};
    ring_alloc(&mem, 50, 50);

    memory_get(&mem, 24, 1);

    TEST_TRUE(ring_commit_safe(&mem, 24));

    // False because of alignment
    //TEST_FALSE(ring_commit_safe(&mem, 26));

    TEST_TRUE(ring_commit_safe(&mem, 30, 1));
    TEST_FALSE(ring_commit_safe(&mem, 45));
    TEST_FALSE(ring_commit_safe(&mem, 101));

    ring_free(&mem);
}

static void test_ring_dump() {
    RingMemory mem = {0};
    ring_alloc(&mem, 50, 50);

    memory_get(&mem, 20);

    byte out[256];
    const int64 dump_size = ring_dump(&mem, out);

    TEST_TRUE(dump_size > 0);
    TEST_TRUE(dump_size <= (int64) sizeof(out));

    // size + head + alignment + tail + end + memory
    const int64 header_size = (int64) (sizeof(size_t) + sizeof(uint64) + sizeof(uint32) + sizeof(uint64) + sizeof(uint64));
    TEST_EQUALS(dump_size, header_size + (int64) mem.size);

    // The memory of the ring is part of the dump
    TEST_MEMORY_EQUALS(out + header_size, mem.memory, mem.size);

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
    TEST_RUN(test_ring_init);
    TEST_RUN(test_ring_memory_get);
    TEST_RUN(test_ring_calculate_position);
    TEST_RUN(test_ring_reset);
    TEST_RUN(test_ring_memory_get_nomove);
    TEST_RUN(test_ring_move_pointer);
    TEST_RUN(test_ring_move_pointer_wrapping);
    TEST_RUN(test_ring_get_element);
    TEST_RUN(test_ring_commit_safe);
    TEST_RUN(test_ring_dump);

    TEST_FINALIZE();

    return 0;
}

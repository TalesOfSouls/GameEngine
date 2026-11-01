#include "../TestFramework.h"
#include "../../memory/ChunkMemoryT.cpp"

struct ChunkTestElement {
    int32 id;
    int32 value;
};

static void test_chunk_t_alloc() {
    ChunkMemoryT<ChunkTestElement> mem = {0};
    chunk_alloc(&mem, 10, 10);

    TEST_EQUALS(mem.capacity, 10);
    TEST_EQUALS(mem.last_pos, -1);
    TEST_NOT_EQUALS(mem.memory, NULL);
    TEST_NOT_EQUALS(mem.free, NULL);

    // The element memory is zero initialized
    TEST_TRUE(memcmp(mem.memory, mem.memory + 1, 9 * sizeof(ChunkTestElement)) == 0);

    // The free array is behind the element memory
    TEST_EQUALS((byte *) mem.free, (byte *) mem.memory + 10 * sizeof(ChunkTestElement));

    chunk_free(&mem);
    TEST_EQUALS(mem.capacity, 0);
    TEST_EQUALS(mem.memory, NULL);
}

static void test_chunk_t_size() {
    // capacity * sizeof(T) + free array + padding
    const size_t size = chunk_size(sizeof(ChunkTestElement), 64);

    TEST_EQUALS(size, 64 * sizeof(ChunkTestElement) + sizeof(size_t) * 1 + sizeof(size_t));
}

static void test_chunk_t_element_get() {
    ChunkMemoryT<ChunkTestElement> mem = {0};
    chunk_alloc(&mem, 10, 10);

    TEST_EQUALS(chunk_element_get(&mem, 2), &mem.memory[2]);

    // Out of bounds
    TEST_EQUALS(chunk_element_get(&mem, 10), NULL);
    TEST_EQUALS(chunk_element_get(&mem, 100), NULL);

    chunk_free(&mem);
}

static void test_chunk_t_reserve() {
    ChunkMemoryT<ChunkTestElement> mem = {0};
    chunk_alloc(&mem, 10, 10);

    TEST_EQUALS(chunk_reserve_one(&mem), 0);
    TEST_EQUALS(chunk_reserve_one(&mem), 1);
    TEST_FALSE(chunk_is_free(&mem, 0));
    TEST_FALSE(chunk_is_free(&mem, 1));
    TEST_TRUE(chunk_is_free(&mem, 2));

    TEST_EQUALS(chunk_reserve(&mem, 2), 2);
    TEST_FALSE(chunk_is_free(&mem, 2));
    TEST_FALSE(chunk_is_free(&mem, 3));

    chunk_free(&mem);
}

static void test_chunk_t_free_element() {
    ChunkMemoryT<ChunkTestElement> mem = {0};
    chunk_alloc(&mem, 10, 10);

    chunk_reserve_one(&mem);
    chunk_reserve_one(&mem);
    TEST_FALSE(chunk_is_free(&mem, 0));

    chunk_free_element(&mem, (uint32) 0);
    TEST_TRUE(chunk_is_free(&mem, 0));
    TEST_FALSE(chunk_is_free(&mem, 1));

    chunk_free_element(&mem, (size_t) 0, 1);
    TEST_TRUE(chunk_is_free(&mem, 1));

    // Free by pointer
    chunk_reserve_one(&mem);
    TEST_FALSE(chunk_is_free(&mem, 0));
    chunk_free_elements(&mem, mem.memory, 1);
    TEST_TRUE(chunk_is_free(&mem, 0));

    chunk_free_elements(&mem, 0, 10);
    TEST_EQUALS(*mem.free, 0);

    chunk_free(&mem);
}

static void test_chunk_t_element_insert() {
    ChunkMemoryT<ChunkTestElement> mem = {0};
    chunk_alloc(&mem, 10, 10);

    const ChunkTestElement a = {1, 10};
    chunk_element_insert(&mem, a);

    const ChunkTestElement* inserted = chunk_element_get(&mem, 0);
    TEST_EQUALS(inserted->id, 1);
    TEST_EQUALS(inserted->value, 10);

    const ChunkTestElement b = {2, 20};
    chunk_element_insert(&mem, &b);

    inserted = chunk_element_get(&mem, 1);
    TEST_EQUALS(inserted->id, 2);
    TEST_EQUALS(inserted->value, 20);

    chunk_free(&mem);
}

static void test_chunk_t_memory_get() {
    ChunkMemoryT<ChunkTestElement> mem = {0};
    chunk_alloc(&mem, 10, 10);

    // One element
    ChunkTestElement* a = memory_get(&mem, sizeof(ChunkTestElement));
    TEST_EQUALS(a, mem.memory);

    // Two elements
    ChunkTestElement* b = memory_get(&mem, sizeof(ChunkTestElement) * 2);
    TEST_EQUALS(b, mem.memory + 1);

    // Temporary memory is free again right away
    ChunkTestElement* c = memory_get_temp(&mem, sizeof(ChunkTestElement) * 2);
    TEST_EQUALS(c, mem.memory + 3);
    TEST_TRUE(chunk_is_free(&mem, 3));
    TEST_TRUE(chunk_is_free(&mem, 4));

    chunk_free(&mem);
}

static void test_chunk_t_iterate() {
    ChunkMemoryT<ChunkTestElement> mem = {0};
    chunk_alloc(&mem, 10, 10);

    for (int32 i = 0; i < 10; ++i) {
        chunk_reserve_one(&mem);
    }

    chunk_free_element(&mem, (uint32) 0);
    chunk_free_element(&mem, (uint32) 5);

    int32 visited_count = 0;
    int32 expected[] = {1, 2, 3, 4, 6, 7, 8, 9};
    int32 expected_index = 0;
    int32 chunk_id = 0;

    chunk_iterate_start(&mem, chunk_id) {
        TEST_EQUALS(chunk_id, expected[expected_index]);
        ++expected_index;
        ++visited_count;
    } chunk_iterate_end

    TEST_EQUALS(visited_count, ARRAY_COUNT(expected));

    chunk_free(&mem);
}

static void test_chunk_t_dump_load() {
    ChunkMemoryT<ChunkTestElement> mem = {0};
    chunk_alloc(&mem, 10, 10);

    mem.memory[0].id = 5;
    mem.memory[0].value = 50;
    chunk_reserve(&mem, 2);

    byte out[1024];
    const int64 dump_size = chunk_dump(&mem, out);

    ChunkMemoryT<ChunkTestElement> mem2 = {0};
    chunk_alloc(&mem2, 10, 10);

    const int64 load_size = chunk_load(&mem2, out);

    TEST_EQUALS(dump_size, load_size);
    TEST_EQUALS(mem2.capacity, mem.capacity);
    TEST_EQUALS(mem2.last_pos, mem.last_pos);
    TEST_EQUALS(mem2.memory[0].id, 5);
    TEST_EQUALS(mem2.memory[0].value, 50);
    TEST_FALSE(chunk_is_free(&mem2, 0));
    TEST_FALSE(chunk_is_free(&mem2, 1));

    chunk_free(&mem);
    chunk_free(&mem2);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main MemoryChunkMemoryTTest
#endif

int main() {
    TEST_INIT(25);

    TEST_RUN(test_chunk_t_alloc);
    TEST_RUN(test_chunk_t_size);
    TEST_RUN(test_chunk_t_element_get);
    TEST_RUN(test_chunk_t_reserve);
    TEST_RUN(test_chunk_t_free_element);
    TEST_RUN(test_chunk_t_element_insert);
    TEST_RUN(test_chunk_t_memory_get);
    TEST_RUN(test_chunk_t_iterate);
    TEST_RUN(test_chunk_t_dump_load);

    TEST_FINALIZE();

    return 0;
}

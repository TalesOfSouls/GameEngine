#include "../TestFramework.h"
#include "../../memory/ChunkMemory.cpp"

static void test_chunk_alloc() {
    ChunkMemory mem = {0};
    chunk_alloc(&mem, 10, 10, 10);

    TEST_TRUE(memcmp(mem.memory, mem.memory + 1, 10 * 10) == 0);
    TEST_EQUALS(*mem.free, 0);

    chunk_free(&mem);
    TEST_EQUALS(mem.size, 0);
    TEST_EQUALS(mem.memory, NULL);
}

// The element/chunk size is aligned up according to the alignment
static void test_chunk_size_alignment() {
    ChunkMemory mem = {0};
    chunk_alloc(&mem, 10, 10, 10);

    // 10 -> ALIGN_UP(10, sizeof(size_t)) = 16
    TEST_EQUALS(mem.chunk_size, 16);
    TEST_EQUALS(mem.alignment, sizeof(size_t));

    chunk_free(&mem);

    ChunkMemory aligned = {0};
    chunk_alloc(&aligned, 4, 4, 10, ASSUMED_CACHE_LINE_SIZE);

    // 10 -> ALIGN_UP(10, 64) = 64
    TEST_EQUALS(aligned.chunk_size, 64);

    chunk_free(&aligned);
}

static void test_chunk_id_from_memory() {
    ChunkMemory mem = {0};
    chunk_alloc(&mem, 10, 10, 10);

    TEST_EQUALS(chunk_id_from_memory(mem.memory, mem.memory, mem.chunk_size), 0);
    TEST_EQUALS(chunk_id_from_memory(mem.memory, mem.memory + 16, mem.chunk_size), 1);
    TEST_EQUALS(chunk_id_from_memory(mem.memory, mem.memory + 32, mem.chunk_size), 2);
    TEST_EQUALS(chunk_id_from_memory(mem.memory, mem.memory + 47, mem.chunk_size), 2);
    TEST_EQUALS(chunk_id_from_memory(mem.memory, mem.memory + 48, mem.chunk_size), 3);

    chunk_free(&mem);
}

static void test_chunk_element_get() {
    ChunkMemory mem = {0};
    chunk_alloc(&mem, 10, 10, 10);

    TEST_EQUALS(chunk_element_get(&mem, 2), mem.memory + 32);

    // Out of bounds must return NULL instead of reading invalid memory
    TEST_EQUALS(chunk_element_get(&mem, 10), NULL);
    TEST_EQUALS(chunk_element_get(&mem, 100), NULL);

    chunk_free(&mem);
}

static void test_chunk_element_count() {
    ChunkMemory mem = {0};
    chunk_alloc(&mem, 10, 10, 10);

    // chunk_size = 16
    TEST_EQUALS(chunk_element_count(&mem, 0), 0);
    TEST_EQUALS(chunk_element_count(&mem, 1), 1);
    TEST_EQUALS(chunk_element_count(&mem, 16), 1);
    TEST_EQUALS(chunk_element_count(&mem, 17), 2);
    TEST_EQUALS(chunk_element_count(&mem, 32), 2);

    chunk_free(&mem);
}

static void test_chunk_reserve_one() {
    ChunkMemory mem = {0};
    chunk_alloc(&mem, 10, 10, 10);

    TEST_EQUALS(chunk_reserve_one(&mem), 0);
    TEST_EQUALS(chunk_reserve_one(&mem), 1);
    TEST_TRUE(IS_BIT_SET_64_R2L(*mem.free, 0));
    TEST_TRUE(IS_BIT_SET_64_R2L(*mem.free, 1));

    chunk_free(&mem);
}

static void test_chunk_reserve() {
    ChunkMemory mem = {0};
    chunk_alloc(&mem, 10, 10, 10);

    TEST_EQUALS(chunk_reserve(&mem, 1), 0);
    TEST_EQUALS(chunk_reserve(&mem, 1), 1);
    TEST_TRUE(IS_BIT_SET_64_R2L(*mem.free, 0));
    TEST_TRUE(IS_BIT_SET_64_R2L(*mem.free, 1));
    TEST_FALSE(IS_BIT_SET_64_R2L(*mem.free, 2));

    TEST_EQUALS(chunk_reserve(&mem, 2), 2);
    TEST_TRUE(IS_BIT_SET_64_R2L(*mem.free, 2));
    TEST_TRUE(IS_BIT_SET_64_R2L(*mem.free, 3));

    chunk_free(&mem);
}

static void test_chunk_is_free() {
    ChunkMemory mem = {0};
    chunk_alloc(&mem, 10, 10, 10);

    TEST_TRUE(chunk_is_free(&mem, 0));

    chunk_reserve_one(&mem);
    TEST_FALSE(chunk_is_free(&mem, 0));
    TEST_TRUE(chunk_is_free(&mem, 1));

    chunk_free(&mem);
}

static void test_chunk_free_element() {
    ChunkMemory mem = {0};
    chunk_alloc(&mem, 10, 10, 10);

    // By id
    chunk_reserve_one(&mem);
    chunk_reserve_one(&mem);
    TEST_FALSE(chunk_is_free(&mem, 0));

    chunk_free_element(&mem, 0);
    TEST_TRUE(chunk_is_free(&mem, 0));
    TEST_FALSE(chunk_is_free(&mem, 1));

    // By word/bit index
    chunk_free_element(&mem, (size_t) 0, 1);
    TEST_TRUE(chunk_is_free(&mem, 1));

    // By memory position
    chunk_reserve_one(&mem);
    TEST_FALSE(chunk_is_free(&mem, 0));

    chunk_free_elements(&mem, mem.memory, 1);
    TEST_TRUE(chunk_is_free(&mem, 0));

    chunk_free(&mem);
}

static void test_chunk_free_elements() {
    ChunkMemory mem = {0};
    chunk_alloc(&mem, 10, 10, 10);

    TEST_EQUALS(chunk_reserve(&mem, 3), 0);

    chunk_free_elements(&mem, 0, 2);
    TEST_FALSE(IS_BIT_SET_64_R2L(*mem.free, 0));
    TEST_FALSE(IS_BIT_SET_64_R2L(*mem.free, 1));
    TEST_TRUE(IS_BIT_SET_64_R2L(*mem.free, 2));

    chunk_free_elements(&mem, 2, 1);
    TEST_FALSE(IS_BIT_SET_64_R2L(*mem.free, 2));

    chunk_free_elements(&mem, 0, 10);
    TEST_EQUALS(*mem.free, 0);

    chunk_free(&mem);
}

// To ensure there is no logical error we test memory wrapping specifically
static void test_chunk_reserve_wrapping() {
    ChunkMemory mem = {0};
    chunk_alloc(&mem, 10, 10, 10);

    mem.last_pos = 7;
    TEST_EQUALS(chunk_reserve(&mem, 5), 0);
    TEST_EQUALS((byte) BITS_GET_8_R2L(*mem.free, 0, 8), (byte) 0b00011111);

    chunk_free(&mem);
}

// To ensure there is no logical error we test the last element specifically
static void test_chunk_reserve_last_element() {
    ChunkMemory mem = {0};
    chunk_alloc(&mem, 10, 10, 10);

    // Get last element when the last_pos is the previous element
    mem.last_pos = 8;
    TEST_EQUALS(chunk_reserve(&mem, 1), 9);

    // Get last element when the last element is not defined and all other elements are in use
    mem.last_pos = -1;
    *mem.free = 511;
    TEST_EQUALS(chunk_reserve(&mem, 1), 9);

    chunk_free(&mem);
}

static void test_chunk_memory_get() {
    ChunkMemory mem = {0};
    chunk_alloc(&mem, 10, 10, 10);

    // 1 element
    byte* a = memory_get(&mem, 1);
    TEST_EQUALS(a, mem.memory);
    TEST_FALSE(chunk_is_free(&mem, 0));

    // 2 elements (chunk_size = 16 -> 17 bytes need 2 chunks)
    byte* b = memory_get(&mem, 17);
    TEST_EQUALS(b, mem.memory + 16);
    TEST_FALSE(chunk_is_free(&mem, 1));
    TEST_FALSE(chunk_is_free(&mem, 2));

    // Temporary memory is free again right away
    byte* c = memory_get_temp(&mem, 16);
    TEST_EQUALS(c, mem.memory + 48);
    TEST_TRUE(chunk_is_free(&mem, 3));

    chunk_free(&mem);
}

// Ensure the bit iterator visits exactly the reserved elements
static void test_chunk_iterate() {
    ChunkMemory mem = {0};
    chunk_alloc(&mem, 10, 10, 10);

    chunk_free_element(&mem, 0, 0);
    chunk_free_element(&mem, 0, 1);
    chunk_free_element(&mem, 0, 2);
    chunk_free_element(&mem, 0, 3);
    chunk_free_element(&mem, 0, 4);
    chunk_free_element(&mem, 0, 5);
    chunk_free_element(&mem, 0, 6);
    chunk_free_element(&mem, 0, 7);
    chunk_free_element(&mem, 0, 8);
    chunk_free_element(&mem, 0, 9);

    chunk_reserve_one(&mem); // 0
    chunk_reserve_one(&mem); // 1
    chunk_reserve_one(&mem); // 2
    chunk_reserve_one(&mem); // 3
    chunk_reserve_one(&mem); // 4
    chunk_reserve_one(&mem); // 5
    chunk_reserve_one(&mem); // 6
    chunk_reserve_one(&mem); // 7
    chunk_reserve_one(&mem); // 8
    chunk_reserve_one(&mem); // 9
    chunk_free_element(&mem, 0);
    chunk_free_element(&mem, 3);
    chunk_free_element(&mem, 9);

    int32 visited_count = 0;
    int32 expected[] = {1, 2, 4, 5, 6, 7, 8};
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

static void test_chunk_dump_load() {
    ChunkMemory mem = {0};
    chunk_alloc(&mem, 10, 10, 10);

    uint32* a = (uint32 *) chunk_element_get(&mem, chunk_reserve_one(&mem));
    uint32* b = (uint32 *) chunk_element_get(&mem, chunk_reserve_one(&mem));

    *a = 5;
    *b = 10;

    byte test_out[1024];
    const int64 dump_size = chunk_dump(&mem, test_out);

    ChunkMemory mem2 = {0};
    chunk_alloc(&mem2, 10, 10, 10);
    const int64 load_size = chunk_load(&mem2, test_out);

    const uint32* c = (uint32 *) chunk_element_get(&mem2, 0);
    const uint32* d = (uint32 *) chunk_element_get(&mem2, 1);

    TEST_EQUALS(dump_size, load_size);
    TEST_EQUALS(mem.size, mem2.size);
    TEST_EQUALS(mem.capacity, mem2.capacity);
    TEST_EQUALS(mem.chunk_size, mem2.chunk_size);
    TEST_EQUALS(*a, *c);
    TEST_EQUALS(*b, *d);

    chunk_free(&mem);
    chunk_free(&mem2);
}

#if !DEBUG
    static void test_chunk_reserve_full() {
        ChunkMemory mem = {0};
        chunk_alloc(&mem, 10, 10, 10);
        mem.free[0] = 0xFFFFFFFFFFFFFFFF;

        TEST_EQUALS(chunk_reserve(&mem, 1), -1);
    }

    static void test_chunk_reserve_invalid_size() {
        ChunkMemory mem = {0};
        chunk_alloc(&mem, 10, 10, 10);

        TEST_EQUALS(chunk_reserve(&mem, 11), -1);

        chunk_free(&mem);
    }
#endif

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main MemoryChunkMemoryTest
#endif

int main() {
    TEST_INIT(25);

    TEST_RUN(test_chunk_alloc);
    TEST_RUN(test_chunk_size_alignment);
    TEST_RUN(test_chunk_id_from_memory);
    TEST_RUN(test_chunk_element_get);
    TEST_RUN(test_chunk_element_count);
    TEST_RUN(test_chunk_reserve_one);
    TEST_RUN(test_chunk_reserve);
    TEST_RUN(test_chunk_is_free);
    TEST_RUN(test_chunk_free_element);
    TEST_RUN(test_chunk_free_elements);
    TEST_RUN(test_chunk_reserve_wrapping);
    TEST_RUN(test_chunk_reserve_last_element);
    TEST_RUN(test_chunk_memory_get);
    TEST_RUN(test_chunk_iterate);
    TEST_RUN(test_chunk_dump_load);

    #if !DEBUG
        TEST_RUN(test_chunk_reserve_full);
        TEST_RUN(test_chunk_reserve_invalid_size);
    #endif

    TEST_FINALIZE();

    return 0;
}

#include "../TestFramework.h"
#include "../../memory/ChunkMemory.h"

static void test_chunk_alloc() {
    ChunkMemory mem = {};
    chunk_alloc(&mem, 10, 10);

    ASSERT_TRUE(memcmp(mem.memory, mem.memory + 1, 10 * 10) == 0);
    ASSERT_EQUALS(mem.size, 10 * 64 + 8 * 1); // chunks are aligned to 64
    ASSERT_EQUALS(*mem.free, 0);

    chunk_free(&mem);
    ASSERT_EQUALS(mem.size, 0);
    ASSERT_EQUALS(mem.memory, NULL);
}

static void test_chunk_id_from_memory() {
    ChunkMemory mem = {};
    chunk_alloc(&mem, 10, 10);

    ASSERT_EQUALS(chunk_id_from_memory(&mem, mem.memory), 0);
    ASSERT_EQUALS(chunk_id_from_memory(&mem, mem.memory + 64), 1);
    ASSERT_EQUALS(chunk_id_from_memory(&mem, mem.memory + 127), 1);
    ASSERT_EQUALS(chunk_id_from_memory(&mem, mem.memory + 128), 2);

    chunk_free(&mem);
}

static void test_chunk_get_element() {
    ChunkMemory mem = {};
    chunk_alloc(&mem, 10, 10);

    ASSERT_EQUALS(chunk_get_element(&mem, 2), mem.memory + 128);

    chunk_free(&mem);
}

static void test_chunk_reserve() {
    ChunkMemory mem = {};
    chunk_alloc(&mem, 10, 10);

    ASSERT_EQUALS(chunk_reserve(&mem, 1), 0);
    ASSERT_EQUALS(chunk_reserve(&mem, 1), 1);
    ASSERT_TRUE(IS_BIT_SET_64_R2L(*mem.free, 0));
    ASSERT_TRUE(IS_BIT_SET_64_R2L(*mem.free, 1));
    ASSERT_FALSE(IS_BIT_SET_64_R2L(*mem.free, 2));

    ASSERT_EQUALS(chunk_reserve(&mem, 2), 2);
    ASSERT_TRUE(IS_BIT_SET_64_R2L(*mem.free, 2));
    ASSERT_TRUE(IS_BIT_SET_64_R2L(*mem.free, 3));

    chunk_free(&mem);
}

static void test_chunk_free_elements() {
    ChunkMemory mem = {};
    chunk_alloc(&mem, 10, 10);

    ASSERT_EQUALS(chunk_reserve(&mem, 3), 0);

    chunk_free_elements(&mem, 0, 2);
    ASSERT_FALSE(IS_BIT_SET_64_R2L(*mem.free, 0));
    ASSERT_FALSE(IS_BIT_SET_64_R2L(*mem.free, 1));
    ASSERT_TRUE(IS_BIT_SET_64_R2L(*mem.free, 2));

    chunk_free_elements(&mem, 2, 1);
    ASSERT_FALSE(IS_BIT_SET_64_R2L(*mem.free, 2));

    chunk_free_elements(&mem, 0, 10);
    ASSERT_EQUALS(*mem.free, 0);

    chunk_free(&mem);
}

// To ensure there is no logical error we test memory wrapping specifically
static void test_chunk_reserve_wrapping() {
    ChunkMemory mem = {};
    chunk_alloc(&mem, 10, 10);

    mem.last_pos = 7;
    ASSERT_EQUALS(chunk_reserve(&mem, 5), 0);
    ASSERT_EQUALS((byte) BITS_GET_8_R2L(*mem.free, 0, 8), (byte) 0b00011111);

    chunk_free(&mem);
}

// To ensure there is no logical error we test the last element specifically
static void test_chunk_reserve_last_element() {
    ChunkMemory mem = {};
    chunk_alloc(&mem, 10, 10);

    // Get last element when the last_pos is the previous element
    mem.last_pos = 8;
    ASSERT_EQUALS(chunk_reserve(&mem, 1), 9);

    // Get last element when the last element is not defined and all other elements are in use
    mem.last_pos = -1;
    *mem.free = 511;
    ASSERT_EQUALS(chunk_reserve(&mem, 1), 9);

    chunk_free(&mem);
}

#if !DEBUG
    static void test_chunk_reserve_full() {
        ChunkMemory mem = {};
        chunk_alloc(&mem, 10, 10);
        mem.free[0] = 0xFFFFFFFFFFFFFFFF;

        ASSERT_EQUALS(chunk_reserve(&mem, 1), -1);
    }

    static void test_chunk_reserve_invalid_size() {
        ChunkMemory mem = {};
        chunk_alloc(&mem, 10, 10);

        ASSERT_EQUALS(chunk_reserve(&mem, 11), -1);

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
    TEST_RUN(test_chunk_id_from_memory);
    TEST_RUN(test_chunk_get_element);
    TEST_RUN(test_chunk_reserve);
    TEST_RUN(test_chunk_free_elements);
    TEST_RUN(test_chunk_reserve_wrapping);
    TEST_RUN(test_chunk_reserve_last_element);

    #if !DEBUG
        TEST_RUN(test_chunk_reserve_full);
        TEST_RUN(test_chunk_reserve_invalid_size);
    #endif

    TEST_FINALIZE();

    return 0;
}

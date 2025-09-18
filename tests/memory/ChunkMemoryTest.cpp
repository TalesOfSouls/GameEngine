#include "../TestFramework.h"
#include "../../memory/ChunkMemory.h"

static void test_chunk_alloc() {
    ChunkMemory mem = {};
    chunk_alloc(&mem, 10, 10);

    TEST_TRUE(memcmp(mem.memory, mem.memory + 1, 10 * 10) == 0);
    TEST_EQUALS((uintptr_t) mem.free, (uintptr_t) mem.memory + 10 * 32);
    TEST_EQUALS(*mem.free, 0);

    chunk_free(&mem);
    TEST_EQUALS(mem.size, 0);
    TEST_EQUALS(mem.memory, NULL);
}

static void test_chunk_id_from_memory() {
    ChunkMemory mem = {};
    chunk_alloc(&mem, 10, 10);

    TEST_EQUALS(chunk_id_from_memory(&mem, mem.memory), 0);
    TEST_EQUALS(chunk_id_from_memory(&mem, mem.memory + 32), 1);
    TEST_EQUALS(chunk_id_from_memory(&mem, mem.memory + 64), 2);
    TEST_EQUALS(chunk_id_from_memory(&mem, mem.memory + 95), 2);
    TEST_EQUALS(chunk_id_from_memory(&mem, mem.memory + 96), 3);

    chunk_free(&mem);
}

static void test_chunk_get_element() {
    ChunkMemory mem = {};
    chunk_alloc(&mem, 10, 10);

    TEST_EQUALS(chunk_get_element(&mem, 2), mem.memory + 64);

    chunk_free(&mem);
}

static void test_chunk_reserve_one() {
    ChunkMemory mem = {};
    chunk_alloc(&mem, 10, 10);

    TEST_EQUALS(chunk_reserve_one(&mem), 0);
    TEST_EQUALS(chunk_reserve_one(&mem), 1);
    TEST_TRUE(IS_BIT_SET_64_R2L(*mem.free, 0));
    TEST_TRUE(IS_BIT_SET_64_R2L(*mem.free, 1));

    chunk_free(&mem);
}

static void test_chunk_reserve() {
    ChunkMemory mem = {};
    chunk_alloc(&mem, 10, 10);

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

static void test_chunk_free_elements() {
    ChunkMemory mem = {};
    chunk_alloc(&mem, 10, 10);

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
    ChunkMemory mem = {};
    chunk_alloc(&mem, 10, 10);

    mem.last_pos = 7;
    TEST_EQUALS(chunk_reserve(&mem, 5), 0);
    TEST_EQUALS((byte) BITS_GET_8_R2L(*mem.free, 0, 8), (byte) 0b00011111);

    chunk_free(&mem);
}

// To ensure there is no logical error we test the last element specifically
static void test_chunk_reserve_last_element() {
    ChunkMemory mem = {};
    chunk_alloc(&mem, 10, 10);

    // Get last element when the last_pos is the previous element
    mem.last_pos = 8;
    TEST_EQUALS(chunk_reserve(&mem, 1), 9);

    // Get last element when the last element is not defined and all other elements are in use
    mem.last_pos = -1;
    *mem.free = 511;
    TEST_EQUALS(chunk_reserve(&mem, 1), 9);

    chunk_free(&mem);
}

static void test_chunk_dump_load() {
    ChunkMemory mem = {};
    chunk_alloc(&mem, 10, 10);

    uint32* a = (uint32 *) chunk_get_element(&mem, chunk_reserve_one(&mem));
    uint32* b = (uint32 *) chunk_get_element(&mem, chunk_reserve_one(&mem));

    *a = 5;
    *b = 10;

    byte test_out[1024];
    chunk_dump(&mem, test_out);

    ChunkMemory mem2 = {};
    chunk_alloc(&mem2, 10, 10);
    chunk_load(&mem2, test_out);

    uint32* c = (uint32 *) chunk_get_element(&mem2, 0);
    uint32* d = (uint32 *) chunk_get_element(&mem2, 1);

    TEST_EQUALS(mem.size, mem2.size);
    TEST_EQUALS(*a, *c);
    TEST_EQUALS(*b, *d);

    chunk_free(&mem);
    chunk_free(&mem2);
}

#if !DEBUG
    static void test_chunk_reserve_full() {
        ChunkMemory mem = {};
        chunk_alloc(&mem, 10, 10);
        mem.free[0] = 0xFFFFFFFFFFFFFFFF;

        TEST_EQUALS(chunk_reserve(&mem, 1), -1);
    }

    static void test_chunk_reserve_invalid_size() {
        ChunkMemory mem = {};
        chunk_alloc(&mem, 10, 10);

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
    TEST_RUN(test_chunk_id_from_memory);
    TEST_RUN(test_chunk_get_element);
    TEST_RUN(test_chunk_reserve_one);
    TEST_RUN(test_chunk_reserve);
    TEST_RUN(test_chunk_free_elements);
    TEST_RUN(test_chunk_reserve_wrapping);
    TEST_RUN(test_chunk_reserve_last_element);
    TEST_RUN(test_chunk_dump_load);

    #if !DEBUG
        TEST_RUN(test_chunk_reserve_full);
        TEST_RUN(test_chunk_reserve_invalid_size);
    #endif

    TEST_FINALIZE();

    return 0;
}

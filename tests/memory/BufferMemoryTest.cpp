#include "../TestFramework.h"
#include "../../memory/BufferMemory.cpp"

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

static void test_buffer_init() {
    // The buffer is aligned inside of the provided memory -> needs slack for the alignment
    // The caller has to provide zeroed memory (@see ASSERT_MEM_ZERO)
    byte buffer[256] = {0};
    BufferMemory mem = {0};
    buffer_init(&mem, buffer, 100);

    TEST_EQUALS(mem.size, 100);
    TEST_EQUALS(mem.head, mem.memory);
    TEST_EQUALS(mem.end, mem.memory + mem.size);

    // The memory is zero initialized
    byte* zero_check = mem.memory;
    bool is_zero = true;
    for (uint32 i = 0; i < 100; ++i) {
        is_zero = is_zero && zero_check[i] == 0;
    }
    TEST_TRUE(is_zero);

    // Never frees memory that it doesn't own
    mem.memory = NULL;
    mem.size = 0;
}

static void test_buffer_memory_get() {
    BufferMemory mem = {0};
    buffer_alloc(&mem, 50, 50);

    TEST_EQUALS(memory_get(&mem, 20), mem.memory);
    TEST_EQUALS(mem.head, mem.memory + 24);

    buffer_free(&mem);
}

static void test_buffer_memory_get_alignment() {
    BufferMemory mem = {0};
    buffer_alloc(&mem, 64, 64);

    // 3 bytes with an alignment of 8 -> head is moved to the next multiple of 8
    TEST_EQUALS(memory_get(&mem, 3), mem.memory);
    TEST_EQUALS(mem.head, mem.memory + 8);

    TEST_EQUALS(memory_get(&mem, 1), mem.memory + 8);
    TEST_EQUALS(mem.head, mem.memory + 16);

    buffer_free(&mem);
}

static void test_buffer_memory_get_temp() {
    BufferMemory mem = {0};
    buffer_alloc(&mem, 64, 64);

    TEST_EQUALS(memory_get_temp(&mem, 16), mem.memory);
    TEST_EQUALS(mem.head, mem.memory);

    // A subsequent request overrides the temporary memory
    TEST_EQUALS(memory_get(&mem, 16), mem.memory);
    TEST_EQUALS(mem.head, mem.memory + 16);

    buffer_free(&mem);
}

static void test_buffer_rewind() {
    BufferMemory mem = {0};
    buffer_alloc(&mem, 64, 64);

    byte* const old_head = mem.head;

    memory_get(&mem, 16);
    memory_get(&mem, 16);
    TEST_EQUALS(mem.head, mem.memory + 32);

    memory_rewind(&mem, old_head);
    TEST_EQUALS(mem.head, mem.memory);

    buffer_free(&mem);
}

static void test_buffer_reset() {
    BufferMemory mem = {0};
    buffer_alloc(&mem, 50, 50);

    memory_get(&mem, 20);
    TEST_NOT_EQUALS(mem.head, mem.memory);

    buffer_reset(&mem);
    TEST_EQUALS(mem.head, mem.memory);

    buffer_free(&mem);
}

static void test_buffer_stack_memory() {
    BufferMemory mem = {0};
    buffer_alloc(&mem, 128, 128);

    byte* a = NULL;
    {
        BUFFER_STACK_MEMORY(&mem, &a, 32, sizeof(size_t));
        TEST_NOT_EQUALS(a, NULL);
        TEST_EQUALS(mem.head, mem.memory + 32);

        byte* b = NULL;
        {
            BUFFER_STACK_MEMORY(&mem, &b, 32, sizeof(size_t));
            TEST_EQUALS(b, mem.memory + 32);
            TEST_EQUALS(mem.head, mem.memory + 64);
        }

        // Inner stack memory is released
        TEST_EQUALS(mem.head, mem.memory + 32);
    }

    // Outer stack memory is released
    TEST_EQUALS(mem.head, mem.memory);

    buffer_free(&mem);
}

static void test_buffer_dump_load() {
    BufferMemory mem = {0};
    buffer_alloc(&mem, 64, 64);

    byte* const data = memory_get(&mem, 12);
    memcpy(data, "12345678901", 12);

    byte out[256];
    const int64 dump_size = buffer_dump(&mem, out);

    BufferMemory mem2 = {0};
    buffer_alloc(&mem2, 64, 64);
    const int64 load_size = buffer_load(&mem2, out);

    TEST_EQUALS(dump_size, load_size);
    TEST_EQUALS(mem.size, mem2.size);
    TEST_EQUALS(mem2.head, mem2.memory + 16);
    TEST_MEMORY_EQUALS(mem.memory, mem2.memory, mem.size);

    buffer_free(&mem);
    buffer_free(&mem2);
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
    TEST_RUN(test_buffer_init);
    TEST_RUN(test_buffer_memory_get);
    TEST_RUN(test_buffer_memory_get_alignment);
    TEST_RUN(test_buffer_memory_get_temp);
    TEST_RUN(test_buffer_rewind);
    TEST_RUN(test_buffer_reset);
    TEST_RUN(test_buffer_stack_memory);
    TEST_RUN(test_buffer_dump_load);

    TEST_FINALIZE();

    return 0;
}

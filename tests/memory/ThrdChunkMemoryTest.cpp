#include "../TestFramework.h"
#include "../../memory/ThrdChunkMemory.cpp"

/**
 * Tests for memory/ThrdChunkMemory.cpp/.h
 *
 * Thread safe ChunkMemory: the free bitmap is atomic and there is an additional
 * completeness bitmap which marks elements that are fully written (important for
 * lockless content hand over). All tests are single threaded, they verify the
 * data structure semantics (reserve/claim/free/complete), not real concurrency.
 */
static void test_thrd_chunk_size() {
    // max_capacity * type_size + 2 bitmaps + 2 size_t overhead
    TEST_EQUALS(thrd_chunk_size(4, 64), 64 * 4 + sizeof(size_t) * 1 + sizeof(size_t) * 1 + sizeof(size_t) * 2);
    TEST_EQUALS(thrd_chunk_size(16, 8), 8 * 16 + sizeof(size_t) * 1 + sizeof(size_t) * 1 + sizeof(size_t) * 2);
    TEST_EQUALS(thrd_chunk_size(4, 128), 128 * 4 + sizeof(size_t) * 2 * 2 + sizeof(size_t) * 2);

    // The chunk size is aligned up, a chunk could otherwise start unaligned
    TEST_EQUALS(thrd_chunk_size_element(14, 8), 16);
    TEST_EQUALS(thrd_chunk_size_element(16, 8), 16);
    TEST_EQUALS(thrd_chunk_size_element(1, 8), 8);
    TEST_EQUALS(thrd_chunk_size_element(8, 4), 8);
    TEST_EQUALS(thrd_chunk_size_element(0, 8), 0);
}

static void test_thrd_chunk_alloc() {
    ThrdChunkMemory mem = {0};
    chunk_alloc(&mem, 8, 16, 16);

    TEST_NOT_EQUALS(mem.memory, NULL);
    TEST_NOT_EQUALS(mem.free, NULL);
    TEST_NOT_EQUALS(mem.completeness, NULL);
    TEST_EQUALS(mem.capacity, 8);
    TEST_EQUALS(mem.chunk_size, 16);
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), -1);
    TEST_EQUALS((uintptr_t) mem.size, (uintptr_t) (8 * 16 + sizeof(size_t) * 2 + sizeof(size_t) * 2));

    // Both bitmaps live behind the element memory
    TEST_EQUALS(mem.free, (atomic<size_t> *) ALIGN_UP((uintptr_t) (mem.memory + 8 * 16), alignof(size_t)));
    TEST_EQUALS(mem.completeness, mem.free + 1);

    // The requested alignment is sizeof(size_t) for chunk_alloc
    // (@see bug report: ThrdChunkMemory.alignment is never set by chunk_init)
    TEST_EQUALS((uintptr_t) mem.memory % sizeof(size_t), 0);

    // All elements are free and none is complete
    bool all_free = true;
    bool none_complete = true;
    for (int32 i = 0; i < 8; ++i) {
        all_free = all_free && chunk_is_free(&mem, (uint32) i);
        none_complete = none_complete && !chunk_is_complete(&mem, (uint32) i);
    }
    TEST_TRUE(all_free);
    TEST_TRUE(none_complete);

    chunk_free(&mem);
    TEST_EQUALS(mem.memory, NULL);
    TEST_EQUALS(mem.capacity, 0);
}

static void test_thrd_chunk_init() {
    // chunk_init doesn't zero the bitmaps, the caller has to provide clean memory
    alignas(64) byte raw[256] = {0};

    ThrdChunkMemory mem = {0};
    chunk_init(&mem, raw, 4, 16);

    TEST_EQUALS(mem.memory, raw);
    TEST_EQUALS(mem.capacity, 4);
    TEST_EQUALS(mem.chunk_size, 16);
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), -1);
    TEST_TRUE(chunk_is_free(&mem, 0));
    TEST_TRUE(chunk_is_free(&mem, 3));

    // The element size is aligned up by chunk_init
    ThrdChunkMemory aligned = {0};
    chunk_init(&aligned, raw, 4, 10);
    TEST_EQUALS(aligned.chunk_size, 16);

    // The memory starts at the requested alignment
    ThrdChunkMemory offset = {0};
    chunk_init(&offset, raw + 1, 4, 16);
    TEST_EQUALS(offset.memory, raw + 8);
}

static void test_thrd_chunk_element_get() {
    ThrdChunkMemory mem = {0};
    chunk_alloc(&mem, 8, 8, 16);

    TEST_EQUALS(chunk_element_get(&mem, 0), mem.memory);
    TEST_EQUALS(chunk_element_get(&mem, 3), mem.memory + 3 * 16);
    TEST_EQUALS(chunk_element_get(&mem, 7), mem.memory + 7 * 16);

    // Out of bounds
    TEST_EQUALS(chunk_element_get(&mem, 8), NULL);
    TEST_EQUALS(chunk_element_get(&mem, 1000), NULL);

    chunk_free(&mem);
}

static void test_thrd_chunk_reserve_one() {
    ThrdChunkMemory mem = {0};
    chunk_alloc(&mem, 8, 8, 16);

    // Elements are reserved in ascending order
    TEST_EQUALS(chunk_reserve_one(&mem), 0);
    TEST_FALSE(chunk_is_free(&mem, 0));
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), 0);

    TEST_EQUALS(chunk_reserve_one(&mem), 1);
    TEST_EQUALS(chunk_reserve_one(&mem), 2);
    TEST_FALSE(chunk_is_free(&mem, 2));
    TEST_TRUE(chunk_is_free(&mem, 3));
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), 2);

    chunk_free(&mem);
}

static void test_thrd_chunk_reserve_range() {
    ThrdChunkMemory mem = {0};
    chunk_alloc(&mem, 8, 8, 16);

    // A contiguous range in a fresh buffer starts at the first element
    TEST_EQUALS(chunk_reserve(&mem, 3), 0);
    TEST_FALSE(chunk_is_free(&mem, 0));
    TEST_FALSE(chunk_is_free(&mem, 1));
    TEST_FALSE(chunk_is_free(&mem, 2));
    TEST_TRUE(chunk_is_free(&mem, 3));
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), 2);

    // The next range starts behind the previous one
    TEST_EQUALS(chunk_reserve(&mem, 2), 3);
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), 4);

    // A single element behaves like chunk_reserve_one
    TEST_EQUALS(chunk_reserve(&mem, 1), 5);
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), 5);

    chunk_free(&mem);
}

static void test_thrd_chunk_reserve_boundary() {
    ThrdChunkMemory mem = {0};
    chunk_alloc(&mem, 8, 8, 16);

    // The whole buffer can be reserved at once
    TEST_EQUALS(chunk_reserve(&mem, 8), 0);
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), 7);

    bool all_used = true;
    for (int32 i = 0; i < 8; ++i) {
        all_used = all_used && !chunk_is_free(&mem, (uint32) i);
    }
    TEST_TRUE(all_used);

    // A full buffer has no free element left
    TEST_EQUALS(chunk_reserve_one(&mem), -1);

    // Freeing a hole in the middle makes the space usable again
    chunk_free_element(&mem, (uint32) 3);
    TEST_TRUE(chunk_is_free(&mem, 3));
    TEST_EQUALS(chunk_reserve_one(&mem), 3);

    chunk_free(&mem);
}

static void test_thrd_chunk_multi_word() {
    // 65 elements need two bitmap words
    ThrdChunkMemory mem = {0};
    chunk_alloc(&mem, 65, 65, 8);

    int32 last = -1;
    bool ascending = true;
    for (int32 i = 0; i < 65; ++i) {
        const int32 id = chunk_reserve_one(&mem);
        ascending = ascending && id == i;
        last = id;
    }

    TEST_TRUE(ascending);
    TEST_EQUALS(last, 64);
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), 64);

    // The second bitmap word is in use as well
    TEST_FALSE(chunk_is_free(&mem, 64));
    TEST_EQUALS(chunk_reserve_one(&mem), -1);

    // Free every second element and reserve them again
    for (int32 i = 0; i < 65; i += 2) {
        chunk_free_element(&mem, (uint32) i);
    }

    TEST_TRUE(chunk_is_free(&mem, 0));
    TEST_TRUE(chunk_is_free(&mem, 64));
    TEST_FALSE(chunk_is_free(&mem, 1));

    // The search starts at last_pos, so the last freed element is found first
    const int32 claimed = chunk_reserve_one(&mem);
    TEST_EQUALS(claimed, 64);

    // A subsequent search wraps around to the beginning of the bitmap
    TEST_EQUALS(chunk_reserve_one(&mem), 0);

    chunk_free(&mem);
}

static void test_thrd_chunk_free_element() {
    ThrdChunkMemory mem = {0};
    chunk_alloc(&mem, 8, 8, 16);

    TEST_EQUALS(chunk_reserve(&mem, 3), 0);
    TEST_FALSE(chunk_is_free(&mem, 0));
    TEST_FALSE(chunk_is_free(&mem, 1));
    TEST_FALSE(chunk_is_free(&mem, 2));

    // By element id
    chunk_free_element(&mem, (uint32) 0);
    TEST_TRUE(chunk_is_free(&mem, 0));
    TEST_FALSE(chunk_is_free(&mem, 1));

    // By bitmap position (free_index, bit_index)
    chunk_free_element(&mem, (size_t) 0, 1);
    TEST_TRUE(chunk_is_free(&mem, 1));
    TEST_FALSE(chunk_is_free(&mem, 2));

    // By data pointer
    chunk_free_element(&mem, chunk_element_get(&mem, 2));
    TEST_TRUE(chunk_is_free(&mem, 2));

    // The next reservation reuses the freed space
    TEST_EQUALS(chunk_reserve_one(&mem), 2);
    TEST_FALSE(chunk_is_free(&mem, 2));

    chunk_free(&mem);
}

static void test_thrd_chunk_free_elements() {
    ThrdChunkMemory mem = {0};
    chunk_alloc(&mem, 8, 8, 16);

    TEST_EQUALS(chunk_reserve(&mem, 4), 0);

    // By element id + count
    chunk_free_elements(&mem, 1, 2);
    TEST_FALSE(chunk_is_free(&mem, 0));
    TEST_TRUE(chunk_is_free(&mem, 1));
    TEST_TRUE(chunk_is_free(&mem, 2));
    TEST_FALSE(chunk_is_free(&mem, 3));

    // By data pointer + count
    chunk_free_elements(&mem, chunk_element_get(&mem, 3), 1);
    TEST_TRUE(chunk_is_free(&mem, 3));

    chunk_free(&mem);
}

static void test_thrd_chunk_completeness() {
    ThrdChunkMemory mem = {0};
    chunk_alloc(&mem, 8, 8, 16);

    chunk_reserve(&mem, 3);

    // Nothing is complete right after reserving
    for (int32 i = 0; i < 3; ++i) {
        TEST_FALSE(chunk_is_complete(&mem, (uint32) i));
    }

    chunk_mark_complete(&mem, (uint32) 0);
    chunk_mark_complete(&mem, chunk_element_get(&mem, 2));

    TEST_TRUE(chunk_is_complete(&mem, (uint32) 0));
    TEST_FALSE(chunk_is_complete(&mem, (uint32) 1));
    TEST_TRUE(chunk_is_complete(&mem, (uint32) 2));
    TEST_TRUE(chunk_is_complete(&mem, chunk_element_get(&mem, 0)));

    // Freeing an element clears its completeness
    chunk_free_element(&mem, (uint32) 0);
    TEST_FALSE(chunk_is_complete(&mem, (uint32) 0));

    // The completeness can be cleared explicitly
    chunk_clear_complete(&mem, (uint32) 2);
    TEST_FALSE(chunk_is_complete(&mem, (uint32) 2));

    chunk_free(&mem);
}

static void test_thrd_chunk_element_count() {
    ThrdChunkMemory mem = {0};
    chunk_alloc(&mem, 8, 8, 16);

    TEST_EQUALS(chunk_element_count(&mem, 0), 0);
    TEST_EQUALS(chunk_element_count(&mem, 1), 1);
    TEST_EQUALS(chunk_element_count(&mem, 16), 1);
    TEST_EQUALS(chunk_element_count(&mem, 17), 2);
    TEST_EQUALS(chunk_element_count(&mem, 32), 2);
    TEST_EQUALS(chunk_element_count(&mem, 128), 8);

    chunk_free(&mem);
}

static void test_thrd_chunk_memory_get() {
    ThrdChunkMemory mem = {0};
    chunk_alloc(&mem, 8, 8, 16);

    // One element request returns the first free chunk
    TEST_EQUALS(memory_get(&mem, 16), mem.memory);
    TEST_FALSE(chunk_is_free(&mem, 0));

    // Two element request returns the next contiguous range
    TEST_EQUALS(memory_get(&mem, 32), mem.memory + 16);
    TEST_FALSE(chunk_is_free(&mem, 1));
    TEST_FALSE(chunk_is_free(&mem, 2));

    // chunk_memory_get_one works on single elements
    TEST_EQUALS(chunk_memory_get_one(&mem), mem.memory + 3 * 16);
    TEST_FALSE(chunk_is_free(&mem, 3));

    // Temporary memory is free again right away
    byte* temp = memory_get_temp(&mem, 32);
    TEST_EQUALS(temp, mem.memory + 4 * 16);
    TEST_TRUE(chunk_is_free(&mem, 4));
    TEST_TRUE(chunk_is_free(&mem, 5));

    chunk_free(&mem);
}

static void test_thrd_chunk_dump_load() {
    ThrdChunkMemory src = {0};
    chunk_alloc(&src, 8, 8, 16);

    // Reserve some elements and write a recognizable pattern
    TEST_EQUALS(chunk_reserve(&src, 3), 0);
    for (int32 i = 0; i < 3; ++i) {
        memset(chunk_element_get(&src, i), i + 1, src.chunk_size);
    }

    byte out[1024] = {0};
    const int64 dump_size = chunk_dump(&src, out);

    // header (capacity, size, chunk_size, last_pos, free_offset) + element memory + bitmaps
    const int64 expected_size = sizeof(int32) + sizeof(size_t) + sizeof(int32) + sizeof(int32) + sizeof(uint32)
        + 8 * 16 + sizeof(size_t) * 1 + sizeof(size_t);
    TEST_EQUALS(dump_size, expected_size);

    ThrdChunkMemory dst = {0};
    chunk_alloc(&dst, 8, 8, 16);

    const int64 load_size = chunk_load(&dst, out);
    TEST_EQUALS(load_size, dump_size);

    TEST_EQUALS(dst.capacity, src.capacity);
    TEST_EQUALS(dst.chunk_size, src.chunk_size);
    TEST_EQUALS(dst.last_pos.load(memory_order_relaxed), src.last_pos.load(memory_order_relaxed));

    // The free bitmap and the element data are restored
    TEST_FALSE(chunk_is_free(&dst, 0));
    TEST_FALSE(chunk_is_free(&dst, 1));
    TEST_FALSE(chunk_is_free(&dst, 2));
    TEST_TRUE(chunk_is_free(&dst, 3));

    TEST_MEMORY_EQUALS(dst.memory, src.memory, 3 * 16);

    // The loaded buffer is usable
    TEST_EQUALS(chunk_reserve_one(&dst), 3);
    TEST_FALSE(chunk_is_free(&dst, 3));

    chunk_free(&src);
    chunk_free(&dst);
}

static void test_thrd_chunk_iterate() {
    ThrdChunkMemory mem = {0};
    chunk_alloc(&mem, 8, 8, 16);

    chunk_reserve(&mem, 4);

    // Only elements that are reserved AND complete are iterated
    chunk_mark_complete(&mem, (uint32) 1);
    chunk_mark_complete(&mem, (uint32) 3);

    int32 chunk_id = 0;
    const int32 expected[] = {1, 3};
    int32 expected_index = 0;
    int32 visited = 0;

    thrd_chunk_iterate_start(&mem, chunk_id) {
        TEST_EQUALS(chunk_id, expected[expected_index]);
        ++expected_index;
        ++visited;
    } thrd_chunk_iterate_end

    TEST_EQUALS(visited, ARRAY_COUNT(expected));

    chunk_free(&mem);
}

static void test_thrd_chunk_arena() {
    MemoryArena* root = mem_arena_alloc(8192, 8192);

    ThrdChunkMemory mem = {0};
    chunk_alloc(&mem, root, 8, 8, 16);

    TEST_NOT_EQUALS(mem.memory, NULL);
    TEST_EQUALS(mem.capacity, 8);
    TEST_EQUALS(mem_arena_stats(root).count, 2);

    // The arena backed chunk memory behaves like the platform backed one
    TEST_EQUALS(chunk_reserve_one(&mem), 0);
    TEST_EQUALS(chunk_element_get(&mem, 0), mem.memory);

    chunk_free(&mem, root);

    // The arena node is released with it
    TEST_EQUALS(mem.memory, NULL);
    TEST_EQUALS(mem.capacity, 0);
    TEST_EQUALS(mem_arena_stats(root).count, 1);

    void* base = root->base;
    platform_aligned_free(&base);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main MemoryThrdChunkMemoryTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_thrd_chunk_size);
    TEST_RUN(test_thrd_chunk_alloc);
    TEST_RUN(test_thrd_chunk_init);
    TEST_RUN(test_thrd_chunk_element_get);
    TEST_RUN(test_thrd_chunk_reserve_one);
    TEST_RUN(test_thrd_chunk_reserve_range);
    TEST_RUN(test_thrd_chunk_reserve_boundary);
    TEST_RUN(test_thrd_chunk_multi_word);
    TEST_RUN(test_thrd_chunk_free_element);
    TEST_RUN(test_thrd_chunk_free_elements);
    TEST_RUN(test_thrd_chunk_completeness);
    TEST_RUN(test_thrd_chunk_element_count);
    TEST_RUN(test_thrd_chunk_memory_get);
    TEST_RUN(test_thrd_chunk_dump_load);
    TEST_RUN(test_thrd_chunk_iterate);
    TEST_RUN(test_thrd_chunk_arena);

    TEST_FINALIZE();

    return 0;
}

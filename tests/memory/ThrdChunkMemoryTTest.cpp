#include "../TestFramework.h"
#include "../../memory/ThrdChunkMemoryT.cpp"

/**
 * Tests for memory/ThrdChunkMemoryT.cpp/.h
 *
 * Typed thread safe ChunkMemory: an atomic free bitmap, an atomic completeness bitmap
 * and typed element access. All tests are single threaded, they verify the data
 * structure semantics (reserve/claim/free/complete), not real concurrency.
 */

#include "../../memory/ThrdChunkMemoryT.cpp"

struct ThrdChunkTElement {
    int32 id;
    int32 value;
};

static void test_thrd_chunk_t_alloc() {
    ThrdChunkMemoryT<ThrdChunkTElement> mem = {0};
    chunk_alloc(&mem, 8, 16);

    TEST_NOT_EQUALS(mem.memory, NULL);
    TEST_NOT_EQUALS(mem.free, NULL);
    TEST_NOT_EQUALS(mem.completeness, NULL);
    TEST_EQUALS(mem.capacity, 8);
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), -1);

    // Both bitmaps live behind the element memory
    TEST_EQUALS(mem.free, (atomic<size_t> *) ALIGN_UP((uintptr_t) (mem.memory + 8), alignof(size_t)));
    TEST_EQUALS(mem.completeness, mem.free + 1);
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

static void test_thrd_chunk_t_init() {
    // chunk_init doesn't zero the bitmaps, the caller has to provide clean memory
    alignas(64) byte raw[256] = {0};

    ThrdChunkMemoryT<ThrdChunkTElement> mem = {0};
    chunk_init(&mem, raw, 4);

    TEST_EQUALS(mem.memory, (ThrdChunkTElement *) raw);
    TEST_EQUALS(mem.capacity, 4);
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), -1);
    TEST_TRUE(chunk_is_free(&mem, 0));
    TEST_TRUE(chunk_is_free(&mem, 3));
    TEST_FALSE(chunk_is_complete(&mem, (uint32) 0));

    // The memory starts at the requested alignment
    ThrdChunkMemoryT<ThrdChunkTElement> offset = {0};
    chunk_init(&offset, raw + 1, 4);
    TEST_EQUALS(offset.memory, (ThrdChunkTElement *) (raw + 8));
}

static void test_thrd_chunk_t_element_get() {
    ThrdChunkMemoryT<ThrdChunkTElement> mem = {0};
    chunk_alloc(&mem, 8, 8);

    TEST_EQUALS(chunk_element_get(&mem, 0), &mem.memory[0]);
    TEST_EQUALS(chunk_element_get(&mem, 3), &mem.memory[3]);
    TEST_EQUALS(chunk_element_get(&mem, 7), &mem.memory[7]);

    // Out of bounds (a negative id is rejected as well)
    TEST_EQUALS(chunk_element_get(&mem, 8), NULL);
    TEST_EQUALS(chunk_element_get(&mem, 1000), NULL);
    TEST_EQUALS(chunk_element_get(&mem, -1), NULL);

    chunk_free(&mem);
}

static void test_thrd_chunk_t_reserve_one() {
    ThrdChunkMemoryT<ThrdChunkTElement> mem = {0};
    chunk_alloc(&mem, 8, 8);

    // Elements are reserved in ascending order
    TEST_EQUALS(chunk_reserve_one(&mem), 0);
    TEST_FALSE(chunk_is_free(&mem, (uint32) 0));
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), 0);

    TEST_EQUALS(chunk_reserve_one(&mem), 1);
    TEST_EQUALS(chunk_reserve_one(&mem), 2);
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), 2);

    // An explicit hint is used when the hinted element is free
    TEST_EQUALS(chunk_reserve_one(&mem, 5), 5);
    TEST_FALSE(chunk_is_free(&mem, (uint32) 5));
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), 5);

    // A taken hint falls back to the regular search
    const int32 next = chunk_reserve_one(&mem, 5);
    TEST_NOT_EQUALS(next, 5);
    TEST_TRUE(next == 3 || next == 4 || next == 6 || next == 7);

    chunk_free(&mem);
}

static void test_thrd_chunk_t_reserve_range() {
    ThrdChunkMemoryT<ThrdChunkTElement> mem = {0};
    chunk_alloc(&mem, 8, 8);

    // A contiguous range in a fresh buffer starts at the first element
    TEST_EQUALS(chunk_reserve(&mem, 3), 0);
    TEST_FALSE(chunk_is_free(&mem, (uint32) 0));
    TEST_FALSE(chunk_is_free(&mem, (uint32) 1));
    TEST_FALSE(chunk_is_free(&mem, (uint32) 2));
    TEST_TRUE(chunk_is_free(&mem, (uint32) 3));
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), 2);

    // The next range starts behind the previous one
    TEST_EQUALS(chunk_reserve(&mem, 2), 3);
    TEST_FALSE(chunk_is_free(&mem, (uint32) 4));

    // A single element reservation is forwarded to chunk_reserve_one
    TEST_EQUALS(chunk_reserve(&mem, 1), 5);

    // Freeing a hole makes it reusable for the next range
    chunk_free_elements(&mem, 1, 2);
    TEST_TRUE(chunk_is_free(&mem, (uint32) 1));
    TEST_TRUE(chunk_is_free(&mem, (uint32) 2));
    TEST_EQUALS(chunk_reserve(&mem, 2), 1);

    chunk_free(&mem);
}

static void test_thrd_chunk_t_reserve_boundary() {
    ThrdChunkMemoryT<ThrdChunkTElement> mem = {0};
    chunk_alloc(&mem, 8, 8);

    // The whole buffer can be reserved at once
    TEST_EQUALS(chunk_reserve(&mem, 8), 0);
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), 7);

    bool all_used = true;
    for (int32 i = 0; i < 8; ++i) {
        all_used = all_used && !chunk_is_free(&mem, (uint32) i);
    }
    TEST_TRUE(all_used);

    // A full buffer has no free element and no free range
    TEST_EQUALS(chunk_reserve_one(&mem), -1);
    TEST_EQUALS(chunk_reserve(&mem, 2), -1);

    // A range bigger than the capacity can never fit
    TEST_EQUALS(chunk_reserve(&mem, 9), -1);

    // memory_get() handles the -1 of the reservation -> NULL, not an out of bounds pointer
    TEST_EQUALS(memory_get(&mem, sizeof(ThrdChunkTElement)), NULL);

    chunk_free(&mem);
}

static void test_thrd_chunk_t_multi_word() {
    // 65 elements need two bitmap words
    ThrdChunkMemoryT<ThrdChunkTElement> mem = {0};
    chunk_alloc(&mem, 65, 65);

    bool ascending = true;
    for (int32 i = 0; i < 65; ++i) {
        ascending = ascending && chunk_reserve_one(&mem) == i;
    }

    TEST_TRUE(ascending);
    TEST_EQUALS(mem.last_pos.load(memory_order_relaxed), 64);
    TEST_FALSE(chunk_is_free(&mem, (uint32) 64));
    TEST_EQUALS(chunk_reserve_one(&mem), -1);

    // A range across the word boundary is claimed as well
    for (int32 i = 32; i < 40; ++i) {
        chunk_free_element(&mem, (uint32) i);
    }

    TEST_TRUE(chunk_is_free(&mem, (uint32) 32));
    TEST_TRUE(chunk_is_free(&mem, (uint32) 39));
    TEST_EQUALS(chunk_reserve(&mem, 4), 32);
    TEST_FALSE(chunk_is_free(&mem, (uint32) 35));

    chunk_free(&mem);
}

static void test_thrd_chunk_t_free_element() {
    ThrdChunkMemoryT<ThrdChunkTElement> mem = {0};
    chunk_alloc(&mem, 8, 8);

    TEST_EQUALS(chunk_reserve(&mem, 3), 0);
    TEST_FALSE(chunk_is_free(&mem, (uint32) 0));

    // By element id
    chunk_free_element(&mem, (uint32) 0);
    TEST_TRUE(chunk_is_free(&mem, (uint32) 0));
    TEST_FALSE(chunk_is_free(&mem, (uint32) 1));

    // By bitmap position (free_index, bit_index)
    chunk_free_element(&mem, (size_t) 0, 1);
    TEST_TRUE(chunk_is_free(&mem, (uint32) 1));

    // By data pointer
    chunk_free_element(&mem, &mem.memory[2]);
    TEST_TRUE(chunk_is_free(&mem, (uint32) 2));

    // Freeing also clears the completeness mark
    chunk_mark_complete(&mem, (uint32) 3);
    TEST_TRUE(chunk_is_complete(&mem, (uint32) 3));
    chunk_free_element(&mem, (uint32) 3);
    TEST_FALSE(chunk_is_complete(&mem, (uint32) 3));

    chunk_free(&mem);
}

static void test_thrd_chunk_t_free_elements() {
    ThrdChunkMemoryT<ThrdChunkTElement> mem = {0};
    chunk_alloc(&mem, 8, 8);

    TEST_EQUALS(chunk_reserve(&mem, 4), 0);

    // By element id + count
    chunk_free_elements(&mem, 1, 2);
    TEST_FALSE(chunk_is_free(&mem, (uint32) 0));
    TEST_TRUE(chunk_is_free(&mem, (uint32) 1));
    TEST_TRUE(chunk_is_free(&mem, (uint32) 2));
    TEST_FALSE(chunk_is_free(&mem, (uint32) 3));

    // By data pointer + count
    chunk_free_elements(&mem, &mem.memory[3], 1);
    TEST_TRUE(chunk_is_free(&mem, (uint32) 3));

    chunk_free(&mem);
}

static void test_thrd_chunk_t_completeness() {
    ThrdChunkMemoryT<ThrdChunkTElement> mem = {0};
    chunk_alloc(&mem, 8, 8);

    chunk_reserve(&mem, 3);

    // Nothing is complete right after reserving
    for (int32 i = 0; i < 3; ++i) {
        TEST_FALSE(chunk_is_complete(&mem, (uint32) i));
    }

    chunk_mark_complete(&mem, (uint32) 0);
    chunk_mark_complete(&mem, (void *) &mem.memory[2]);

    TEST_TRUE(chunk_is_complete(&mem, (uint32) 0));
    TEST_FALSE(chunk_is_complete(&mem, (uint32) 1));
    TEST_TRUE(chunk_is_complete(&mem, (uint32) 2));
    TEST_TRUE(chunk_is_complete(&mem, (void *) &mem.memory[0]));

    chunk_clear_complete(&mem, (uint32) 2);
    TEST_FALSE(chunk_is_complete(&mem, (uint32) 2));

    chunk_free(&mem);
}

static void test_thrd_chunk_t_element_insert() {
    ThrdChunkMemoryT<ThrdChunkTElement> mem = {0};
    chunk_alloc(&mem, 4, 4);

    const ThrdChunkTElement a = {1, 10};
    chunk_element_insert(&mem, a);

    const ThrdChunkTElement b = {2, 20};
    chunk_element_insert(&mem, &b);

    // Inserted elements are written and marked as complete
    TEST_EQUALS(chunk_element_get(&mem, 0)->id, 1);
    TEST_EQUALS(chunk_element_get(&mem, 0)->value, 10);
    TEST_TRUE(chunk_is_complete(&mem, (uint32) 0));

    TEST_EQUALS(chunk_element_get(&mem, 1)->id, 2);
    TEST_EQUALS(chunk_element_get(&mem, 1)->value, 20);
    TEST_TRUE(chunk_is_complete(&mem, (uint32) 1));

    const ThrdChunkTElement reference = {2, 20};
    TEST_MEMORY_EQUALS(chunk_element_get(&mem, 1), &reference, sizeof(ThrdChunkTElement));

    chunk_free(&mem);
}

static void test_thrd_chunk_t_memory_get() {
    ThrdChunkMemoryT<ThrdChunkTElement> mem = {0};
    chunk_alloc(&mem, 8, 8);

    // One element request returns the first free element
    TEST_EQUALS(memory_get(&mem, sizeof(ThrdChunkTElement)), &mem.memory[0]);
    TEST_FALSE(chunk_is_free(&mem, (uint32) 0));

    // Two element request returns the next contiguous range
    TEST_EQUALS(memory_get(&mem, 2 * sizeof(ThrdChunkTElement)), &mem.memory[1]);
    TEST_FALSE(chunk_is_free(&mem, (uint32) 1));
    TEST_FALSE(chunk_is_free(&mem, (uint32) 2));

    // chunk_memory_get_one and chunk_memory_get work on single elements
    TEST_EQUALS(chunk_memory_get_one(&mem), &mem.memory[3]);
    TEST_EQUALS(chunk_memory_get(&mem, 1), &mem.memory[4]);

    // Temporary memory is free again right away
    ThrdChunkTElement* temp = memory_get_temp(&mem, 2 * sizeof(ThrdChunkTElement));
    TEST_EQUALS(temp, &mem.memory[5]);
    TEST_TRUE(chunk_is_free(&mem, (uint32) 5));
    TEST_TRUE(chunk_is_free(&mem, (uint32) 6));

    chunk_free(&mem);
}

static void test_thrd_chunk_t_dump_load() {
    ThrdChunkMemoryT<ThrdChunkTElement> src = {0};
    chunk_alloc(&src, 8, 8);

    const ThrdChunkTElement reference[] = {{1, 10}, {2, 20}, {3, 30}};
    for (int32 i = 0; i < 3; ++i) {
        chunk_element_insert(&src, reference[i]);
    }

    byte out[1024] = {0};
    const int64 dump_size = chunk_dump(&src, out);

    // header (capacity, last_pos, free_offset) + element memory + free bitmap + padding
    const int64 expected_size = sizeof(int32) + sizeof(int32) + sizeof(uint32)
        + 8 * sizeof(ThrdChunkTElement) + sizeof(size_t) * 1 + sizeof(size_t);
    TEST_EQUALS(dump_size, expected_size);

    ThrdChunkMemoryT<ThrdChunkTElement> dst = {0};
    chunk_alloc(&dst, 8, 8);

    const int64 load_size = chunk_load(&dst, out);
    TEST_EQUALS(load_size, dump_size);

    TEST_EQUALS(dst.capacity, src.capacity);
    TEST_EQUALS(dst.last_pos.load(memory_order_relaxed), src.last_pos.load(memory_order_relaxed));

    // The data and the free bitmap are restored
    for (int32 i = 0; i < 3; ++i) {
        TEST_MEMORY_EQUALS(chunk_element_get(&dst, i), &reference[i], sizeof(ThrdChunkTElement));
    }
    TEST_FALSE(chunk_is_free(&dst, (uint32) 0));
    TEST_FALSE(chunk_is_free(&dst, (uint32) 2));
    TEST_TRUE(chunk_is_free(&dst, (uint32) 3));

    // The loaded buffer is usable
    TEST_EQUALS(chunk_reserve_one(&dst), 3);

    chunk_free(&src);
    chunk_free(&dst);
}

static void test_thrd_chunk_t_arena() {
    MemoryArena* root = mem_arena_alloc(8192, 8192);

    ThrdChunkMemoryT<ThrdChunkTElement> mem = {0};
    chunk_alloc(&mem, root, 8, 8);

    TEST_NOT_EQUALS(mem.memory, NULL);
    TEST_EQUALS(mem.capacity, 8);
    TEST_EQUALS(mem_arena_stats(root).count, 2);

    // The arena backed chunk memory behaves like the platform backed one
    TEST_EQUALS(chunk_reserve_one(&mem), 0);
    TEST_EQUALS(chunk_element_get(&mem, 0), &mem.memory[0]);

    chunk_free(&mem, root);

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
    #define main MemoryThrdChunkMemoryTTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_thrd_chunk_t_alloc);
    TEST_RUN(test_thrd_chunk_t_init);
    TEST_RUN(test_thrd_chunk_t_element_get);
    TEST_RUN(test_thrd_chunk_t_reserve_one);
    TEST_RUN(test_thrd_chunk_t_reserve_range);
    TEST_RUN(test_thrd_chunk_t_reserve_boundary);
    TEST_RUN(test_thrd_chunk_t_multi_word);
    TEST_RUN(test_thrd_chunk_t_free_element);
    TEST_RUN(test_thrd_chunk_t_free_elements);
    TEST_RUN(test_thrd_chunk_t_completeness);
    TEST_RUN(test_thrd_chunk_t_element_insert);
    TEST_RUN(test_thrd_chunk_t_memory_get);
    TEST_RUN(test_thrd_chunk_t_dump_load);
    TEST_RUN(test_thrd_chunk_t_arena);

    TEST_FINALIZE();

    return 0;
}

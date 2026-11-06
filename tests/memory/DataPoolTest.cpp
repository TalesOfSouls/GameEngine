#include "../TestFramework.h"
#include "../../memory/DataPool.h"

/**
 * Tests for memory/DataPool.h
 *
 * DataPool is a ChunkMemory plus a second bitmap (`used`) which tracks whether
 * somebody currently points at the data. Elements marked as used may not be moved
 * by pool_defragment().
 *
 * WARNING: pool_defragment() is only covered on a statically reserved pool, and the
 * arena based pool_alloc()/pool_free() variants are not covered because mem_arena_remove()
 * currently crashes (@see bug report, MemoryArena.h:123).
 */
struct DataPoolElement {
    int32 id;
    int32 value;
};

static void test_data_pool_bitmap_words() {
    TEST_EQUALS(pool_bitmap_words(1), 1);
    TEST_EQUALS(pool_bitmap_words(8), 1);
    TEST_EQUALS(pool_bitmap_words(63), 1);
    TEST_EQUALS(pool_bitmap_words(64), 1);
    TEST_EQUALS(pool_bitmap_words(65), 2);
    TEST_EQUALS(pool_bitmap_words(128), 2);
    TEST_EQUALS(pool_bitmap_words(129), 3);

    // One bit per element, one size_t holds 64 bits
    TEST_EQUALS(pool_bitmap_words(64), (uint32) (64 / (sizeof(size_t) * 8)));
    TEST_EQUALS(pool_bitmap_words(256), (uint32) (256 / (sizeof(size_t) * 8)));
}

static void test_data_pool_alloc() {
    DataPool pool = {0};
    pool_alloc(&pool, 8, 16);

    TEST_NOT_EQUALS(pool.memory, NULL);
    TEST_NOT_EQUALS(pool.free, NULL);
    TEST_NOT_EQUALS(pool.used, NULL);
    TEST_EQUALS(pool.capacity, 8);
    TEST_EQUALS(pool.chunk_size, 16);
    TEST_EQUALS(pool.alignment, sizeof(size_t));
    TEST_EQUALS(pool.last_pos, -1);

    // capacity * chunk_size + free bitmap + used bitmap + alignment overhead
    const size_t expected_size = 8 * 16
        + sizeof(size_t) * pool_bitmap_words(8) * 2
        + alignof(size_t) * 3;
    TEST_EQUALS((uintptr_t) pool.size, (uintptr_t) expected_size);

    // Both bitmaps are behind the data area and the used bitmap follows the free bitmap
    TEST_EQUALS(pool.free, (size_t *) ALIGN_UP((uintptr_t) (pool.memory + 8 * 16), alignof(size_t)));
    TEST_EQUALS(pool.used, pool.free + pool_bitmap_words(8));

    // The bitmaps are zeroed -> nothing is reserved and nothing is in use
    TEST_EQUALS(*pool.free, 0);
    TEST_EQUALS(*pool.used, 0);

    pool_free(&pool);
    TEST_EQUALS(pool.memory, NULL);
    TEST_EQUALS(pool.size, 0);
}

static void test_data_pool_init_buffer() {
    BufferMemory buffer = {0};
    buffer_alloc(&buffer, 2048, 2048);

    DataPool pool = {0};
    pool_init(&pool, &buffer, 8, 16);

    TEST_NOT_EQUALS(pool.memory, NULL);
    TEST_EQUALS(pool.capacity, 8);
    TEST_EQUALS(pool.chunk_size, 16);
    TEST_EQUALS(pool.last_pos, -1);

    // The pool is carved out of the provided buffer
    TEST_TRUE(pool.memory >= buffer.memory);
    TEST_TRUE(pool.memory + pool.size <= buffer.end);

    // The free/used bitmaps are zeroed by pool_init
    TEST_EQUALS(*pool.free, 0);
    TEST_EQUALS(*pool.used, 0);

    // The pool doesn't own the memory
    buffer_free(&buffer);
}

static void test_data_pool_init_byte() {
    alignas(64) byte raw[1024] = {0};

    DataPool pool = {0};
    pool_init(&pool, raw, 4, 16);

    TEST_NOT_EQUALS(pool.memory, NULL);
    TEST_EQUALS(pool.memory, raw);
    TEST_EQUALS(pool.capacity, 4);
    TEST_EQUALS(pool.chunk_size, 16);

    // An unaligned chunk size is aligned up by pool_init
    DataPool aligned = {0};
    pool_init(&aligned, raw, 4, 10);
    TEST_EQUALS(aligned.chunk_size, 16);

    // The memory starts at the requested alignment
    DataPool offset = {0};
    pool_init(&offset, raw + 1, 4, 16);
    TEST_EQUALS((uintptr_t) offset.memory % offset.alignment, 0);
    TEST_EQUALS(offset.memory, raw + 8);
}

static void test_data_pool_reserve() {
    DataPool pool = {0};
    pool_alloc(&pool, 8, 16);

    // Elements are reserved in ascending order
    TEST_EQUALS(pool_reserve(&pool), 0);
    TEST_EQUALS(pool_reserve(&pool), 1);
    TEST_EQUALS(pool_reserve(&pool), 2);
    TEST_EQUALS(pool.last_pos, 2);

    TEST_FALSE(chunk_is_free_internal(pool.free, 0));
    TEST_FALSE(chunk_is_free_internal(pool.free, 1));
    TEST_FALSE(chunk_is_free_internal(pool.free, 2));
    TEST_TRUE(chunk_is_free_internal(pool.free, 3));

    // Reserved doesn't mean used, only pool_element_get() marks an element as used
    TEST_TRUE(chunk_is_free_internal(pool.used, 0));

    // Multiple elements at once
    TEST_EQUALS(pool_reserve(&pool, 3), 3);
    TEST_FALSE(chunk_is_free_internal(pool.free, 3));
    TEST_FALSE(chunk_is_free_internal(pool.free, 4));
    TEST_FALSE(chunk_is_free_internal(pool.free, 5));
    TEST_TRUE(chunk_is_free_internal(pool.free, 6));

    pool_free(&pool);
}

static void test_data_pool_element_get() {
    DataPool pool = {0};
    pool_alloc(&pool, 8, 16);

    pool_reserve(&pool, 3);

    byte* element0 = pool_element_get(&pool, 0);
    TEST_EQUALS(element0, pool.memory);
    TEST_FALSE(chunk_is_free_internal(pool.used, 0));

    byte* element2 = pool_element_get(&pool, 2);
    TEST_EQUALS(element2, pool.memory + 2 * 16);
    TEST_FALSE(chunk_is_free_internal(pool.used, 2));

    // Getting the same element twice is idempotent
    TEST_EQUALS(pool_element_get(&pool, 2), element2);

    // Out of bounds
    TEST_EQUALS(pool_element_get(&pool, 8), NULL);
    TEST_EQUALS(pool_element_get(&pool, 1000), NULL);

    // Element 1 was reserved but never requested by pointer -> still unused
    TEST_TRUE(chunk_is_free_internal(pool.used, 1));

    pool_free(&pool);
}

static void test_data_pool_release() {
    DataPool pool = {0};
    pool_alloc(&pool, 8, 16);

    pool_reserve(&pool, 2);
    pool_element_get(&pool, 0);
    pool_element_get(&pool, 1);

    TEST_FALSE(chunk_is_free_internal(pool.used, 0));
    TEST_FALSE(chunk_is_free_internal(pool.used, 1));

    pool_release(&pool, 0);

    TEST_TRUE(chunk_is_free_internal(pool.used, 0));
    TEST_FALSE(chunk_is_free_internal(pool.used, 1));

    // Releasing only clears the used mark, the chunk stays reserved
    TEST_FALSE(chunk_is_free_internal(pool.free, 0));

    // Releasing again is harmless
    pool_release(&pool, 0);
    TEST_TRUE(chunk_is_free_internal(pool.used, 0));

    pool_free(&pool);
}

static void test_data_pool_reserve_unused() {
    DataPool pool = {0};
    pool_alloc(&pool, 8, 16);

    // Finds the first element that is not marked as used and marks it
    TEST_EQUALS(pool_reserve_unused(&pool), 0);
    TEST_FALSE(chunk_is_free_internal(pool.used, 0));

    TEST_EQUALS(pool_reserve_unused(&pool), 1);
    TEST_FALSE(chunk_is_free_internal(pool.used, 1));

    TEST_EQUALS(pool_reserve_unused(&pool, 4), 4);
    TEST_FALSE(chunk_is_free_internal(pool.used, 4));

    // A released element becomes available again
    pool_release(&pool, 1);
    TEST_EQUALS(pool_reserve_unused(&pool), 1);

    pool_free(&pool);
}

static void test_data_pool_memory_get() {
    DataPool pool = {0};
    pool_alloc(&pool, 8, 16);

    byte* two = pool_memory_get(&pool, 2);
    TEST_EQUALS(two, pool.memory);

    // The returned element is marked as used (@see bug report: only the first chunk of a
    // multi element request is marked, the remaining chunks can be moved by pool_defragment())
    TEST_FALSE(chunk_is_free_internal(pool.used, 0));
    TEST_TRUE(chunk_is_free_internal(pool.used, 2));

    byte* one = pool_memory_get_one(&pool);
    TEST_EQUALS(one, pool.memory + 2 * 16);
    TEST_FALSE(chunk_is_free_internal(pool.used, 2));

    // One more two element request starts at chunk 3
    TEST_EQUALS(pool_memory_get(&pool, 2), pool.memory + 3 * 16);

    pool_free(&pool);
}

static void test_data_pool_defragment() {
    DataPool pool = {0};
    pool_alloc(&pool, 8, 16);

    // Reserve the first four chunks and hand out element 0 by pointer
    pool_reserve(&pool, 4);
    pool_element_get(&pool, 0);

    // Fill every chunk so the relocation can be verified
    for (int32 i = 0; i < 4; ++i) {
        memset(pool.memory + i * 16, i + 1, 16);
    }

    // Chunk 1 is the only free hole, chunk 3 is the last allocated but unused chunk
    chunk_free_element((ChunkMemory *) &pool, 1);

    TEST_TRUE(chunk_is_free_internal(pool.free, 1));
    TEST_TRUE(chunk_is_free_internal(pool.used, 3));

    pool_defragment(&pool);

    // Chunk 3 moved into the hole at chunk 1
    TEST_FALSE(chunk_is_free_internal(pool.free, 1));
    TEST_TRUE(chunk_is_free_internal(pool.free, 3));
    TEST_FALSE(chunk_is_free_internal(pool.free, 0));
    TEST_FALSE(chunk_is_free_internal(pool.free, 2));

    bool moved = true;
    for (int32 i = 0; i < 16; ++i) {
        moved = moved && pool.memory[1 * 16 + i] == 4;
    }
    TEST_TRUE(moved);

    // The moved element is no longer marked as used
    TEST_TRUE(chunk_is_free_internal(pool.used, 1));
    TEST_TRUE(chunk_is_free_internal(pool.used, 3));

    // The used element never moves
    bool kept = true;
    for (int32 i = 0; i < 16; ++i) {
        kept = kept && pool.memory[i] == 1;
    }
    TEST_TRUE(kept);
    TEST_FALSE(chunk_is_free_internal(pool.used, 0));

    // last_pos is recomputed to the highest allocated chunk
    TEST_EQUALS(pool.last_pos, 2);

    pool_free(&pool);
}

static void test_data_pool_round_trip() {
    DataPool pool = {0};
    pool_alloc(&pool, 8, sizeof(DataPoolElement));

    // The pool handles it's own chunk size, write through the returned pointers
    const DataPoolElement reference[] = {{1, 10}, {2, 20}, {3, 30}};

    DataPoolElement* elements[3] = {0};
    for (int32 i = 0; i < 3; ++i) {
        elements[i] = (DataPoolElement *) pool_memory_get_one(&pool);
        TEST_NOT_EQUALS(elements[i], NULL);
        memcpy(elements[i], &reference[i], sizeof(DataPoolElement));
    }

    for (int32 i = 0; i < 3; ++i) {
        TEST_EQUALS(elements[i]->id, reference[i].id);
        TEST_EQUALS(elements[i]->value, reference[i].value);
        TEST_MEMORY_EQUALS(elements[i], &reference[i], sizeof(DataPoolElement));
    }

    // The elements are distinct and ordered like the reference array
    TEST_EQUALS(elements[1], elements[0] + 1);
    TEST_EQUALS(elements[2], elements[1] + 1);
    TEST_NOT_EQUALS(elements[0], elements[2]);

    // Defragmenting must not move any of the elements because all are in use
    pool_defragment(&pool);

    for (int32 i = 0; i < 3; ++i) {
        TEST_MEMORY_EQUALS(elements[i], &reference[i], sizeof(DataPoolElement));
    }
    TEST_EQUALS(pool.last_pos, 2);

    pool_free(&pool);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main MemoryDataPoolTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_data_pool_bitmap_words);
    TEST_RUN(test_data_pool_alloc);
    TEST_RUN(test_data_pool_init_buffer);
    TEST_RUN(test_data_pool_init_byte);
    TEST_RUN(test_data_pool_reserve);
    TEST_RUN(test_data_pool_element_get);
    TEST_RUN(test_data_pool_release);
    TEST_RUN(test_data_pool_reserve_unused);
    TEST_RUN(test_data_pool_memory_get);
    TEST_RUN(test_data_pool_defragment);
    TEST_RUN(test_data_pool_round_trip);

    TEST_FINALIZE();

    return 0;
}

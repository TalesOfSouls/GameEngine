#include "../TestFramework.h"

#if defined(_MSC_VER)
    // memory/FragmentMemory.h compares a signed loop counter against the unsigned count (C4018)
    #pragma warning(push)
    #pragma warning(disable: 4018)
#endif

#include "../../memory/FragmentMemory.h"

#if defined(_MSC_VER)
    #pragma warning(pop)
#endif

/**
 * Tests for memory/FragmentMemory.h
 *
 * The untyped fragment memory hands out byte chunks from a chunk pool.
 * The free array stores byte offsets (the typed variant stores indices).
 */
// The root arena owns the platform allocation, its base lives inside of it
static void fragment_arena_release_root(MemoryArena* root) {
    void* base = root->base;
    platform_aligned_free(&base);
}

static void test_fragment_alloc() {
    FragmentMemory fragment = {0};
    fragment_alloc(&fragment, 10, 20, 16);

    TEST_NOT_EQUALS(fragment.memory, NULL);
    TEST_NOT_EQUALS(fragment.free, NULL);
    TEST_EQUALS(fragment.count, 10);
    TEST_EQUALS(fragment.chunk_size, 16);
    TEST_EQUALS(fragment.alignment, sizeof(size_t));
    TEST_EQUALS(fragment.last_pos, 9);
    TEST_EQUALS((uintptr_t) fragment.size, (uintptr_t) fragment_size_total(10, 16, sizeof(size_t)));

    // The free array is behind the element memory
    TEST_EQUALS(
        fragment.free,
        (int32 *) ALIGN_UP((uintptr_t) (fragment.memory + 10 * 16), alignof(int32))
    );

    // Every chunk is available and stores its byte offset
    bool offsets_ok = true;
    for (int32 i = 0; i < 10; ++i) {
        offsets_ok = offsets_ok && fragment.free[i] == i * 16;
    }
    TEST_TRUE(offsets_ok);

    // The chunk memory starts at the requested start alignment
    TEST_EQUALS((uintptr_t) fragment.memory % ASSUMED_CACHE_LINE_SIZE, 0);

    fragment_free(&fragment);
}

static void test_fragment_size() {
    // The chunk size is aligned up, a chunk could otherwise start unaligned
    TEST_EQUALS(fragment_size_element(14, 8), 16);
    TEST_EQUALS(fragment_size_element(16, 8), 16);
    TEST_EQUALS(fragment_size_element(1, 8), 8);
    TEST_EQUALS(fragment_size_element(8, 4), 8);
    TEST_EQUALS(fragment_size_element(3, 4), 4);

    // count * chunk_size + free array + alignment overhead
    TEST_EQUALS((uintptr_t) fragment_size_total(4, 16, 8), (uintptr_t) (4 * 16 + 4 * 4 + 16));
    TEST_EQUALS((uintptr_t) fragment_size_total(1, 8, 8), (uintptr_t) (8 + 4 + 16));
    TEST_EQUALS((uintptr_t) fragment_size_total(10, 16, 8), (uintptr_t) (10 * 16 + 10 * 4 + 16));

    // An element size that is already aligned doesn't change the chunk size
    TEST_EQUALS(
        (uintptr_t) fragment_size_total(10, 14, 8),
        (uintptr_t) fragment_size_total(10, 16, 8)
    );
}

static void test_fragment_memory_get() {
    FragmentMemory fragment = {0};
    fragment_alloc(&fragment, 10, 10, 16);

    // Chunks are handed out from the back to the front
    byte* first = fragment_memory_get(&fragment);
    TEST_EQUALS(first, fragment.memory + 9 * 16);
    TEST_EQUALS(fragment.last_pos, 8);

    byte* second = fragment_memory_get(&fragment);
    TEST_EQUALS(second, fragment.memory + 8 * 16);
    TEST_EQUALS(fragment.last_pos, 7);

    // The chunks don't overlap and are aligned to the chunk size
    TEST_TRUE(first + 16 <= second || second + 16 <= first);
    TEST_EQUALS((uintptr_t) first % fragment.alignment, 0);
    TEST_EQUALS((uintptr_t) second % fragment.alignment, 0);

    fragment_free(&fragment);
}

static void test_fragment_memory_get_empty() {
    FragmentMemory fragment = {0};
    fragment_alloc(&fragment, 4, 4, 8);

    byte* chunks[4] = {0};
    for (int32 i = 0; i < 4; ++i) {
        chunks[i] = fragment_memory_get(&fragment);
        TEST_NOT_EQUALS(chunks[i], NULL);
    }

    TEST_EQUALS(fragment.last_pos, -1);

    // The fragment memory is exhausted
    TEST_EQUALS(fragment_memory_get(&fragment), NULL);
    TEST_EQUALS(fragment_memory_get(&fragment), NULL);
    TEST_EQUALS(fragment.last_pos, -1);

    fragment_free(&fragment);
}

static void test_fragment_release_memory() {
    FragmentMemory fragment = {0};
    fragment_alloc(&fragment, 4, 4, 16);

    byte* a = fragment_memory_get(&fragment);
    byte* b = fragment_memory_get(&fragment);

    // Released memory is handed out again, last released first
    fragment_release_memory(&fragment, a);
    TEST_EQUALS(fragment.last_pos, 2);
    TEST_EQUALS(fragment.free[2], (int32) (a - fragment.memory));

    byte* c = fragment_memory_get(&fragment);
    TEST_EQUALS(c, a);
    TEST_EQUALS(fragment.last_pos, 1);

    fragment_release_memory(&fragment, b);
    fragment_release_memory(&fragment, c);
    TEST_EQUALS(fragment.last_pos, 3);

    // Both chunks are free again and handed out in reverse release order
    TEST_EQUALS(fragment_memory_get(&fragment), c);
    TEST_EQUALS(fragment_memory_get(&fragment), b);

    fragment_free(&fragment);
}

static void test_fragment_round_trip() {
    FragmentMemory fragment = {0};
    fragment_alloc(&fragment, 5, 5, 16);

    byte* chunks[5] = {0};
    for (int32 i = 0; i < 5; ++i) {
        chunks[i] = fragment_memory_get(&fragment);
        memset(chunks[i], i + 1, fragment.chunk_size);
    }

    // The content of every chunk is independent
    for (int32 i = 0; i < 5; ++i) {
        bool is_value = true;
        for (int32 j = 0; j < fragment.chunk_size; ++j) {
            is_value = is_value && chunks[i][j] == (byte) (i + 1);
        }
        TEST_TRUE(is_value);
    }

    // Releasing and requesting again returns the same chunks (in reverse release order)
    // and the chunk content is untouched
    for (int32 i = 0; i < 5; ++i) {
        fragment_release_memory(&fragment, chunks[i]);
    }

    for (int32 i = 0; i < 5; ++i) {
        byte* reused = fragment_memory_get(&fragment);
        TEST_EQUALS(reused, chunks[4 - i]);

        bool is_value = true;
        for (int32 j = 0; j < fragment.chunk_size; ++j) {
            is_value = is_value && reused[j] == (byte) ((4 - i) + 1);
        }
        TEST_TRUE(is_value);
    }

    fragment_free(&fragment);
}

static void test_fragment_reuse_all() {
    FragmentMemory fragment = {0};
    fragment_alloc(&fragment, 8, 8, 8);

    byte* first_pass[8] = {0};
    for (int32 i = 0; i < 8; ++i) {
        first_pass[i] = fragment_memory_get(&fragment);
    }

    // Release everything in allocation order (so the LIFO order flips)
    for (int32 i = 0; i < 8; ++i) {
        fragment_release_memory(&fragment, first_pass[i]);
    }

    byte* second_pass[8] = {0};
    for (int32 i = 0; i < 8; ++i) {
        second_pass[i] = fragment_memory_get(&fragment);
    }

    // The same set of chunks is available, just in the opposite order
    for (int32 i = 0; i < 8; ++i) {
        TEST_EQUALS(second_pass[i], first_pass[7 - i]);
    }

    fragment_free(&fragment);
}

static void test_fragment_free() {
    FragmentMemory fragment = {0};
    fragment_alloc(&fragment, 6, 6, 12, 4);

    TEST_NOT_EQUALS(fragment.memory, NULL);
    TEST_EQUALS(fragment.chunk_size, 12);
    TEST_EQUALS(fragment.alignment, 4);

    const size_t size = fragment.size;
    fragment_free(&fragment);

    TEST_EQUALS(fragment.memory, NULL);
    TEST_EQUALS(fragment.size, 0);
    TEST_EQUALS(fragment.last_pos, -1);
    TEST_TRUE(size > 0);
}

static void test_fragment_alignment() {
    // An element size that is not a multiple of the alignment is rounded up
    FragmentMemory fragment = {0};
    fragment_alloc(&fragment, 4, 4, 14, 8);

    TEST_EQUALS(fragment.chunk_size, 16);
    TEST_EQUALS(fragment.alignment, 8);

    for (int32 i = 0; i < 4; ++i) {
        byte* chunk = fragment_memory_get(&fragment);
        TEST_EQUALS((uintptr_t) chunk % 8, 0);
        TEST_EQUALS(chunk, fragment.memory + (3 - i) * 16);
    }

    fragment_free(&fragment);

    // A 4 byte alignment produces 4 byte aligned chunks
    fragment_alloc(&fragment, 4, 8, 10, 4);
    TEST_EQUALS(fragment.chunk_size, 12);
    TEST_EQUALS((uintptr_t) fragment.memory % ASSUMED_CACHE_LINE_SIZE, 0);

    byte* chunk = fragment_memory_get(&fragment);
    TEST_EQUALS((uintptr_t) chunk % 4, 0);

    fragment_free(&fragment);
}

static void test_fragment_arena() {
    MemoryArena* root = mem_arena_alloc(8192, 8192);

    FragmentMemory fragment = {0};
    fragment_alloc(&fragment, root, 8, 8, 16);

    TEST_NOT_EQUALS(fragment.memory, NULL);
    TEST_EQUALS(fragment.count, 8);
    TEST_EQUALS(fragment.chunk_size, 16);
    TEST_EQUALS(fragment.last_pos, 7);
    TEST_EQUALS((uintptr_t) fragment.memory % ASSUMED_CACHE_LINE_SIZE, 0);

    // The fragment memory is the user memory of a new arena in the chain
    bool inside_chain = false;
    for (MemoryArena* mem = root; mem; mem = mem->next) {
        inside_chain = inside_chain || fragment.memory == mem->memory;
    }
    TEST_TRUE(inside_chain);
    TEST_EQUALS(mem_arena_stats(root).count, 2);

    // The arena backed fragment memory behaves like the platform backed one
    byte* first = fragment_memory_get(&fragment);
    TEST_EQUALS(first, fragment.memory + 7 * 16);
    TEST_EQUALS(fragment.last_pos, 6);

    fragment_release_memory(&fragment, first);
    TEST_EQUALS(fragment_memory_get(&fragment), first);
    TEST_EQUALS(fragment.last_pos, 6);

    fragment_free(&fragment, root);

    // The arena node is released with it
    TEST_EQUALS(fragment.memory, NULL);
    TEST_EQUALS(fragment.size, 0);
    TEST_EQUALS(fragment.last_pos, -1);
    TEST_EQUALS(mem_arena_stats(root).count, 1);

    // The root is still usable
    root->memory[0] = 9;
    TEST_EQUALS(root->memory[0], 9);

    fragment_arena_release_root(root);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main MemoryFragmentMemoryTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_fragment_alloc);
    TEST_RUN(test_fragment_size);
    TEST_RUN(test_fragment_memory_get);
    TEST_RUN(test_fragment_memory_get_empty);
    TEST_RUN(test_fragment_release_memory);
    TEST_RUN(test_fragment_round_trip);
    TEST_RUN(test_fragment_reuse_all);
    TEST_RUN(test_fragment_free);
    TEST_RUN(test_fragment_alignment);
    TEST_RUN(test_fragment_arena);

    TEST_FINALIZE();

    return 0;
}

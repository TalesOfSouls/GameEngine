#include "../TestFramework.h"
#include "../../memory/MemoryArena.h"

/**
 * Tests for memory/MemoryArena.h
 *
 * A MemoryArena is a chain of platform allocations where every arena node is stored
 * inside the platform allocation itself (in front of the user memory).
 * There is no mem_arena_free(): the root can't remove itself (the caller owns the pointer),
 * so the tests remove every added arena with mem_arena_remove() and release the root last.
 */
static void arena_release_all(MemoryArena* root) {
    // Every added arena can be removed through the chain
    while (root->next) {
        mem_arena_remove(root, root->next);
    }

    // The root itself is owned by the caller, so its platform allocation is released directly
    // (the base pointer lives inside the allocation and has to be copied first)
    void* base = root->base;
    platform_aligned_free(&base);
}

static void test_mem_arena_alloc() {
    MemoryArena* mem = mem_arena_alloc(4096, 8192);
    TEST_NOT_EQUALS(mem, NULL);

    TEST_NOT_EQUALS(mem->base, NULL);
    TEST_NOT_EQUALS(mem->header, NULL);
    TEST_NOT_EQUALS(mem->memory, NULL);
    TEST_EQUALS(mem->next, NULL);

    // The arena metadata lives in front of the user memory
    TEST_TRUE(mem->memory >= mem->base + sizeof(MemoryArena));
    TEST_TRUE(mem->memory > (byte *) mem);

    // The default alignment is sizeof(void*)
    TEST_EQUALS((uintptr_t) mem->memory % sizeof(void *), 0);

    // The header is the platform allocation header (@see platform_alloc_aligned)
    TEST_EQUALS(mem->header, (platform_alloc_header *) ((void **) mem->base)[-1]);
    TEST_TRUE(mem->header->committed_size >= 4096);
    TEST_TRUE(mem->header->reserved_size >= mem->header->committed_size);

    arena_release_all(mem);
}

static void test_mem_arena_memory_write() {
    MemoryArena* mem = mem_arena_alloc(4096, 4096);

    // Committed platform memory is zero initialized
    bool is_zero = true;
    for (uint32 i = 0; i < 64; ++i) {
        is_zero = is_zero && mem->memory[i] == 0;
    }
    TEST_TRUE(is_zero);

    // The requested amount of memory is usable
    for (uint32 i = 0; i < 4096; ++i) {
        mem->memory[i] = (byte) (i & 0xFF);
    }

    bool is_pattern = true;
    for (uint32 i = 0; i < 4096; ++i) {
        is_pattern = is_pattern && mem->memory[i] == (byte) (i & 0xFF);
    }
    TEST_TRUE(is_pattern);

    arena_release_all(mem);
}

static void test_mem_arena_add() {
    MemoryArena* root = mem_arena_alloc(2048, 2048);
    MemoryArena* a1 = mem_arena_add(root, 4096, 4096);

    TEST_NOT_EQUALS(a1, NULL);
    TEST_EQUALS(root->next, a1);
    TEST_EQUALS(a1->next, NULL);
    TEST_NOT_EQUALS(a1->memory, root->memory);
    TEST_NOT_EQUALS(a1->base, root->base);
    TEST_EQUALS((uintptr_t) a1->memory % sizeof(void *), 0);

    MemoryArena* a2 = mem_arena_add(root, 8192, 8192);

    TEST_NOT_EQUALS(a2, NULL);
    TEST_EQUALS(root->next, a1);
    TEST_EQUALS(a1->next, a2);
    TEST_EQUALS(a2->next, NULL);
    TEST_NOT_EQUALS(a2->memory, a1->memory);
    TEST_EQUALS(mem_arena_stats(root).count, 3);

    arena_release_all(root);
}

static void test_mem_arena_add_deep() {
    MemoryArena* root = mem_arena_alloc(1024, 1024);
    MemoryArena* last = root;

    // Every add walks to the end of the chain and appends there
    for (int32 i = 0; i < 8; ++i) {
        MemoryArena* added = mem_arena_add(root, 1024, 1024);
        TEST_EQUALS(last->next, added);
        TEST_EQUALS(added->next, NULL);

        last = added;
    }

    int32 count = 0;
    for (MemoryArena* mem = root; mem; mem = mem->next) {
        ++count;
    }

    TEST_EQUALS(count, 9);
    TEST_EQUALS(root->next != NULL, 1);
    TEST_EQUALS(last->next, NULL);
    TEST_EQUALS(mem_arena_stats(root).count, 9);

    arena_release_all(root);
}

static void test_mem_arena_stats() {
    MemoryArena* root = mem_arena_alloc(4096, 8192);
    MemoryArena* a1 = mem_arena_add(root, 2048, 4096);

    const MemoryArenaStats stats = mem_arena_stats(root);

    TEST_EQUALS(stats.count, 2);
    TEST_EQUALS(
        (uintptr_t) stats.reserved_size,
        (uintptr_t) root->header->reserved_size + (uintptr_t) a1->header->reserved_size
    );
    TEST_EQUALS(
        (uintptr_t) stats.committed_size,
        (uintptr_t) root->header->committed_size + (uintptr_t) a1->header->committed_size
    );

    // At least the requested user memory is committed and never more than reserved
    TEST_TRUE(stats.committed_size >= 4096 + 2048);
    TEST_TRUE(stats.reserved_size >= stats.committed_size);

    // The overhead is the platform header + alignment padding in front of the user memory
    TEST_TRUE(stats.overhead > 0);
    TEST_TRUE((uintptr_t) stats.overhead >= 2 * sizeof(platform_alloc_header));

    arena_release_all(root);

    // A single arena is the same as a chain of one
    root = mem_arena_alloc(4096, 4096);
    const MemoryArenaStats single = mem_arena_stats(root);
    TEST_EQUALS(single.count, 1);
    TEST_EQUALS((uintptr_t) single.reserved_size, (uintptr_t) root->header->reserved_size);
    TEST_EQUALS((uintptr_t) single.committed_size, (uintptr_t) root->header->committed_size);
    arena_release_all(root);
}

static void test_mem_arena_alignment() {
    const int32 alignments[] = {8, 16, 32, 64};
    MemoryArena* arenas[ARRAY_COUNT(alignments)] = {0};

    for (uint32 i = 0; i < ARRAY_COUNT(alignments); ++i) {
        arenas[i] = mem_arena_alloc(1024, 1024, alignments[i]);
        TEST_NOT_EQUALS(arenas[i], NULL);

        TEST_EQUALS((uintptr_t) arenas[i]->memory % alignments[i], 0);
        TEST_EQUALS((uintptr_t) arenas[i]->header % sizeof(size_t), 0);
    }

    // Two arenas never share their user memory
    TEST_NOT_EQUALS(arenas[0]->memory, arenas[1]->memory);
    TEST_TRUE(arenas[0]->memory + 1024 <= arenas[1]->memory
        || arenas[1]->memory + 1024 <= arenas[0]->memory);

    for (uint32 i = 0; i < ARRAY_COUNT(alignments); ++i) {
        arena_release_all(arenas[i]);
    }
}

static void test_mem_arena_reserve() {
    // Reserving more than is initially committed is allowed
    MemoryArena* mem = mem_arena_alloc(4096, 65536);

    TEST_NOT_EQUALS(mem, NULL);
    TEST_TRUE(mem->header->reserved_size > mem->header->committed_size);
    TEST_TRUE(mem->header->committed_size < 65536);
    TEST_TRUE(mem->header->reserved_size >= 65536);

    // Same with a larger initial size than the reserve is not allowed by the API contract,
    // so only the equal case is checked here
    arena_release_all(mem);

    mem = mem_arena_alloc(8192, 8192);
    TEST_EQUALS(
        (uintptr_t) mem->header->reserved_size,
        (uintptr_t) mem->header->committed_size
    );
    arena_release_all(mem);
}

static void test_mem_arena_remove() {
    MemoryArena* root = mem_arena_alloc(2048, 2048);
    MemoryArena* a1 = mem_arena_add(root, 2048, 2048);
    MemoryArena* a2 = mem_arena_add(root, 2048, 2048);
    MemoryArena* a3 = mem_arena_add(root, 2048, 2048);

    TEST_EQUALS(mem_arena_stats(root).count, 4);
    TEST_EQUALS(root->next, a1);

    // Removing a link in the middle reconnects the neighbors
    mem_arena_remove(root, a2);
    TEST_EQUALS(root->next, a1);
    TEST_EQUALS(a1->next, a3);
    TEST_EQUALS(a3->next, NULL);
    TEST_EQUALS(mem_arena_stats(root).count, 3);

    // Removing the first added arena keeps the rest intact
    mem_arena_remove(root, a1);
    TEST_EQUALS(root->next, a3);
    TEST_EQUALS(a3->next, NULL);
    TEST_EQUALS(mem_arena_stats(root).count, 2);

    // The remaining arena is still usable
    a3->memory[0] = 42;
    TEST_EQUALS(a3->memory[0], 42);
    TEST_TRUE(a3->header->committed_size >= 2048);

    // Removing the last added arena empties the chain
    mem_arena_remove(root, a3);
    TEST_EQUALS(root->next, NULL);
    TEST_EQUALS(mem_arena_stats(root).count, 1);

    // The root itself is untouched and still usable
    root->memory[0] = 7;
    TEST_EQUALS(root->memory[0], 7);

    arena_release_all(root);
}

static void test_mem_arena_remove_by_memory() {
    MemoryArena* root = mem_arena_alloc(2048, 2048);
    MemoryArena* a1 = mem_arena_add(root, 2048, 2048);
    MemoryArena* a2 = mem_arena_add(root, 2048, 2048);

    // The user memory address alone identifies the arena
    mem_arena_remove(root, a1->memory);
    TEST_EQUALS(root->next, a2);
    TEST_EQUALS(a2->next, NULL);
    TEST_EQUALS(mem_arena_stats(root).count, 2);

    mem_arena_remove(root, a2->memory);
    TEST_EQUALS(root->next, NULL);
    TEST_EQUALS(mem_arena_stats(root).count, 1);

    arena_release_all(root);
}

static void test_mem_arena_remove_not_found() {
    MemoryArena* root = mem_arena_alloc(2048, 2048);
    MemoryArena* added = mem_arena_add(root, 2048, 2048);

    // An arena of a different chain is not part of this arena -> no-op
    MemoryArena* other = mem_arena_alloc(2048, 2048);
    mem_arena_remove(root, other);

    TEST_EQUALS(root->next, added);
    TEST_EQUALS(added->next, NULL);
    TEST_EQUALS(mem_arena_stats(root).count, 2);

    // The other arena is still intact and usable
    other->memory[0] = 3;
    TEST_EQUALS(other->memory[0], 3);
    TEST_EQUALS(other->next, NULL);

    arena_release_all(other);
    arena_release_all(root);
}

struct ArenaTestElement {
    int32 id;
    f32 value;
};

static void test_mem_arena_round_trip() {
    MemoryArena* root = mem_arena_alloc(4096, 8192);
    MemoryArena* a1 = mem_arena_add(root, 4096, 4096);

    const ArenaTestElement reference = {1234, 1.5f};

    ArenaTestElement* root_element = (ArenaTestElement *) root->memory;
    ArenaTestElement* added_element = (ArenaTestElement *) a1->memory;
    memcpy(root_element, &reference, sizeof(ArenaTestElement));
    memcpy(added_element, &reference, sizeof(ArenaTestElement));

    TEST_EQUALS(root_element->id, reference.id);
    TEST_EQUALS_WITH_DELTA(root_element->value, reference.value, 0.0001);
    TEST_MEMORY_EQUALS(root_element, &reference, sizeof(ArenaTestElement));
    TEST_MEMORY_EQUALS(added_element, &reference, sizeof(ArenaTestElement));

    // Every arena has its own user memory
    TEST_NOT_EQUALS(root_element, added_element);

    added_element->id = 5;
    TEST_EQUALS(added_element->id, 5);
    TEST_EQUALS(root_element->id, reference.id);

    // The metadata of the second arena is untouched by user writes
    TEST_EQUALS(a1->header, (platform_alloc_header *) ((void **) a1->base)[-1]);
    TEST_TRUE(a1->memory >= a1->base + sizeof(MemoryArena));

    arena_release_all(root);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main MemoryMemoryArenaTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_mem_arena_alloc);
    TEST_RUN(test_mem_arena_memory_write);
    TEST_RUN(test_mem_arena_add);
    TEST_RUN(test_mem_arena_add_deep);
    TEST_RUN(test_mem_arena_stats);
    TEST_RUN(test_mem_arena_remove);
    TEST_RUN(test_mem_arena_remove_by_memory);
    TEST_RUN(test_mem_arena_remove_not_found);
    TEST_RUN(test_mem_arena_alignment);
    TEST_RUN(test_mem_arena_reserve);
    TEST_RUN(test_mem_arena_round_trip);

    TEST_FINALIZE();

    return 0;
}

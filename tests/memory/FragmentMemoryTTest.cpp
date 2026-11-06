#include "../TestFramework.h"
#include "../../memory/FragmentMemoryT.h"

/**
 * Tests for memory/FragmentMemoryT.h
 *
 * The typed fragment memory hands out single T elements from a chunk pool.
 * The free array stores indices (not byte offsets like the untyped FragmentMemory).
 */

struct FragmentTElement {
    int32 id;
    int32 value;
};

static void test_fragment_t_alloc() {
    FragmentMemoryT<FragmentTElement> fragment = {0};
    fragment_alloc(&fragment, 8, 16);

    TEST_NOT_EQUALS(fragment.memory, NULL);
    TEST_NOT_EQUALS(fragment.free, NULL);
    TEST_EQUALS(fragment.capacity, 8);
    TEST_EQUALS(fragment.last_pos, 7);

    // The free array is behind the element memory (capacity * sizeof(T) bytes)
    TEST_EQUALS(
        fragment.free,
        (int32 *) ALIGN_UP(
            (uintptr_t) ((byte *) fragment.memory + 8 * sizeof(FragmentTElement)),
            alignof(int32)
        )
    );
    TEST_TRUE((byte *) fragment.free >= (byte *) (fragment.memory + 8));

    // Every element is available and stores its index
    bool indices_ok = true;
    for (int32 i = 0; i < 8; ++i) {
        indices_ok = indices_ok && fragment.free[i] == i;
    }
    TEST_TRUE(indices_ok);

    // The default alignment is sizeof(size_t)
    TEST_EQUALS((uintptr_t) fragment.memory % sizeof(size_t), 0);

    fragment_free(&fragment);
}

static void test_fragment_t_memory_get() {
    FragmentMemoryT<FragmentTElement> fragment = {0};
    fragment_alloc(&fragment, 8, 8);

    // Elements are handed out from the back to the front
    FragmentTElement* first = fragment_memory_get(&fragment);
    TEST_EQUALS(first, &fragment.memory[7]);
    TEST_EQUALS(fragment.last_pos, 6);

    FragmentTElement* second = fragment_memory_get(&fragment);
    TEST_EQUALS(second, &fragment.memory[6]);
    TEST_EQUALS(fragment.last_pos, 5);

    // Typed pointer arithmetic: the elements are adjacent and don't overlap
    TEST_EQUALS(second + 1, first);
    TEST_EQUALS(first - second, 1);

    fragment_free(&fragment);
}

static void test_fragment_t_get_empty() {
    FragmentMemoryT<FragmentTElement> fragment = {0};
    fragment_alloc(&fragment, 4, 4);

    FragmentTElement* elements[4] = {0};
    for (int32 i = 0; i < 4; ++i) {
        elements[i] = fragment_memory_get(&fragment);
        TEST_NOT_EQUALS(elements[i], NULL);
    }

    TEST_EQUALS(fragment.last_pos, -1);

    // The fragment memory is exhausted
    TEST_EQUALS(fragment_memory_get(&fragment), NULL);
    TEST_EQUALS(fragment_memory_get(&fragment), NULL);
    TEST_EQUALS(fragment.last_pos, -1);

    fragment_free(&fragment);
}

static void test_fragment_t_release() {
    FragmentMemoryT<FragmentTElement> fragment = {0};
    fragment_alloc(&fragment, 4, 4);

    FragmentTElement* a = fragment_memory_get(&fragment);
    FragmentTElement* b = fragment_memory_get(&fragment);

    // Released elements are handed out again, last released first
    fragment_release_memory(&fragment, a);
    TEST_EQUALS(fragment.last_pos, 2);
    TEST_EQUALS(fragment.free[2], (int32) (a - fragment.memory));

    FragmentTElement* c = fragment_memory_get(&fragment);
    TEST_EQUALS(c, a);
    TEST_EQUALS(fragment.last_pos, 1);

    fragment_release_memory(&fragment, b);
    fragment_release_memory(&fragment, c);
    TEST_EQUALS(fragment.last_pos, 3);

    TEST_EQUALS(fragment_memory_get(&fragment), c);
    TEST_EQUALS(fragment_memory_get(&fragment), b);

    fragment_free(&fragment);
}

static void test_fragment_t_round_trip() {
    FragmentMemoryT<FragmentTElement> fragment = {0};
    fragment_alloc(&fragment, 5, 5);

    FragmentTElement* elements[5] = {0};
    for (int32 i = 0; i < 5; ++i) {
        elements[i] = fragment_memory_get(&fragment);
        TEST_NOT_EQUALS(elements[i], NULL);

        elements[i]->id = i;
        elements[i]->value = i * 10;
    }

    // The reference values are readable through the returned pointers
    for (int32 i = 0; i < 5; ++i) {
        const FragmentTElement reference = {i, i * 10};
        TEST_MEMORY_EQUALS(elements[i], &reference, sizeof(FragmentTElement));
    }

    // Release everything and request it again: the content must survive
    for (int32 i = 0; i < 5; ++i) {
        fragment_release_memory(&fragment, elements[i]);
    }

    for (int32 i = 0; i < 5; ++i) {
        FragmentTElement* reused = fragment_memory_get(&fragment);
        TEST_EQUALS(reused, elements[4 - i]);
        TEST_EQUALS(reused->id, 4 - i);
        TEST_EQUALS(reused->value, (4 - i) * 10);
    }

    fragment_free(&fragment);
}

static void test_fragment_t_release_order() {
    FragmentMemoryT<FragmentTElement> fragment = {0};
    fragment_alloc(&fragment, 6, 6);

    FragmentTElement* first_pass[6] = {0};
    for (int32 i = 0; i < 6; ++i) {
        first_pass[i] = fragment_memory_get(&fragment);
    }

    // Releasing in allocation order flips the LIFO order of the next pass
    for (int32 i = 0; i < 6; ++i) {
        fragment_release_memory(&fragment, first_pass[i]);
    }

    for (int32 i = 0; i < 6; ++i) {
        TEST_EQUALS(fragment_memory_get(&fragment), first_pass[5 - i]);
    }

    TEST_EQUALS(fragment.last_pos, -1);

    fragment_free(&fragment);
}

static void test_fragment_t_free() {
    FragmentMemoryT<FragmentTElement> fragment = {0};
    fragment_alloc(&fragment, 6, 12);

    TEST_NOT_EQUALS(fragment.memory, NULL);
    TEST_EQUALS(fragment.capacity, 6);

    fragment_free(&fragment);

    TEST_EQUALS(fragment.memory, NULL);
    TEST_EQUALS(fragment.capacity, 0);
    TEST_EQUALS(fragment.last_pos, -1);
}

struct FragmentTBigElement {
    int32 a;
    int32 b;
    int32 c;
};

static void test_fragment_t_alignment() {
    // The start alignment is the requested alignment, the element size is sizeof(T)
    FragmentMemoryT<FragmentTBigElement> fragment = {0};
    fragment_alloc(&fragment, 4, 8, 16);

    TEST_EQUALS(sizeof(FragmentTBigElement), 12);
    TEST_EQUALS(fragment.capacity, 4);
    TEST_EQUALS((uintptr_t) fragment.memory % 16, 0);

    TEST_EQUALS(
        fragment.free,
        (int32 *) ALIGN_UP(
            (uintptr_t) ((byte *) fragment.memory + 4 * sizeof(FragmentTBigElement)),
            alignof(int32)
        )
    );
    TEST_TRUE((byte *) fragment.free >= (byte *) (fragment.memory + 4));
    TEST_EQUALS((uintptr_t) fragment.free % alignof(int32), 0);

    FragmentTBigElement* element = fragment_memory_get(&fragment);
    TEST_EQUALS(element, &fragment.memory[3]);
    TEST_EQUALS((uintptr_t) element % alignof(FragmentTBigElement), 0);

    element->a = 1;
    element->b = 2;
    element->c = 3;
    TEST_EQUALS(element->a + element->b + element->c, 6);

    fragment_free(&fragment);
}

static void test_fragment_t_independent_elements() {
    FragmentMemoryT<FragmentTElement> fragment = {0};
    fragment_alloc(&fragment, 4, 4);

    FragmentTElement* a = fragment_memory_get(&fragment);
    FragmentTElement* b = fragment_memory_get(&fragment);

    a->id = 1;
    a->value = 100;
    b->id = 2;
    b->value = 200;

    TEST_EQUALS(a->id, 1);
    TEST_EQUALS(a->value, 100);
    TEST_EQUALS(b->id, 2);
    TEST_EQUALS(b->value, 200);

    // Overwriting a neighbor must not touch the other element
    b->id = 3;
    TEST_EQUALS(a->id, 1);
    TEST_EQUALS(a->value, 100);

    fragment_free(&fragment);
}

static void test_fragment_t_free_array_no_overlap() {
    // The free index array must live behind the element memory, a wrong offset
    // (element pointer arithmetic instead of bytes) would let elements overwrite it
    FragmentMemoryT<FragmentTElement> fragment = {0};
    fragment_alloc(&fragment, 4, 4);

    int32 free_before[4];
    memcpy(free_before, fragment.free, sizeof(free_before));

    // Write into every element
    for (int32 i = 0; i < 4; ++i) {
        fragment.memory[i].id = 0x01020304 + i;
        fragment.memory[i].value = 0x05060708 + i;
    }

    TEST_MEMORY_EQUALS(fragment.free, free_before, sizeof(free_before));
    TEST_EQUALS(fragment.free[0], 0);
    TEST_EQUALS(fragment.free[3], 3);

    // The elements are still intact
    TEST_EQUALS(fragment.memory[0].id, 0x01020304);
    TEST_EQUALS(fragment.memory[3].value, 0x05060708 + 3);

    // The element memory and the free array don't overlap
    TEST_TRUE((uintptr_t) fragment.free >= (uintptr_t) fragment.memory + 4 * sizeof(FragmentTElement));

    fragment_free(&fragment);
}

static void test_fragment_t_capacity_boundary() {
    // A single element pool
    FragmentMemoryT<FragmentTElement> single = {0};
    fragment_alloc(&single, 1, 4);

    TEST_EQUALS(single.capacity, 1);
    TEST_EQUALS(single.last_pos, 0);

    FragmentTElement* only = fragment_memory_get(&single);
    TEST_EQUALS(only, &single.memory[0]);
    TEST_EQUALS(single.last_pos, -1);
    TEST_EQUALS(fragment_memory_get(&single), NULL);

    fragment_release_memory(&single, only);
    TEST_EQUALS(fragment_memory_get(&single), only);
    TEST_EQUALS(fragment_memory_get(&single), NULL);

    fragment_free(&single);

    // The capacity, not the max capacity, defines the available elements
    FragmentMemoryT<FragmentTElement> fragment = {0};
    fragment_alloc(&fragment, 2, 16);

    TEST_EQUALS(fragment.capacity, 2);
    TEST_EQUALS(fragment.last_pos, 1);

    TEST_NOT_EQUALS(fragment_memory_get(&fragment), NULL);
    TEST_NOT_EQUALS(fragment_memory_get(&fragment), NULL);
    TEST_EQUALS(fragment_memory_get(&fragment), NULL);

    fragment_free(&fragment);
}

static void test_fragment_t_arena() {
    MemoryArena* root = mem_arena_alloc(8192, 8192);

    FragmentMemoryT<FragmentTElement> fragment = {0};
    fragment_alloc(&fragment, root, 8, 8);

    TEST_NOT_EQUALS(fragment.memory, NULL);
    TEST_EQUALS(fragment.capacity, 8);
    TEST_EQUALS(fragment.last_pos, 7);

    // The fragment memory is the user memory of a new arena in the chain
    bool inside_chain = false;
    for (MemoryArena* mem = root; mem; mem = mem->next) {
        inside_chain = inside_chain || fragment.memory == (FragmentTElement *) mem->memory;
    }
    TEST_TRUE(inside_chain);
    TEST_EQUALS(mem_arena_stats(root).count, 2);

    // The arena backed fragment memory behaves like the platform backed one
    FragmentTElement* first = fragment_memory_get(&fragment);
    TEST_EQUALS(first, &fragment.memory[7]);
    first->id = 11;
    first->value = 110;

    fragment_release_memory(&fragment, first);
    TEST_EQUALS(fragment_memory_get(&fragment), first);
    TEST_EQUALS(first->id, 11);
    TEST_EQUALS(first->value, 110);

    fragment_free(&fragment, root);

    // The arena node is released with it
    TEST_EQUALS(fragment.memory, NULL);
    TEST_EQUALS(fragment.capacity, 0);
    TEST_EQUALS(fragment.last_pos, -1);
    TEST_EQUALS(mem_arena_stats(root).count, 1);

    // The root is still usable
    root->memory[0] = 9;
    TEST_EQUALS(root->memory[0], 9);

    void* base = root->base;
    platform_aligned_free(&base);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main MemoryFragmentMemoryTTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_fragment_t_alloc);
    TEST_RUN(test_fragment_t_memory_get);
    TEST_RUN(test_fragment_t_get_empty);
    TEST_RUN(test_fragment_t_release);
    TEST_RUN(test_fragment_t_round_trip);
    TEST_RUN(test_fragment_t_release_order);
    TEST_RUN(test_fragment_t_free);
    TEST_RUN(test_fragment_t_alignment);
    TEST_RUN(test_fragment_t_independent_elements);
    TEST_RUN(test_fragment_t_free_array_no_overlap);
    TEST_RUN(test_fragment_t_capacity_boundary);
    TEST_RUN(test_fragment_t_arena);

    TEST_FINALIZE();

    return 0;
}

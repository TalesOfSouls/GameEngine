#include "../TestFramework.h"
#include "../../memory/Heap.h"

/**
 * Tests for memory/Heap.h
 *
 * Heap is a max heap (the compare function orders the elements, heap_peek() is the
 * biggest element). The element memory is allocated with one additional element slot
 * which is used as scratch memory by heap_swap().
 */
static int32 heap_compare_int(const void* a, const void* b) {
    const int32 va = *(const int32 *) a;
    const int32 vb = *(const int32 *) b;

    return va > vb ? 1 : (va < vb ? -1 : 0);
}

static void test_heap_alloc() {
    Heap heap = {0};
    heap_alloc(&heap, sizeof(int32), 10, 20, heap_compare_int);

    TEST_NOT_EQUALS(heap.elements, NULL);
    TEST_EQUALS(heap.element_size, sizeof(int32));
    TEST_EQUALS(heap.capacity, 10);
    TEST_EQUALS(heap.size, 0);
    TEST_EQUALS(heap.compare, heap_compare_int);

    // The scratch memory is the element behind the last heap element
    TEST_NOT_EQUALS(heap.helper_mem, NULL);
    TEST_EQUALS(heap.helper_mem, heap.elements + heap.element_size * heap.capacity);
    TEST_TRUE(heap.helper_mem >= heap.elements + heap.capacity * heap.element_size);
    TEST_TRUE(heap.helper_mem < heap.elements + (heap.capacity + 1) * heap.element_size);

    heap_free(&heap);
}

static void test_heap_free() {
    Heap heap = {0};
    heap_alloc(&heap, sizeof(int64), 4, 8, heap_compare_int);
    TEST_NOT_EQUALS(heap.elements, NULL);

    const int64 value = 1;
    heap_push(&heap, &value);
    TEST_EQUALS(heap.size, 1);

    heap_free(&heap);

    // The platform allocation is released and the metadata is reset
    TEST_EQUALS(heap.elements, NULL);
    TEST_EQUALS(heap.helper_mem, NULL);
    TEST_EQUALS(heap.size, 0);
    TEST_EQUALS(heap.capacity, 0);
}

static void test_heap_push_peek() {
    Heap heap = {0};
    heap_alloc(&heap, sizeof(int32), 8, 8, heap_compare_int);

    const int32 value = 42;
    heap_push(&heap, &value);

    TEST_EQUALS(heap.size, 1);
    TEST_EQUALS(*(int32 *) heap_peek(&heap), 42);

    heap_free(&heap);
}

static void test_heap_push_pop_order() {
    Heap heap = {0};
    heap_alloc(&heap, sizeof(int32), 16, 16, heap_compare_int);

    const int32 values[] = {3, 1, 4, 1, 5, 9, 2, 6};
    for (uint32 i = 0; i < ARRAY_COUNT(values); ++i) {
        heap_push(&heap, &values[i]);
    }

    TEST_EQUALS(heap.size, ARRAY_COUNT(values));

    // A max heap pops in descending order
    const int32 expected[] = {9, 6, 5, 4, 3, 2, 1, 1};
    for (uint32 i = 0; i < ARRAY_COUNT(expected); ++i) {
        int32 out = 0;
        heap_pop(&heap, &out);

        TEST_EQUALS(out, expected[i]);
        TEST_EQUALS(heap.size, ARRAY_COUNT(expected) - 1 - i);
    }

    TEST_EQUALS(heap.size, 0);

    heap_free(&heap);
}

static void test_heap_push_capacity() {
    Heap heap = {0};
    heap_alloc(&heap, sizeof(int32), 4, 8, heap_compare_int);

    const int32 values[] = {10, 20, 30, 40, 50};
    for (uint32 i = 0; i < ARRAY_COUNT(values); ++i) {
        heap_push(&heap, &values[i]);
    }

    // The heap is full and silently ignores further pushes
    TEST_EQUALS(heap.size, 4);
    TEST_EQUALS(*(int32 *) heap_peek(&heap), 40);

    // A pop returns the biggest element and reopens one slot
    int32 out = 0;
    heap_pop(&heap, &out);
    TEST_EQUALS(out, 40);
    TEST_EQUALS(heap.size, 3);

    const int32 next = 50;
    heap_push(&heap, &next);
    TEST_EQUALS(heap.size, 4);
    TEST_EQUALS(*(int32 *) heap_peek(&heap), 50);

    heap_free(&heap);
}

static void test_heap_pop_empty() {
    Heap heap = {0};
    heap_alloc(&heap, sizeof(int32), 4, 4, heap_compare_int);

    int32 out = -1;
    heap_pop(&heap, &out);
    TEST_EQUALS(out, -1);
    TEST_EQUALS(heap.size, 0);

    heap_free(&heap);
}

static void test_heap_push_pop_mixed() {
    Heap heap = {0};
    heap_alloc(&heap, sizeof(int32), 8, 8, heap_compare_int);

    const int32 a[] = {7, 2, 9};
    for (uint32 i = 0; i < ARRAY_COUNT(a); ++i) {
        heap_push(&heap, &a[i]);
    }

    int32 out = 0;
    heap_pop(&heap, &out);
    TEST_EQUALS(out, 9);

    const int32 b[] = {9, 1, 8};
    for (uint32 i = 0; i < ARRAY_COUNT(b); ++i) {
        heap_push(&heap, &b[i]);
    }

    TEST_EQUALS(heap.size, 5);

    const int32 expected[] = {9, 8, 7, 2, 1};
    for (uint32 i = 0; i < ARRAY_COUNT(expected); ++i) {
        heap_pop(&heap, &out);
        TEST_EQUALS(out, expected[i]);
    }

    TEST_EQUALS(heap.size, 0);

    heap_free(&heap);
}

static void test_heap_swap() {
    // heap_swap is part of the API and is also used by heapify_up/down
    int32 a = 1;
    int32 b = 2;
    int32 helper = 0;

    heap_swap(&a, &b, sizeof(int32), &helper);

    TEST_EQUALS(a, 2);
    TEST_EQUALS(b, 1);

    // The scratch memory keeps the original value of the first argument
    TEST_EQUALS(helper, 1);

    // Swapping a block with itself must not change anything
    // (the scratch buffer has to be as big as the swapped block)
    int32 block[4] = {3, 4, 5, 6};
    int32 block_helper[4] = {0};
    heap_swap(block, block, sizeof(block), block_helper);
    TEST_EQUALS(block[0], 3);
    TEST_EQUALS(block[1], 4);
    TEST_EQUALS(block[2], 5);
    TEST_EQUALS(block[3], 6);
}

static void test_heap_init() {
    BufferMemory buffer = {0};
    buffer_alloc(&buffer, 1024, 1024);

    Heap heap = {0};
    heap_init(&heap, &buffer, sizeof(int32), 8, heap_compare_int);

    TEST_NOT_EQUALS(heap.elements, NULL);
    TEST_EQUALS(heap.element_size, sizeof(int32));
    TEST_EQUALS(heap.capacity, 8);
    TEST_EQUALS(heap.size, 0);
    TEST_EQUALS(heap.compare, heap_compare_int);

    // The elements and the scratch element are carved out of the buffer
    TEST_TRUE(heap.elements >= buffer.memory);
    TEST_TRUE(heap.elements + (heap.capacity + 1) * heap.element_size <= buffer.end);
    TEST_EQUALS(heap.helper_mem, heap.elements + heap.element_size * heap.capacity);

    // Several swaps are required for this order, so the helper memory is in use
    const int32 values[] = {4, 2, 8, 1, 9, 5};
    for (uint32 i = 0; i < ARRAY_COUNT(values); ++i) {
        heap_push(&heap, &values[i]);
    }

    TEST_EQUALS(heap.size, ARRAY_COUNT(values));
    TEST_EQUALS(*(int32 *) heap_peek(&heap), 9);

    const int32 expected[] = {9, 8, 5, 4, 2, 1};
    for (uint32 i = 0; i < ARRAY_COUNT(expected); ++i) {
        int32 out = 0;
        heap_pop(&heap, &out);
        TEST_EQUALS(out, expected[i]);
    }

    TEST_EQUALS(heap.size, 0);

    buffer_free(&buffer);
}

static void test_heap_peek() {
    Heap heap = {0};
    heap_alloc(&heap, sizeof(int32), 8, 8, heap_compare_int);

    const int32 values[] = {5, 30, 20};
    for (uint32 i = 0; i < ARRAY_COUNT(values); ++i) {
        heap_push(&heap, &values[i]);
    }

    // peek always returns the root slot
    TEST_EQUALS(heap_peek(&heap), heap.elements);
    TEST_EQUALS(*(int32 *) heap_peek(&heap), 30);

    // peek doesn't consume the element
    TEST_EQUALS(heap.size, 3);
    TEST_EQUALS(*(int32 *) heap_peek(&heap), 30);

    heap_free(&heap);
}

static void test_heap_duplicates() {
    Heap heap = {0};
    heap_alloc(&heap, sizeof(int32), 8, 8, heap_compare_int);

    const int32 values[] = {7, 7, 7, 3, 3};
    for (uint32 i = 0; i < ARRAY_COUNT(values); ++i) {
        heap_push(&heap, &values[i]);
    }

    // Duplicates are kept and don't break the ordering
    const int32 expected[] = {7, 7, 7, 3, 3};
    for (uint32 i = 0; i < ARRAY_COUNT(expected); ++i) {
        int32 out = 0;
        heap_pop(&heap, &out);
        TEST_EQUALS(out, expected[i]);
    }

    TEST_EQUALS(heap.size, 0);

    heap_free(&heap);
}

#ifdef UBER_TEST
    #ifdef main
        #undef main
    #endif
    #define main MemoryHeapTest
#endif

int main() {
    TEST_INIT(64);

    TEST_RUN(test_heap_alloc);
    TEST_RUN(test_heap_free);
    TEST_RUN(test_heap_push_peek);
    TEST_RUN(test_heap_push_pop_order);
    TEST_RUN(test_heap_push_capacity);
    TEST_RUN(test_heap_pop_empty);
    TEST_RUN(test_heap_push_pop_mixed);
    TEST_RUN(test_heap_swap);
    TEST_RUN(test_heap_init);
    TEST_RUN(test_heap_peek);
    TEST_RUN(test_heap_duplicates);

    TEST_FINALIZE();

    return 0;
}

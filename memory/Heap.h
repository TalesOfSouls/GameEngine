/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MEMORY_HEAP_H
#define COMS_MEMORY_HEAP_H

#include <stdio.h>
#include <string.h>
#include "../stdlib/Types.h"
#include "../log/DebugMemory.h"
#include "BufferMemory.h"
#include "../system/Allocator.h"

struct Heap {
    byte* elements;
    byte* helper_mem;
    uint32 element_size;
    uint64 capacity;
    uint64 size;
    int32 (*compare) (const void*, const void*);
};

void heap_alloc(Heap* heap, uint32 element_size, uint64 capacity, int32 (*compare)(const void*, const void*)) {
    ASSERT_SIMPLE(element_size * capacity);

    heap->elements = (byte *) platform_alloc(element_size * capacity + element_size);
    if (!heap->elements) {
        return;
    }

    heap->element_size = element_size;
    heap->capacity = capacity;
    heap->size = 0;
    heap->compare = compare;
    heap->helper_mem = heap->elements + element_size;
}

void heap_free(Heap* heap)
{
    DEBUG_MEMORY_DELETE((uintptr_t) heap->elements, heap->element_size * heap->capacity);
    platform_free((void **) &heap->elements);
}

void heap_init(Heap* heap, BufferMemory* buf, uint32 element_size, uint64 capacity, int32 (*compare)(const void*, const void*)) {
    ASSERT_SIMPLE(element_size * capacity);

    heap->elements = buffer_get_memory(buf, element_size * capacity, 8, true);
    if (!heap->elements) {
        return;
    }

    heap->element_size = element_size;
    heap->capacity = capacity;
    heap->size = 0;
    heap->compare = compare;
}

void heapify_down(Heap* heap, uint64 index) {
    uint64 left_child = 2 * index + 1;
    uint64 right_child = left_child + 1;
    uint64 largest = index;

    void* largest_element = heap->elements + (largest * heap->element_size);
    if (left_child < heap->size) {
        void* left = heap->elements + (left_child * heap->element_size);
        if (heap->compare(left, largest_element) > 0) {
            largest = left_child;
        }
    }

    if (right_child < heap->size) {
        void* right = heap->elements + (right_child * heap->element_size);
        void* current_largest = heap->elements + (largest * heap->element_size);
        if (heap->compare(right, current_largest) > 0) {
            largest = right_child;
        }
    }

    if (largest != index) {
        heap_swap(
            heap->elements + (index * heap->element_size),
            heap->elements + (largest * heap->element_size),
            heap->element_size, heap->helper_mem
        );

        heapify_down(heap, largest);
    }
}

void heapify_up(Heap* heap, uint64 index) {
    if (index == 0) {
        return; // Root node
    }

    uint64 parent_index = (index - 1) / 2;
    void* current = heap->elements + (index * heap->element_size);
    void* parent = heap->elements + (parent_index * heap->element_size);

    if (heap->compare(current, parent) > 0) {
        heap_swap(current, parent, heap->element_size, heap->helper_mem);
        heapify_up(heap, parent_index);
    }
}

void heap_push(Heap* heap, const void* element) {
    if (heap->size >= heap->capacity) {
        return;
    }

    void* target = heap->elements + (heap->size * heap->element_size);
    memcpy(target, element, heap->element_size);
    heapify_up(heap, heap->size);
    ++heap->size;
}

void heap_pop(Heap* heap, void* out) {
    if (heap->size == 0) {
        return;
    }

    DEBUG_MEMORY_READ((uintptr_t) heap->elements, heap->element_size);

    memcpy(out, heap->elements, heap->element_size);
    void* last_element = heap->elements + ((heap->size - 1) * heap->element_size);
    memcpy(heap->elements, last_element, heap->element_size);
    --heap->size;
    heapify_down(heap, 0);
}

inline
void* heap_peek(Heap* heap) {
    DEBUG_MEMORY_READ((uintptr_t) heap->elements, heap->element_size);

    return heap->elements;
}

inline
void heap_swap(void* a, void* b, uint32 size, void* helper_mem) {
    memcpy(helper_mem, a, size);
    memcpy(a, b, size);
    memcpy(b, helper_mem, size);
}

#endif
#ifndef COMS_SORT_HEAP_SORT_H
#define COMS_SORT_HEAP_SORT_H

#include "../stdlib/Types.h"
#include "../utils/Utils.h"
#include "SortDefine.h"

void heapsort(void* arr, size_t num, size_t size, SortCompareFunc compare) NO_EXCEPT {
    char* base = (char*)arr;

    // Build a max heap
    for (size_t i = num / 2; i > 0; --i) {
        size_t parent = i - 1;
        size_t child = 2 * parent + 1;
        while (child < num) {
            if (child + 1 < num && compare(base + child * size, base + (child + 1) * size) < 0) {
                child++;
            }

            if (compare(base + parent * size, base + child * size) >= 0) {
                break;
            }

            intrin_swap_memory(base + parent * size, base + child * size, size);
            parent = child;
            child = 2 * parent + 1;
        }
    }

    // Extract elements from the heap
    for (size_t i = num - 1; i > 0; --i) {
        intrin_swap_memory(base, base + i * size, size);
        size_t parent = 0;
        size_t child = 1;
        while (child < i) {
            if (child + 1 < i && compare(base + child * size, base + (child + 1) * size) < 0) {
                child++;
            }

            if (compare(base + parent * size, base + child * size) >= 0) {
                break;
            }

            intrin_swap_memory(base + parent * size, base + child * size, size);
            parent = child;
            child = 2 * parent + 1;
        }
    }
}

void heapsort_small(void* arr, size_t num, size_t size, SortCompareFunc compare) NO_EXCEPT {
    char* base = (char*)arr;

    // Build a max heap
    for (size_t i = num / 2; i > 0; --i) {
        size_t parent = i - 1;
        size_t child = 2 * parent + 1;
        while (child < num) {
            if (child + 1 < num && compare(base + child * size, base + (child + 1) * size) < 0) {
                child++;
            }

            if (compare(base + parent * size, base + child * size) >= 0) {
                break;
            }

            swap_memory(base + parent * size, base + child * size, size);
            parent = child;
            child = 2 * parent + 1;
        }
    }

    // Extract elements from the heap
    for (size_t i = num - 1; i > 0; --i) {
        swap_memory(base, base + i * size, size);
        size_t parent = 0;
        size_t child = 1;
        while (child < i) {
            if (child + 1 < i && compare(base + child * size, base + (child + 1) * size) < 0) {
                child++;
            }

            if (compare(base + parent * size, base + child * size) >= 0) {
                break;
            }

            swap_memory(base + parent * size, base + child * size, size);
            parent = child;
            child = 2 * parent + 1;
        }
    }
}

#endif
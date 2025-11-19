#ifndef COMS_SORT_QUICK_SORT_H
#define COMS_SORT_QUICK_SORT_H

#include "../stdlib/Types.h"
#include "../utils/Utils.h"
#include "SortDefine.h"

size_t quicksort_partition(void* arr, size_t size, size_t low, size_t high, SortCompareFunc compare) NO_EXCEPT {
    char* base = (char*) arr;
    void* pivot = base + high * size;
    size_t i = low;

    for (size_t j = low; j < high; ++j) {
        if (compare(base + j * size, pivot) < 0) {
            intrin_swap_memory(base + i * size, base + j * size, size);
            ++i;
        }
    }

    intrin_swap_memory(base + i * size, pivot, size);

    return i;
}

void quicksort(void* arr, size_t size, size_t low, size_t high, SortCompareFunc compare) NO_EXCEPT {
    if (low < high) {
        size_t pi = quicksort_partition(arr, size, low, high, compare);

        if (pi > 0) {
            // Sort the left subarray
            quicksort(arr, size, low, pi - 1, compare);
        }

        // Sort the right subarray
        quicksort(arr, size, pi + 1, high, compare);
    }
}

size_t quicksort_partition_small(void* arr, size_t size, size_t low, size_t high, SortCompareFunc compare) NO_EXCEPT {
    char* base = (char*) arr;
    void* pivot = base + high * size;
    size_t i = low;

    for (size_t j = low; j < high; ++j) {
        if (compare(base + j * size, pivot) < 0) {
            swap_memory(base + i * size, base + j * size, size);
            ++i;
        }
    }

    swap_memory(base + i * size, pivot, size);
    return i;
}

void quicksort_small(void* arr, size_t size, size_t low, size_t high, SortCompareFunc compare) NO_EXCEPT {
    if (low < high) {
        size_t pi = quicksort_partition_small(arr, size, low, high, compare);

        if (pi > 0) {
            // Sort the left subarray
            quicksort_small(arr, size, low, pi - 1, compare);
        }

        // Sort the right subarray
        quicksort_small(arr, size, pi + 1, high, compare);
    }
}

#endif
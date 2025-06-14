#ifndef COMS_SORT_QUICK_SORT_H
#define COMS_SORT_QUICK_SORT_H

#include "../stdlib/Types.h"
#include "../utils/Utils.h"

size_t quicksort_partition(void* arr, size_t size, size_t low, size_t high, int32 (*compare)(const void* __restrict, const void* __restrict)) noexcept {
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

void quicksort(void* arr, size_t size, size_t low, size_t high, int32 (*compare)(const void* __restrict, const void* __restrict)) noexcept {
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

#endif
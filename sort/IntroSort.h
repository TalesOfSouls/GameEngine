#ifndef COMS_SORT_INTRO_SORT_H
#define COMS_SORT_INTRO_SORT_H

#include "../stdlib/Types.h"
#include "InsertionSort.h"
#include "HeapSort.h"
#include "QuickSort.h"

void introsort(void* arr, size_t num, size_t size, int32 (*compare)(const void* __restrict, const void* __restrict), size_t depth_limit) noexcept {
    byte* base = (byte*) arr;

    // Use InsertionSort for small subarrays
    if (num < 16) {
        insertionsort(arr, num, size, compare);
        return;
    }

    // If the depth limit is reached, switch to HeapSort
    if (depth_limit == 0) {
        heapsort(arr, num, size, compare);
        return;
    }

    // Otherwise, perform QuickSort
    size_t pi = quicksort_partition(arr, size, 0, num - 1, compare);
    if (pi > 0) {
        // Sort the left subarray
        introsort(arr, pi, size, compare, depth_limit - 1);
    }

    if (pi + 1 < num) {
        // Sort the right subarray
        introsort(base + (pi + 1) * size, num - (pi + 1), size, compare, depth_limit - 1);
    }
}

#endif
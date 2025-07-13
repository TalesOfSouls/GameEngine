#ifndef COMS_SORT_H
#define COMS_SORT_H

#include "../stdlib/Types.h"
#include "QuickSort.h"
#include "HeapSort.h"
#include "IntroSort.h"
#include "InsertionSort.h"

inline
void sort_introsort(void* arr, size_t num, size_t size, int32 (*compare)(const void* __restrict, const void* __restrict) noexcept) noexcept {
    size_t depth_limit = 0;
    for (size_t n = num; n > 0; n >>= 1) {
        ++depth_limit;
    }

    depth_limit *= 2;

    introsort(arr, num, size, compare, depth_limit);
}

inline
void sort_quicksort(void* arr, size_t num, size_t size, int32 (*compare)(const void* __restrict, const void* __restrict) noexcept) noexcept {
    quicksort(arr, size, 0, num - 1, compare);
}

#define sort_heapsort heapsort
#define sort_insertionsort insertionsort

int32 sort_compare_int32(const void* __restrict a, const void* __restrict b) noexcept {
    return (*(int32 *) a) - (*(int32 *) b);
}

#endif
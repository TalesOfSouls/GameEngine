#ifndef COMS_SORT_H
#define COMS_SORT_H

#include "../stdlib/Types.h"
#include "SortDefine.h"
#include "QuickSort.h"
#include "HeapSort.h"
#include "IntroSort.h"
#include "InsertionSort.h"

/**
 * The _small functions use a quicker memory swap than for smaller values (up to 16 bytes per value)
 * Arrays with values of 32 bytes should use the version without _small
 * Array with values of 16 bytes should be profiled
 */

inline
void sort_introsort(void* arr, size_t num, size_t size, SortCompareFunc compare) NO_EXCEPT {
    size_t depth_limit = 0;
    for (size_t n = num; n > 0; n >>= 1) {
        ++depth_limit;
    }

    depth_limit *= 2;

    introsort(arr, num, size, compare, depth_limit);
}

inline
void sort_quicksort(void* arr, size_t num, size_t size, SortCompareFunc compare) NO_EXCEPT {
    quicksort(arr, size, 0, num - 1, compare);
}

#define sort_heapsort heapsort
#define sort_insertionsort insertionsort

inline
void sort_introsort_small(void* arr, size_t num, size_t size, SortCompareFunc compare) NO_EXCEPT {
    size_t depth_limit = 0;
    for (size_t n = num; n > 0; n >>= 1) {
        ++depth_limit;
    }

    depth_limit *= 2;

    introsort_small(arr, num, size, compare, depth_limit);
}

inline
void sort_quicksort_small(void* arr, size_t num, size_t size, SortCompareFunc compare) NO_EXCEPT {
    quicksort_small(arr, size, 0, num - 1, compare);
}

#define sort_heapsort_small heapsort_small
#define sort_insertionsort_small insertionsort_small


int32 sort_compare_int32(const void* __restrict a, const void* __restrict b) NO_EXCEPT {
    return (*(int32 *) a) - (*(int32 *) b);
}

#endif
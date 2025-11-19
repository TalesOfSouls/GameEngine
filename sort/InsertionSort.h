#ifndef COMS_SORT_Insertion_SORT_H
#define COMS_SORT_Insertion_SORT_H

#include "../stdlib/Types.h"
#include "../utils/Utils.h"
#include "SortDefine.h"

void insertionsort(void* arr, size_t num, size_t size, SortCompareFunc compare) NO_EXCEPT {
    char* base = (char*) arr;
    for (size_t i = 1; i < num; ++i) {
        for (size_t j = i; j > 0 && compare(base + j * size, base + (j - 1) * size) < 0; --j) {
            intrin_swap_memory(base + j * size, base + (j - 1) * size, size);
        }
    }
}

void insertionsort_small(void* arr, size_t num, size_t size, SortCompareFunc compare) NO_EXCEPT {
    char* base = (char*) arr;
    for (size_t i = 1; i < num; ++i) {
        for (size_t j = i; j > 0 && compare(base + j * size, base + (j - 1) * size) < 0; --j) {
            swap_memory(base + j * size, base + (j - 1) * size, size);
        }
    }
}

#endif
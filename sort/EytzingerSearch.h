#ifndef COMS_SORT_EYTZINGER_SEARCH_H
#define COMS_SORT_EYTZINGER_SEARCH_H

#include "../stdlib/Types.h"
#include "../memory/RingMemory.h"

// @performance We could optimize eytzinger by using 1 based index
// Consider this https://en.algorithmica.org/hpc/data-structures/binary-search/

static
void eytzinger_rearrange(byte* arr, byte* temp, size_t start, size_t* index, size_t num, size_t size) noexcept {
    if (start >= num) {
        return;
    }

    // Fill the left child
    eytzinger_rearrange(arr, temp, 2 * start + 1, index, num, size);

    // Assign the current element
    memcpy(temp + start * size, arr + (*index) * size, size);
    ++(*index);

    // Fill the right child
    eytzinger_rearrange(arr, temp, 2 * start + 2, index, num, size);
}

// @performance Instead of expecting a sorted array maybe we can improve it by immediately create the eytzinger array and thus avoid one "sort"
// arr MUST be sorted by a sorting algorithm of your choice
void eytzinger_create(byte* arr, size_t num, size_t size, RingMemory* ring) {
    byte* temp = ring_get_memory(ring, size * num);

    size_t index = 0;
    eytzinger_rearrange(arr, temp, 0, &index, num, size);

    // Copy the rearranged array back to the original array
    for (size_t i = 0; i < size; ++i) {
        memcpy(arr + i * size, temp + i * size, size);
    }
}

// Example search for structs with an int id
// WARNING: the prefetching is usually only useful, if we go into 256KB array size
/*
void* search_eytzinger(MyStruct* arr, size_t num, int32 target_id) {
    int32 index = 0;
    while (index < num) {
        // Prefetch the left and right children
        int32 left_child = 2 * index + 1;
        int32 right_child = 2 * index + 2;

        if (left_child < num) {
            // Prefetch left child
            intrin_prefetch(&arr[left_child]);
        }
        if (right_child < num) {
            // Prefetch right child
            intrin_prefetch(&arr[right_child]);
        }

        // Check the current node
        if (arr[index].id == target_id) {
            return &arr[index];
        } else if (arr[index].id < target_id) {
            // Move to the right child
            index = right_child;
        } else {
            // Move to the left child
            index = left_child;
        }
    }

    return NULL;
}
*/

#endif
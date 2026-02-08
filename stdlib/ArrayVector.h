/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_STDLIB_ARRAY_VECTOR_H
#define COMS_STDLIB_ARRAY_VECTOR_H

#include "Stdlib.h"

template<typename T>
struct ArrayVector {
    int size;
    int count;
    T* elements;
};

template<typename T>
void array_vector_alloc(ArrayVector<T>* vec, int size, int alignment = sizeof(size_t)) NO_EXCEPT
{
    vec->size = size;
    vec->elements = (T *) platform_alloc_aligned(size * sizeof(T), size * sizeof(T), alignment);
}

template<typename T>
inline
void array_vector_free(ArrayVector<T>* vec) NO_EXCEPT
{
    platform_aligned_free(&vec->elements);
    vec->size = 0;
}

template<typename T>
FORCE_INLINE
int array_vector_insert(ArrayVector<T>* vec, T element) NO_EXCEPT
{
    if (vec->size <= vec->count) {
        return -1;
    }

    int index = vec->count;
    vec->elements[index] = element;

    ++vec->count;

    return index;
}

template<typename T>
FORCE_INLINE
T array_vector_get(ArrayVector<T>* vec, int index) NO_EXCEPT
{
    return vec->elements[index];
}

#define array_vector_iterate_start(vec, element_ptr) {   \
    for (int _i; i < vec->count; ++_i) {                 \
        element_ptr = vec->elements + _i;

#define array_vector_iterate_end }}

/*
struct ArrayVector {
    int size;
    int count;
    int element_size;
    byte* elements;
};

void array_vector_alloc(ArrayVector* vec, int element_size, int size, int alignment = sizeof(size_t)) NO_EXCEPT
{
    vec->size = size;
    vec->element_size = element_size;
    vec->elements = (byte *) platform_alloc_aligned(size * element_size, size * element_size, alignment);
}

inline
void array_vector_free(ArrayVector* vec) NO_EXCEPT
{
    platform_aligned_free(&vec->elements);
    vec->size = 0;
}

FORCE_INLINE
int array_vector_insert(ArrayVector* __restrict vec, void* __restrict element) NO_EXCEPT
{
    if (vec->size <= vec->count) {
        return -1;
    }

    memcpy(vec->elements + vec->element_size * vec->count, element, vec->element_size);

    int index = vec->count;
    ++vec->count;

    return index;
}

FORCE_INLINE
byte* array_vector_get(ArrayVector* vec, int index) NO_EXCEPT
{
    return vec->elements + index * vec->element_size;
}

#define array_vector_iterate_start(vec, element_ptr) {   \
    for (int _i; i < vec->count; ++_i) {                 \
        element_ptr = vec->elements + vec->element_size * _i;

#define array_vector_iterate_end }}
*/

#endif
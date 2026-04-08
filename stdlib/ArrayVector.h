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
    int32 capacity;
    int32 max_capacity;
    int32 count;
    T* elements;
};

template<typename T>
FORCE_INLINE
void array_vector_alloc(ArrayVector<T>* vec, int capacity, int max_capacity, int alignment = sizeof(size_t)) NO_EXCEPT
{
    vec->capacity = capacity;
    vec->max_capacity = max_capacity;
    vec->elements = (T *) platform_alloc_aligned(capacity * sizeof(T), max_capacity * sizeof(T), alignment);
}

template<typename T>
FORCE_INLINE
void array_vector_free(ArrayVector<T>* vec) NO_EXCEPT
{
    platform_aligned_free(&vec->elements);
    vec->capacity = 0;
}

template<typename T>
FORCE_INLINE
void array_vector_init(ArrayVector<T>* vec, BufferMemory* buf, int capacity, int alignment = sizeof(size_t)) NO_EXCEPT
{
    vec->capacity = capacity;
    vec->elements = (T *) buffer_memory_get(buf, capacity, alignment);
}

template<typename T>
FORCE_INLINE
void array_vector_insert(ArrayVector<T>* vec, T element) NO_EXCEPT
{
    vec->elements[vec->count++] = element;
}

template<typename T>
FORCE_INLINE
void array_vector_insert(ArrayVector<T>* vec, T* element) NO_EXCEPT
{
    memcpy(vec->elements + vec->count, element, sizeof(T));
    ++vec->count;
}

template<typename T>
FORCE_INLINE
void array_vector_insert(ArrayVector<T>* vec, T* element, int count) NO_EXCEPT
{
    memcpy(vec->elements + vec->count, element, count * sizeof(T));
    vec->count += count;
}

template<typename T>
FORCE_INLINE
bool array_vector_insert_safe(ArrayVector<T>* vec, T element) NO_EXCEPT
{
    if (vec->count >= vec->capacity) {
        return false;
    }

    vec->elements[vec->count++] = element;

    return true;
}

template<typename T>
FORCE_INLINE
bool array_vector_insert_safe(ArrayVector<T>* vec, T* element) NO_EXCEPT
{
    if (vec->count >= vec->capacity) {
        return false;
    }

    memcpy(vec->elements + vec->count, element, sizeof(T));
    ++vec->count;

    return true;
}

template<typename T>
FORCE_INLINE
bool array_vector_insert_safe(ArrayVector<T>* vec, T* element, int count) NO_EXCEPT
{
    if (vec->count >= vec->capacity) {
        return false;
    }

    memcpy(vec->elements + vec->count, element, count * sizeof(T));
    vec->count += count;

    return true;
}

template<typename T>
FORCE_INLINE
T array_vector_get(ArrayVector<T>* vec, int index) NO_EXCEPT
{
    return vec->elements[index];
}

template<typename T>
FORCE_INLINE
void array_vector_reset(ArrayVector<T>* vec) NO_EXCEPT
{
    vec->count = 0;;
}

#define array_vector_iterate_start(vec, element_ptr) {   \
    for (int _i = 0; _i < vec.count; ++_i) {                 \
        element_ptr = vec.elements + _i;

#define array_vector_iterate_ptr_start(vec, element_ptr) {   \
    for (int _i = 0; _i < vec->count; ++_i) {                 \
        element_ptr = vec->elements + _i;

#define array_vector_iterate_end }}

#endif
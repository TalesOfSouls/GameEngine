/**
 * Shuffles bits for floats which can significantly improve the compression rate.
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_COMPRESSION_BIT_SHUFFLE_H
#define COMS_COMPRESSION_BIT_SHUFFLE_H

#include "../stdlib/Stdlib.h"

static FORCE_INLINE
void bit_shuffle_bit_set(byte* buffer, size_t bit_index, int value) NO_EXCEPT
{
    size_t byte_index = bit_index >> 3;
    unsigned int bit_in_byte = 7u - (unsigned int)(bit_index & 7u);

    if (value) {
        buffer[byte_index] |= (byte) (1u << bit_in_byte);
    } else {
        buffer[byte_index] &= (byte) ~(1u << bit_in_byte);
    }
}

static FORCE_INLINE
int bit_shuffle_bit_get(const byte* buffer, size_t bit_index) NO_EXCEPT
{
    size_t byte_index = bit_index >> 3;
    unsigned int bit_in_byte = 7u - (unsigned int)(bit_index & 7u);

    return (buffer[byte_index] >> bit_in_byte) & 1u;
}

void bit_shuffle(
    const byte* __restrict input,
    byte* __restrict output, // needs to be zeroed
    size_t elements,
    size_t element_size = sizeof(f32) // could also be f64
) NO_EXCEPT
{
    const size_t total_bits_per_element = element_size * 8;

    size_t plane;
    size_t element;
    size_t output_bit = 0;

    for (plane = 0; plane < total_bits_per_element; ++plane) {
        for (element = 0; element < elements; ++element) {
            size_t input_bit = element * total_bits_per_element + plane;

            const int bit = bit_shuffle_bit_get(input, input_bit);
            bit_shuffle_bit_set(output, output_bit, bit);

            ++output_bit;
        }
    }
}

void bit_unshuffle(
    const byte* __restrict input,
    byte* __restrict output, // needs to be zeroed
    size_t elements,
    size_t element_size = sizeof(f32)
) NO_EXCEPT
{
    const size_t total_bits_per_element = element_size * 8;

    size_t plane;
    size_t element;

    size_t input_bit = 0;

    for (plane = 0; plane < total_bits_per_element; ++plane) {
        for (element = 0; element < elements; ++element) {
            const int bit = bit_shuffle_bit_get(input, input_bit);

            const size_t output_bit = element * total_bits_per_element + plane;
            bit_shuffle_bit_set(output, output_bit, bit);

            ++input_bit;
        }
    }
}

#endif
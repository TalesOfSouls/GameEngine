/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef TOS_MEMORY_ELEMENT_MEMORY_H
#define TOS_MEMORY_ELEMENT_MEMORY_H

#include <string.h>
#include "../stdlib/Types.h"
#include "MathUtils.h"

struct ChunkMemory {
    byte* memory;

    uint64 count;
    uint64 chunk_size;
    uint64 last_pos = -1;

    // length = count
    // free describes which locations are used and which are free
    // @performance using uint32 or even uint64 might be faster
    //      since we can check for free elements faster if the memory is almost filled
    //      at the moment we can only check 8 elements at a time
    uint64* free;
};

inline
byte* chunk_get_memory(ChunkMemory* buf, uint64 element)
{
    return buf->memory + element * buf->chunk_size;
}

/**
 * In some cases we know exactly which index is free
 */
void chunk_reserve_index(ChunkMemory* buf, int64 index, int elements = 1, bool zeroed = false)
{
    int byte_index = index / 64;
    int bit_index = index % 64;

    // Mark the bits as reserved
    for (int j = 0; j < elements; ++j) {
        int current_byte_index = byte_index + (bit_index + j) / 64;
        int current_bit_index = (bit_index + j) % 64;
        buf->free[current_byte_index] |= (1 << current_bit_index);
    }

    if (zeroed) {
        memset(buf->memory + index * buf->chunk_size, 0, elements * buf->chunk_size);
    }
}

int64 chunk_reserve(ChunkMemory* buf, int elements = 1, bool zeroed = false)
{
    int64 byte_index = (buf->last_pos + 1) / 64;
    int bit_index;

    int64 free_element = -1;
    byte mask;

    int i = 0;
    while (free_element < 0 && i < (buf->count + 7) / 64) {
        ++i;

        if (buf->free[byte_index] == 0xFF) {
            ++byte_index;

            continue;
        }

        // @performance There is some redundancy happening down below, we should ++byte_index in certain conditions?
        for (bit_index = 0; bit_index < 64; ++bit_index) {
            int consecutive_free_bits = 0;

            // Check if there are 'elements' consecutive free bits
            for (int j = 0; j < elements; ++j) {
                int current_byte_index = byte_index + (bit_index + j) / 64;
                int current_bit_index = (bit_index + j) % 64;

                if (current_byte_index >= (buf->count + 7) / 64) {
                    break;
                }

                mask = 1 << current_bit_index;
                if ((buf->free[current_byte_index] & mask) == 0) {
                    ++consecutive_free_bits;
                } else {
                    break;
                }
            }

            if (consecutive_free_bits == elements) {
                free_element = byte_index * 64 + bit_index;

                // Mark the bits as reserved
                for (int j = 0; j < elements; ++j) {
                    int current_byte_index = byte_index + (bit_index + j) / 64;
                    int current_bit_index = (bit_index + j) % 64;
                    buf->free[current_byte_index] |= (1 << current_bit_index);
                }

                break;
            }
        }

        ++i;
        ++byte_index;
    }

    if (free_element < 0) {
        return -1;
    }

    if (zeroed) {
        memset(buf->memory + free_element * buf->chunk_size, 0, elements * buf->chunk_size);
    }

    return free_element;
}

byte* chunk_find_free(ChunkMemory* buf)
{
    int byte_index = (buf->last_pos + 1) / 64;
    int bit_index;

    int64 free_element = -1;
    byte mask;

    int i = 0;
    int max_loop = buf->count * buf->chunk_size;

    while (free_element < 0 && i < max_loop) {
        if (buf->free[byte_index] == 0xFF) {
            ++i;
            ++byte_index;

            continue;
        }

        // This always breaks!
        // @performance on the first iteration through the buffer we could optimize this by starting at a different bit_index
        // because we know that the bit_index is based on last_pos
        for (bit_index = 0; bit_index < 64; ++bit_index) {
            mask = 1 << bit_index;
            if ((buf->free[byte_index] & mask) == 0) {
                free_element = byte_index * 64 + bit_index;
                break;
            }
        }
    }

    if (free_element < 0) {
        return NULL;
    }

    buf->free[byte_index] |= (1 << bit_index);

    return buf->memory + free_element * buf->chunk_size;
}

inline
void chunk_element_free(ChunkMemory* buf, uint64 element)
{
    int byte_index = element / 64;
    int bit_index = element % 64;

    buf->free[byte_index] &= ~(1 << bit_index);
}

#endif
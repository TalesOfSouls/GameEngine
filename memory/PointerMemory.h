/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_POINTER_MEMORY_H
#define COMS_MEMORY_POINTER_MEMORY_H

#include "../stdlib/Stdlib.h"

/**
 * This is a dangerous memory system since we could easily run out of bounds
 * The memory overflow needs to be entirely handled by the developer
 */

inline
byte* memory_get(byte** data, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    byte* out = (byte *) align_up((uintptr_t) *data, alignment);
    *data = out + size;

    return out;
}

inline
byte* memory_get(byte* data, MAYBE_UNUSED size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    PSEUDO_USE(size);
    return (byte *) align_up((uintptr_t) data, alignment);
}

#endif
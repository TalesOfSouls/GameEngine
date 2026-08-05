/**
 * @copyright Jingga
 * @license   OMS License 2.0
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

FORCE_INLINE
bool memory_resize(byte**, size_t) NO_EXCEPT
{
    return false;
}

FORCE_INLINE
byte* memory_get(byte** data, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    byte* out = (byte *) align_up((uintptr_t) *data, alignment);
    *data = out + size;

    return out;
}

FORCE_INLINE
byte* memory_get_temp(byte** data, size_t, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    return (byte *) align_up((uintptr_t) *data, alignment);
}

FORCE_INLINE
byte* memory_get(byte* data, size_t, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    return (byte *) align_up((uintptr_t) data, alignment);
}

FORCE_INLINE
byte* memory_get_temp(byte* data, size_t, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    return (byte *) align_up((uintptr_t) data, alignment);
}

#endif
/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_BUFFER_MEMORY_H
#define COMS_MEMORY_BUFFER_MEMORY_H

#include "../stdlib/Stdlib.h"
#include "../thread/Spinlock.h"

struct BufferMemory {
    byte* memory;
    byte* end;
    byte* head;

    size_t size;
    int32 alignment;

    alignas(ASSUMED_CACHE_LINE_SIZE) spinlock32 lock;
    char _pad[ASSUMED_CACHE_LINE_SIZE - sizeof(spinlock32)];
};

#endif
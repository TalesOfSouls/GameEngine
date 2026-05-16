/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_BUFFER_MEMORY_H
#define COMS_MEMORY_BUFFER_MEMORY_H

#include "../stdlib/Stdlib.h"
#include "../thread/Thread.h"

struct BufferMemory {
    byte* memory;
    byte* end;
    byte* head;

    size_t size;
    int32 alignment;

    mutex lock;
};

#endif
/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS__MEMORY_RING_MEMORY_H
#define COMS__MEMORY_RING_MEMORY_H

#include "../stdlib/Stdlib.h"
#include "../thread/Atomic.h"
#include "../thread/ThreadDefines.h"

// WARNING: Changing this structure has effects on other data structures (e.g. Queue)
// When changing make sure you understand what you are doing
// @performance In some functions we use ring memory even though the memory is instantly free again
//              AND we don't have any subsequent function calls that really use this
//              Consider to create a "small" 128 MB buffer that can be passed for local heap memory
struct RingMemory {
    byte* memory;
    byte* end;

    atomic_ptr byte* head;

    // This variable is usually only used by single producer/consumer code mostly found in threads.
    // One thread inserts elements -> updates head
    // The other thread reads elements -> updates tail
    // This code itself doesn't change this variable
    atomic_ptr byte* tail;

    size_t size;
    uint32 alignment;

    mutex lock;
};

#endif
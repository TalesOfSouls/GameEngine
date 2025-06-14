/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MEMORY_THREADED_RING_MEMORY_H
#define COMS_MEMORY_THREADED_RING_MEMORY_H

#include "RingMemory.h"
#include "../thread/Thread.h"

// @todo This is a horrible implementation. Please implement a lock free solution

struct ThreadedRingMemory {
    byte* memory;
    byte* end;

    byte* head;

    // This variable is usually only used by single producer/consumer code mostly found in threads.
    // One thread inserts elements -> updates head
    // The other thread reads elements -> updates tail
    // This code itself doesn't change this variable
    byte* tail;

    uint64 size;
    int32 alignment;

    // The ring memory ends here
    mutex lock;
};

// @bug alignment should also include the end point, not just the start

inline
void thrd_ring_alloc(ThreadedRingMemory* ring, uint64 size, int32 alignment = 64)
{
    ring_alloc((RingMemory *) ring, size, alignment);
    mutex_init(&ring->lock, NULL);
}

inline
void thrd_ring_init(ThreadedRingMemory* ring, BufferMemory* buf, uint64 size, int32 alignment = 64)
{
    ring_init((RingMemory *) ring, buf, size, alignment);
    mutex_init(&ring->lock, NULL);
}

inline
void thrd_ring_init(ThreadedRingMemory* ring, byte* buf, uint64 size, int32 alignment = 64)
{
    ring_init((RingMemory *) ring, buf, size, alignment);
    mutex_init(&ring->lock, NULL);
}

inline
void thrd_ring_free(ThreadedRingMemory* ring)
{
    ring_free((RingMemory *) ring);
    mutex_destroy(&ring->lock);
}

inline
byte* thrd_ring_calculate_position(ThreadedRingMemory* ring, uint64 size, byte aligned = 4) noexcept
{
    mutex_lock(&ring->lock);
    byte* result = ring_calculate_position((RingMemory *) ring, size, aligned);
    mutex_unlock(&ring->lock);

    return result;
}

inline
void thrd_ring_reset(ThreadedRingMemory* ring) noexcept
{
    mutex_lock(&ring->lock);
    ring_reset((RingMemory *) ring);
    mutex_unlock(&ring->lock);
}

// Moves a pointer based on the size you want to consume (new position = after consuming size)
void thrd_ring_move_pointer(ThreadedRingMemory* ring, byte** pos, uint64 size, byte aligned = 4) noexcept
{
    mutex_lock(&ring->lock);
    ring_move_pointer((RingMemory *) ring, pos, size, aligned);
    mutex_unlock(&ring->lock);
}

byte* thrd_ring_get_memory(ThreadedRingMemory* ring, uint64 size, byte aligned = 4, bool zeroed = false) noexcept
{
    mutex_lock(&ring->lock);
    byte* result = ring_get_memory((RingMemory *) ring, size, aligned, zeroed);
    mutex_unlock(&ring->lock);

    return result;
}

// Same as ring_get_memory but DOESN'T move the head
byte* thrd_ring_get_memory_nomove(ThreadedRingMemory* ring, uint64 size, byte aligned = 4, bool zeroed = false) noexcept
{
    mutex_lock(&ring->lock);
    byte* result = ring_get_memory_nomove((RingMemory *) ring, size, aligned, zeroed);
    mutex_unlock(&ring->lock);

    return result;
}

// Used if the ring only contains elements of a certain size
// This way you can get a certain element
inline
byte* thrd_ring_get_element(ThreadedRingMemory* ring, uint64 element, uint64 size) noexcept
{
    mutex_lock(&ring->lock);
    byte* result = ring_get_element((RingMemory *) ring, element, size);
    mutex_unlock(&ring->lock);

    return result;
}

/**
 * Checks if one additional element can be inserted without overwriting the tail index
 */
inline
bool thrd_ring_commit_safe(ThreadedRingMemory* ring, uint64 size, byte aligned = 4) noexcept
{
    mutex_lock(&ring->lock);
    bool result = ring_commit_safe((RingMemory *) ring, size, aligned);
    mutex_unlock(&ring->lock);

    return result;
}

inline
void thrd_ring_force_head_update(const ThreadedRingMemory* ring) noexcept
{
    _mm_clflush(ring->head);
}

inline
void thrd_ring_force_tail_update(const ThreadedRingMemory* ring) noexcept
{
    _mm_clflush(ring->tail);
}

inline
int64 thrd_ring_dump(ThreadedRingMemory* ring, byte* data) noexcept
{
    mutex_lock(&ring->lock);
    int64 result = ring_dump((RingMemory *) ring, data);
    mutex_unlock(&ring->lock);

    return result;
}

#endif
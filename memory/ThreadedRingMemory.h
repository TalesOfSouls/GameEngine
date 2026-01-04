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

FORCE_INLINE
void thrd_ring_alloc(ThreadedRingMemory* const ring, uint64 size, int32 alignment = sizeof(size_t))
{
    ring_alloc((RingMemory *) ring, size, alignment);
    mutex_init(&ring->lock, NULL);
}

FORCE_INLINE
void thrd_ring_init(ThreadedRingMemory* const ring, BufferMemory* const buf, uint64 size, int32 alignment = sizeof(size_t))
{
    ring_init((RingMemory *) ring, buf, size, alignment);
    mutex_init(&ring->lock, NULL);
}

FORCE_INLINE
void thrd_ring_init(ThreadedRingMemory* const ring, byte* buf, uint64 size, int32 alignment = sizeof(size_t))
{
    ring_init((RingMemory *) ring, buf, size, alignment);
    mutex_init(&ring->lock, NULL);
}

FORCE_INLINE
void thrd_ring_free(ThreadedRingMemory* const ring)
{
    ring_free((RingMemory *) ring);
    mutex_destroy(&ring->lock);
}

FORCE_INLINE
byte* thrd_ring_calculate_position(ThreadedRingMemory* const ring, uint64 size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    MutexGuard _guard(&ring->lock);

    return ring_calculate_position((RingMemory *) ring, size, alignment);
}

FORCE_INLINE
void thrd_ring_reset(ThreadedRingMemory* const ring) NO_EXCEPT
{
    MutexGuard _guard(&ring->lock);
    ring_reset((RingMemory *) ring);
}

// Moves a pointer based on the size you want to consume (new position = after consuming size)
FORCE_INLINE
void thrd_ring_move_pointer(ThreadedRingMemory* const ring, byte** pos, uint64 size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    MutexGuard _guard(&ring->lock);
    ring_move_pointer((RingMemory *) ring, pos, size, alignment);
}

FORCE_INLINE
byte* thrd_ring_get_memory(ThreadedRingMemory* const ring, uint64 size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    MutexGuard _guard(&ring->lock);
    return ring_get_memory((RingMemory *) ring, size, alignment);
}

FORCE_INLINE
byte* thrd_ring_grow_memory(ThreadedRingMemory* const ring, const byte* old, uint64 size_old, uint64 size_new, int32 alignment = sizeof(size_t)) {
    MutexGuard _guard(&ring->lock);
    return ring_grow_memory((RingMemory *) ring, old, size_old, size_new, alignment);
}

// Same as ring_get_memory but DOESN'T move the head
FORCE_INLINE
byte* thrd_ring_get_memory_nomove(ThreadedRingMemory* const ring, uint64 size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    MutexGuard _guard(&ring->lock);
    return ring_get_memory_nomove((RingMemory *) ring, size, alignment);
}

// Used if the ring only contains elements of a certain size
// This way you can get a certain element
FORCE_INLINE
byte* thrd_ring_get_element(ThreadedRingMemory* const ring, uint64 element, uint64 size) NO_EXCEPT
{
    MutexGuard _guard(&ring->lock);
    return ring_get_element((RingMemory *) ring, element, size);
}

/**
 * Checks if one additional element can be inserted without overwriting the tail index
 */
FORCE_INLINE
bool thrd_ring_commit_safe(ThreadedRingMemory* const ring, uint64 size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    MutexGuard _guard(&ring->lock);
    return ring_commit_safe((RingMemory *) ring, size, alignment);
}

FORCE_INLINE
void thrd_ring_force_head_update(const ThreadedRingMemory* const ring) NO_EXCEPT
{
    _mm_clflush(ring->head);
}

FORCE_INLINE
void thrd_ring_force_tail_update(const ThreadedRingMemory* const ring) NO_EXCEPT
{
    _mm_clflush(ring->tail);
}

FORCE_INLINE
int64 thrd_ring_dump(ThreadedRingMemory* const ring, byte* data) NO_EXCEPT
{
    MutexGuard _guard(&ring->lock);
    return ring_dump((RingMemory *) ring, data);
}

#endif
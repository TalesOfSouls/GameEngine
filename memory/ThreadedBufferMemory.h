/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MEMORY_THREADED_BUFFER_MEMORY_H
#define COMS_MEMORY_THREADED_BUFFER_MEMORY_H

#include "../stdlib/Types.h"
#include "../thread/Thread.h"
#include "BufferMemory.h"

struct ThreadedBufferMemory {
    byte* memory;
    byte* end;
    byte* head;

    size_t size;
    int32 alignment;
    int32 element_alignment;

    // The buffer memory ends here
    mutex lock;
};

FORCE_INLINE
void thrd_buffer_alloc(ThreadedBufferMemory* const buf, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    buffer_alloc((BufferMemory *) buf, size, alignment);
    mutex_init(&buf->lock, NULL);
}

FORCE_INLINE
void thrd_buffer_free(ThreadedBufferMemory* const buf) NO_EXCEPT
{
    buffer_free((BufferMemory *) buf);
    mutex_destroy(&buf->lock);
}

FORCE_INLINE
void thrd_buffer_init(ThreadedBufferMemory* const buf, byte* data, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    buffer_init((BufferMemory) buf, data, size, alignment);
    mutex_init(&buf->lock, NULL);
}

FORCE_INLINE
void thrd_buffer_reset(ThreadedBufferMemory* const buf) NO_EXCEPT
{
    MutexGuard _guard(&buf->lock);
    buffer_reset((BufferMemory *) buf);
}

FORCE_INLINE
byte* thrd_buffer_get_memory(ThreadedBufferMemory* const buf, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    MutexGuard _guard(&buf->lock);
    return buffer_get_memory((BufferMemory *) buf, size, alignment);
}

FORCE_INLINE
void thrd_buffer_dump(ThreadedBufferMemory* const buf, byte* data) NO_EXCEPT
{
    MutexGuard _guard(&buf->lock);
    return buffer_dump((BufferMemory *) buf, data);
}

FORCE_INLINE
int64 thrd_buffer_load(ThreadedBufferMemory* const buf, const byte* data) NO_EXCEPT
{
    MutexGuard _guard(&buf->lock);
    return buffer_load((BufferMemory *) buf, data);
}

#endif
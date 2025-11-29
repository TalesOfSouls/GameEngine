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
void thrd_buffer_alloc(ThreadedBufferMemory* buf, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    buffer_alloc((BufferMemory *) buf, size, alignment);
    mutex_init(&buf->lock, NULL);
}

FORCE_INLINE
void thrd_buffer_free(ThreadedBufferMemory* buf) NO_EXCEPT
{
    buffer_free((BufferMemory *) buf);
    mutex_destroy(&buf->lock);
}

FORCE_INLINE
void thrd_buffer_init(ThreadedBufferMemory* buf, byte* data, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    buffer_init((BufferMemory) buf, data, size, alignment);
    mutex_init(&buf->lock, NULL);
}

FORCE_INLINE
void thrd_buffer_reset(ThreadedBufferMemory* buf) NO_EXCEPT
{
    mutex_lock(&buf->lock);
    buffer_reset((BufferMemory *) buf);
    mutex_unlock(&buf->lock);
}

FORCE_INLINE
byte* thrd_buffer_get_memory(ThreadedBufferMemory* buf, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    mutex_lock(&buf->lock);
    byte* data = buffer_get_memory((BufferMemory *) buf, size, alignment);
    mutex_unlock(&buf->lock);

    return data;
}

FORCE_INLINE
void thrd_buffer_dump(ThreadedBufferMemory* buf, byte* data) NO_EXCEPT
{
    mutex_lock(&buf->lock);
    int64 size = buffer_dump((BufferMemory *) buf, data);
    mutex_unlock(&buf->lock);

    return size;
}

FORCE_INLINE
int64 thrd_buffer_load(ThreadedBufferMemory* buf, const byte* data) NO_EXCEPT
{
    mutex_lock(&buf->lock);
    int64 size = buffer_load((BufferMemory *) buf, data);
    mutex_unlock(&buf->lock);

    return size;
}

#endif
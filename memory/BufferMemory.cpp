/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_MEMORY_BUFFER_MEMORY_C
#define COMS_MEMORY_BUFFER_MEMORY_C

#include "../stdlib/Stdlib.h"
#include "../log/Log.h"
#include "../log/Stats.h"
#include "../log/PerformanceProfiler.h"
#include "../log/DebugMemory.h"
#include "../system/Allocator.h"
#include "../thread/ThreadHelper.cpp"
#include "MemoryArena.h"
#include "BufferMemory.h"

inline
void buffer_alloc(
    BufferMemory* const buf,
    size_t size,
    size_t max_size,
    int32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    PROFILE(PROFILE_BUFFER_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(size);
    ASSERT_TRUE(max_size >= size);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    size = align_up(size, ASSUMED_CACHE_LINE_SIZE);
    max_size = align_up(max_size, ASSUMED_CACHE_LINE_SIZE);
    LOG_1("[INFO] Allocating BufferMemory: %n B", {DATA_TYPE_UINT64, &size});

    buf->memory = (byte *) platform_alloc_aligned(size, max_size, start_alignment);

    buf->end = buf->memory + size;
    buf->head = buf->memory;
    buf->size = size;
    buf->alignment = alignment;

    memset(buf->memory, 0, buf->size);

    STATS_INCREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, buf->size);
}

FORCE_INLINE
void thrd_buffer_alloc(
    BufferMemory* const buf,
    size_t size,
    size_t max_size,
    int32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    buffer_alloc(buf, size, max_size, alignment, start_alignment);
    mutex_init(&buf->lock, NULL);
}

inline
void buffer_alloc(
    BufferMemory* const buf,
    MemoryArena* const mem,
    size_t size,
    size_t max_size,
    int32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    PROFILE(PROFILE_BUFFER_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(size);
    ASSERT_TRUE(max_size >= size);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    size = align_up(size, ASSUMED_CACHE_LINE_SIZE);
    max_size = align_up(max_size, ASSUMED_CACHE_LINE_SIZE);
    LOG_1("[INFO] Allocating BufferMemory: %n B", {DATA_TYPE_UINT64, &size});

    MemoryArena* arena = mem_arena_add(
        mem,
        size,
        max_size,
        start_alignment
    );
    buf->memory = arena->memory;

    buf->end = buf->memory + size;
    buf->head = buf->memory;
    buf->size = size;
    buf->alignment = alignment;

    memset(buf->memory, 0, buf->size);

    STATS_INCREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, buf->size);
}

FORCE_INLINE
void thrd_buffer_alloc(
    BufferMemory* const buf,
    MemoryArena* const mem,
    size_t size,
    size_t max_size,
    int32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    buffer_alloc(buf, mem, size, max_size, alignment, start_alignment);
    mutex_init(&buf->lock, NULL);
}

inline
void buffer_free(BufferMemory* const buf) NO_EXCEPT
{
    platform_aligned_free((void **) &buf->memory);

    buf->size = 0;
    buf->memory = NULL;
}

FORCE_INLINE
void thrd_buffer_free(BufferMemory* const buf) NO_EXCEPT
{
    buffer_free(buf);
    mutex_destroy(&buf->lock);
}

inline
void buffer_free(BufferMemory* const buf, MemoryArena* const mem) NO_EXCEPT
{
    mem_arena_remove(mem, buf->memory);
    buf->size = 0;
    buf->memory = NULL;
}

FORCE_INLINE
void thrd_buffer_free(BufferMemory* const buf, MemoryArena* const mem) NO_EXCEPT
{
    buffer_free(buf, mem);
    mutex_destroy(&buf->lock);
}

inline
void buffer_init(
    BufferMemory* const buf,
    byte* const data,
    size_t size,
    int32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    ASSERT_TRUE(size);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    buf->memory = (byte *) align_up((uintptr_t) data, start_alignment);

    buf->end = buf->memory + size;
    buf->head = buf->memory;
    buf->size = size;
    buf->alignment = alignment;

    memset(buf->memory, 0, buf->size);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

FORCE_INLINE
void thrd_buffer_init(
    BufferMemory* const buf,
    byte* data,
    size_t size,
    int32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    buffer_init(buf, data, size, alignment, start_alignment);
    mutex_init(&buf->lock, NULL);
}

FORCE_INLINE
void buffer_reset(BufferMemory* const buf) NO_EXCEPT
{
    // @bug aren't we wasting element 0 (see get_memory, we are not using 0 only next element)
    DEBUG_MEMORY_DELETE((uintptr_t) buf->memory, buf->head - buf->memory);
    buf->head = buf->memory;
}

FORCE_INLINE
void thrd_buffer_reset(BufferMemory* const buf) NO_EXCEPT
{
    MutexGuard _guard(&buf->lock);
    buffer_reset(buf);
}

inline HOT_CODE
byte* memory_get(BufferMemory* const buf, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ASSERT_TRUE(size <= buf->size);

    buf->head = (byte *) align_up((uintptr_t) buf->head, alignment);
    size = align_up(size, alignment);

    ASSERT_TRUE(buf->head + size <= buf->end);

    DEBUG_MEMORY_WRITE((uintptr_t) buf->head, size);

    byte* const offset = buf->head;
    buf->head += size;

    ASSERT_TRUE(offset);
    ASSERT_STRICT((uintptr_t) offset + size < (uintptr_t) buf->memory + buf->size);
    STATS_MAX_PERSISTENT(DEBUG_COUNTER_BUFFER_MAX_REQUEST, size);
    STATS_MAX_PERSISTENT(DEBUG_COUNTER_BUFFER_MAX_USAGE, buf->head - buf->memory);

    return offset;
}

FORCE_INLINE HOT_CODE
void memory_rewind(BufferMemory* const buf, void* head) NO_EXCEPT
{
    buf->head = (byte *) head;
}

// This resets (memory_rewind) the BufferMemory after it leaves scope
// This is usually helpful if all you need is a larger stack memory
// Why would you use this instead of a specific stack memory implementation using the heap?
// Because you maybe don't want to have too many different memory arenas
// This way you can "abuse" the BufferMemory also as a automatic stack
struct BufferStackMemory {
    BufferMemory* buffer;
    byte* head_old;

    HOT_CODE inline
    explicit BufferStackMemory(
        BufferMemory* buf,
        byte** mem,
        size_t size,
        int32 alignment = sizeof(size_t)
    ) NO_EXCEPT
    {
        this->head_old = buf->head;
        *mem = memory_get(buf, size, alignment);
        this->buffer = buf;
    }

    /**
     * This is a pseudo stack memory request
     * We basically just want to store the current buffer head
     * This is useful when we perform a bunch of subsequent buffer interactions and want to reset the position later
     */
    HOT_CODE inline
    explicit BufferStackMemory(
        BufferMemory* buf
    ) NO_EXCEPT
    {
        this->head_old = buf->head;
        this->buffer = buf;
    }

    HOT_CODE inline
    ~BufferStackMemory() NO_EXCEPT
    {
        memory_rewind(this->buffer, this->head_old);
    }
};
#define BUFFER_STACK_MEMORY(buf, mem, size, alignment) BufferStackMemory __buffer_stack_##__func__##_##__LINE__((buf), (mem), (size), (alignment))
#define BUFFER_STACK_MEMORY_START(buf) BufferStackMemory __buffer_stack_##__func__##_##__LINE__((buf))

FORCE_INLINE
byte* thrd_memory_get(BufferMemory* const buf, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    MutexGuard _guard(&buf->lock);
    return memory_get(buf, size, alignment);
}

inline
void buffer_init(
    BufferMemory* const buf,
    BufferMemory* const data,
    size_t size,
    int32 alignment = sizeof(size_t),
    int32 start_alignment = ASSUMED_CACHE_LINE_SIZE
) NO_EXCEPT
{
    ASSERT_TRUE(size);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    buf->memory = memory_get(data, size, start_alignment);

    buf->end = buf->memory + size;
    buf->head = buf->memory;
    buf->size = size;
    buf->alignment = alignment;

    memset(buf->memory, 0, buf->size);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

inline
int64 buffer_dump(const BufferMemory* const buf, byte* data) NO_EXCEPT
{
    const byte* const start = data;

    data = write_le(data, buf->size);
    data = write_le(data, (uint64) (buf->head - buf->memory));
    data = write_le(data, buf->alignment);
    data = write_le(data, (uint64) (buf->end - buf->memory));

    // All memory is handled in the buffer -> simply copy the buffer
    memcpy(data, buf->memory, buf->size);
    data += buf->size;

    return data - start;
}

FORCE_INLINE
int64 thrd_buffer_dump(BufferMemory* const buf, byte* data) NO_EXCEPT
{
    MutexGuard _guard(&buf->lock);
    return buffer_dump(buf, data);
}

inline
int64 buffer_load(BufferMemory* const buf, const byte* data) NO_EXCEPT
{
    const byte* const start = data;

    data = read_le(data, &buf->size);

    uint64 head;
    data = read_le(data, &head);
    buf->head = buf->memory + head;

    data = read_le(data, &buf->alignment);

    uint64 end;
    data = read_le(data, &end);
    buf->end = buf->memory + end;

    memcpy(buf->memory, data, buf->size);
    data += buf->size;

    return data - start;
}

FORCE_INLINE
int64 thrd_buffer_load(BufferMemory* const buf, const byte* data) NO_EXCEPT
{
    MutexGuard _guard(&buf->lock);
    return buffer_load(buf, data);
}

#define BUFFER_ELEMENT_GET(buf, struct_val) memory_get(buf, sizeof(struct_val), alignof(struct_val))

#endif
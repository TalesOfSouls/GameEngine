/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_MEMORY_BUFFER_MEMORY_H
#define COMS_MEMORY_BUFFER_MEMORY_H

#include <string.h>
#include "../stdlib/Types.h"
#include "../utils/EndianUtils.h"
#include "../utils/Assert.h"
#include "../log/Log.h"
#include "../log/Stats.h"
#include "../log/PerformanceProfiler.h"
#include "../log/DebugMemory.h"
#include "../system/Allocator.h"

struct BufferMemory {
    byte* memory;
    byte* end;
    byte* head;

    size_t size;
    int32 alignment;
    int32 element_alignment;
};

inline
void buffer_alloc(BufferMemory* buf, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    PROFILE(PROFILE_BUFFER_ALLOC, NULL, PROFILE_FLAG_SHOULD_LOG);
    ASSERT_TRUE(size);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    LOG_1("[INFO] Allocating BufferMemory: %n B", {DATA_TYPE_UINT64, &size});

    buf->memory = alignment < 2
        ? (byte *) platform_alloc(size)
        : (byte *) platform_alloc_aligned(size, alignment);

    buf->end = buf->memory + size;
    buf->head = buf->memory;
    buf->size = size;
    buf->alignment = alignment;
    buf->element_alignment = 0;

    memset(buf->memory, 0, buf->size);

    LOG_INCREMENT_BY(DEBUG_COUNTER_MEM_ALLOC, buf->size);
}

inline
void buffer_free(BufferMemory* buf) NO_EXCEPT
{
    if (buf->alignment < 2) {
        platform_free((void **) &buf->memory);
    } else {
        platform_aligned_free((void **) &buf->memory);
    }
}

inline
void buffer_init(BufferMemory* buf, byte* data, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ASSERT_TRUE(size);
    ASSERT_TRUE(alignment % sizeof(int) == 0);

    // @bug what if an alignment is defined?
    buf->memory = data;

    buf->end = buf->memory + size;
    buf->head = buf->memory;
    buf->size = size;
    buf->alignment = alignment;
    buf->element_alignment = 0;

    memset(buf->memory, 0, buf->size);

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

FORCE_INLINE
void buffer_reset(BufferMemory* buf) NO_EXCEPT
{
    // @bug aren't we wasting element 0 (see get_memory, we are not using 0 only next element)
    DEBUG_MEMORY_DELETE((uintptr_t) buf->memory, buf->head - buf->memory);
    buf->head = buf->memory;
}

inline HOT_CODE
byte* buffer_get_memory(BufferMemory* buf, size_t size, int32 alignment = sizeof(size_t)) NO_EXCEPT
{
    ASSERT_TRUE(size <= buf->size);

    buf->head = (byte *) OMS_ALIGN_UP((uintptr_t) buf->head, alignment);
    size = OMS_ALIGN_UP(size, alignment);

    ASSERT_TRUE(buf->head + size <= buf->end);

    DEBUG_MEMORY_WRITE((uintptr_t) buf->head, size);

    byte* offset = buf->head;
    buf->head += size;

    ASSERT_TRUE(offset);

    return offset;
}

inline
int64 buffer_dump(const BufferMemory* buf, byte* data) NO_EXCEPT
{
    byte* start = data;

    // Size
    *((uint64 *) data) = SWAP_ENDIAN_LITTLE(buf->size);
    data += sizeof(buf->size);

    // head
    *((uint64 *) data) = SWAP_ENDIAN_LITTLE((uint64) (buf->head - buf->memory));
    data += sizeof(uint64);

    // Alignment
    *((int32 *) data) = SWAP_ENDIAN_LITTLE(buf->alignment);
    data += sizeof(buf->alignment);

    *((int32 *) data) = SWAP_ENDIAN_LITTLE(buf->element_alignment);
    data += sizeof(buf->element_alignment);

    // End
    *((uint64 *) data) = SWAP_ENDIAN_LITTLE((uint64) (buf->end - buf->memory));
    data += sizeof(buf->end);

    // All memory is handled in the buffer -> simply copy the buffer
    memcpy(data, buf->memory, buf->size);
    data += buf->size;

    return data - start;
}

inline
int64 buffer_load(BufferMemory* buf, const byte* data) NO_EXCEPT
{
    const byte* start = data;

    // Size
    buf->size = SWAP_ENDIAN_LITTLE(*((uint64 *) data));
    data += sizeof(buf->size);

    // head
    buf->head = buf->memory + SWAP_ENDIAN_LITTLE(*((uint64 *) data));
    data += sizeof(uint64);

    // Alignment
    buf->alignment = SWAP_ENDIAN_LITTLE(*((int32 *) data));
    data += sizeof(buf->alignment);

    // End
    buf->end = buf->memory + SWAP_ENDIAN_LITTLE(*((uint64 *) data));
    data += sizeof(uint64);

    memcpy(buf->memory, data, buf->size);
    data += buf->size;

    return data - start;
}

#endif
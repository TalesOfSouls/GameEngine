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
#include "../utils/TestUtils.h"
#include "../log/Log.h"
#include "../log/Stats.h"
#include "../log/PerformanceProfiler.h"
#include "../log/DebugMemory.h"
#include "../system/Allocator.h"

// @question Consider to use element_alignment to automatically align/pad elements

struct BufferMemory {
    byte* memory;
    byte* end;
    byte* head;

    uint64 size;
    int32 alignment;
    int32 element_alignment;
};

inline
void buffer_alloc(BufferMemory* buf, uint64 size, int32 alignment = 64)
{
    ASSERT_SIMPLE(size);
    PROFILE(PROFILE_BUFFER_ALLOC, NULL, false, true);
    LOG_1("[INFO] Allocating BufferMemory: %n B", {{LOG_DATA_UINT64, &size}});

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
void buffer_free(BufferMemory* buf)
{
    if (buf->alignment < 2) {
        platform_free((void **) &buf->memory);
    } else {
        platform_aligned_free((void **) &buf->memory);
    }
}

inline
void buffer_init(BufferMemory* buf, byte* data, uint64 size, int32 alignment = 64)
{
    ASSERT_SIMPLE(size);

    // @bug what if an alignment is defined?
    buf->memory = data;

    buf->end = buf->memory + size;
    buf->head = buf->memory;
    buf->size = size;
    buf->alignment = alignment;
    buf->element_alignment = 0;

    DEBUG_MEMORY_SUBREGION((uintptr_t) buf->memory, buf->size);
}

inline
void buffer_reset(BufferMemory* buf) noexcept
{
    // @bug aren't we wasting element 0 (see get_memory, we are not using 0 only next element)
    DEBUG_MEMORY_DELETE((uintptr_t) buf->memory, buf->head - buf->memory);
    buf->head = buf->memory;
}

inline
byte* buffer_get_memory(BufferMemory* buf, uint64 size, int32 aligned = 4, bool zeroed = false) noexcept
{
    ASSERT_SIMPLE(size <= buf->size);

    if (aligned == 0) {
        aligned = (byte) OMS_MAX(buf->element_alignment, 1);
    }

    if (aligned > 1) {
        uintptr_t address = (uintptr_t) buf->head;
        buf->head += (aligned - (address & (aligned - 1))) % aligned;
    }

    size = ROUND_TO_NEAREST(size, aligned);
    ASSERT_SIMPLE(buf->head + size <= buf->end);

    if (zeroed) {
        memset((void *) buf->head, 0, size);
    }

    DEBUG_MEMORY_WRITE((uintptr_t) buf->head, size);

    byte* offset = buf->head;
    buf->head += size;

    ASSERT_SIMPLE(offset);

    return offset;
}

inline
int64 buffer_dump(const BufferMemory* buf, byte* data)
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
int64 buffer_load(BufferMemory* buf, const byte* data)
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
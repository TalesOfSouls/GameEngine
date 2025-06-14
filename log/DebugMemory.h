/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_LOG_DEBUG_MEMORY_H
#define COMS_LOG_DEBUG_MEMORY_H

#include "../stdlib/Types.h"
#include "../thread/Atomic.h"

#ifndef DEBUG_MEMORY_RANGE_MAX
    #define DEBUG_MEMORY_RANGE_MAX 500
#endif

#ifndef DEBUG_MEMORY_RANGE_RES_MAX
    #define DEBUG_MEMORY_RANGE_RES_MAX 100
#endif

struct DebugMemoryRange {
    int32 type;
    uintptr_t start;
    uint64 size;
    uint64 time;

    const char* function_name;
};

struct DebugMemory {
    uint64 usage;
    uintptr_t start;
    uint64 size;

    alignas(4) atomic_32 uint32 action_idx;
    alignas(4) atomic_32 uint32 reserve_action_idx;
    DebugMemoryRange last_action[DEBUG_MEMORY_RANGE_MAX];
    DebugMemoryRange reserve_action[DEBUG_MEMORY_RANGE_RES_MAX];
};

struct DebugMemoryContainer {
    uint32 memory_size;
    uint32 memory_element_idx;
    DebugMemory* memory_stats;
};
static DebugMemoryContainer* _dmc = NULL;

enum MemoryDebugType {
    MEMORY_DEBUG_TYPE_DELETE = -1,
    MEMORY_DEBUG_TYPE_READ = 0,
    MEMORY_DEBUG_TYPE_WRITE = 1,
    MEMORY_DEBUG_TYPE_RESERVE = 2,
    MEMORY_DEBUG_TYPE_SUBREGION = 3,
};

inline
DebugMemory* debug_memory_find(uintptr_t start) noexcept
{
    for (uint64 i = 0; i < _dmc->memory_size; ++i) {
        if (_dmc->memory_stats[i].start <= start
            && _dmc->memory_stats[i].start + _dmc->memory_stats[i].size >= start
        ) {
            return &_dmc->memory_stats[i];
        }
    }

    return NULL;
}

void debug_memory_init(uintptr_t start, uint64 size) noexcept
{
    if (!start || !_dmc || (_dmc->memory_size && !_dmc->memory_stats)) {
        return;
    }

    const DebugMemory* mem = debug_memory_find(start);
    if (mem) {
        return;
    }

    if (_dmc->memory_size <= _dmc->memory_element_idx) {
        const uint64 new_size = _dmc->memory_size + 3;
        DebugMemory* new_stats = (DebugMemory *) calloc(new_size, sizeof(DebugMemory));
        if (!new_stats) {
            return;
        }

        if (_dmc->memory_stats) {
            memcpy(new_stats, _dmc->memory_stats, _dmc->memory_size * sizeof(DebugMemory));
            free(_dmc->memory_stats);
        }

        _dmc->memory_stats = new_stats;
        _dmc->memory_size = new_size;
    }

    DebugMemory* debug_mem = &_dmc->memory_stats[_dmc->memory_element_idx];
    debug_mem->start = start;
    debug_mem->size = size;
    debug_mem->usage = 0;

    ++_dmc->memory_element_idx;
}

void debug_memory_log(uintptr_t start, uint64 size, int32 type, const char* function) noexcept
{
    if (!start || !_dmc) {
        return;
    }

    DebugMemory* mem = debug_memory_find(start);
    if (!mem) {
        return;
    }

    uint32 idx = atomic_fetch_add_relaxed(&mem->action_idx, 1);
    if (idx >= ARRAY_COUNT(mem->last_action)) {
        atomic_set_release(&mem->action_idx, 1);
        idx %= ARRAY_COUNT(mem->last_action);
    }

    DebugMemoryRange* dmr = &mem->last_action[idx];
    dmr->type = type;
    dmr->start = start - mem->start;
    dmr->size = size;

    dmr->time = intrin_timestamp_counter();
    dmr->function_name = function;

    if (type < 0 && mem->usage < size * -type) {
        mem->usage = 0;
    } else {
        mem->usage += size * type;
    }
}

void debug_memory_reserve(uintptr_t start, uint64 size, int32 type, const char* function) noexcept
{
    if (!start || !_dmc) {
        return;
    }

    DebugMemory* mem = debug_memory_find(start);
    if (!mem) {
        return;
    }

    uint32 idx = atomic_fetch_add_relaxed(&mem->reserve_action_idx, 1);
    if (idx >= ARRAY_COUNT(mem->reserve_action)) {
        atomic_set_release(&mem->reserve_action_idx, 1);
        idx %= ARRAY_COUNT(mem->reserve_action);
    }

    DebugMemoryRange* dmr = &mem->reserve_action[idx];
    dmr->type = type;
    dmr->start = start - mem->start;
    dmr->size = size;

    dmr->time = intrin_timestamp_counter();
    dmr->function_name = function;
}

// undo reserve
void debug_memory_free(uintptr_t start) noexcept
{
    if (!start || !_dmc) {
        return;
    }

    DebugMemory* mem = debug_memory_find(start);
    if (!mem) {
        return;
    }

    for (uint32 i = 0; i < ARRAY_COUNT(mem->reserve_action); ++i) {
        DebugMemoryRange* dmr = &mem->reserve_action[i];
        if (dmr->start == start - mem->start) {
            dmr->size = 0;
            return;
        }
    }
}

// @bug This probably requires thread safety
inline
void debug_memory_reset() noexcept
{
    if (!_dmc) {
        return;
    }

    // We remove debug information that are "older" than 1GHz
    uint64 time = intrin_timestamp_counter() - 1 * GHZ;

    for (uint32 i = 0; i < _dmc->memory_element_idx; ++i) {
        for (int32 j = 0; j < DEBUG_MEMORY_RANGE_MAX; ++j) {
            if (_dmc->memory_stats[i].last_action[j].time < time) {
                memset(&_dmc->memory_stats[i].last_action[j], 0, sizeof(DebugMemoryRange));
            }
        }
    }
}

#if DEBUG
    #define DEBUG_MEMORY_INIT(start, size) debug_memory_init((start), (size))
    #define DEBUG_MEMORY_READ(start, size) debug_memory_log((start), (size), MEMORY_DEBUG_TYPE_READ, __func__)
    #define DEBUG_MEMORY_WRITE(start, size) debug_memory_log((start), (size), MEMORY_DEBUG_TYPE_WRITE, __func__)
    #define DEBUG_MEMORY_DELETE(start, size) debug_memory_log((start), (size), MEMORY_DEBUG_TYPE_DELETE, __func__)
    #define DEBUG_MEMORY_RESERVE(start, size) debug_memory_reserve((start), (size), MEMORY_DEBUG_TYPE_RESERVE, __func__)
    #define DEBUG_MEMORY_SUBREGION(start, size) debug_memory_reserve((start), (size), MEMORY_DEBUG_TYPE_SUBREGION, __func__)
    #define DEBUG_MEMORY_FREE(start) debug_memory_free((start))
    #define DEBUG_MEMORY_RESET() debug_memory_reset()
#else
    #define DEBUG_MEMORY_INIT(start, size) ((void) 0)
    #define DEBUG_MEMORY_READ(start, size) ((void) 0)
    #define DEBUG_MEMORY_WRITE(start, size) ((void) 0)
    #define DEBUG_MEMORY_DELETE(start, size) ((void) 0)
    #define DEBUG_MEMORY_RESERVE(start, size) ((void) 0)
    #define DEBUG_MEMORY_SUBREGION(start, size) ((void) 0)
    #define DEBUG_MEMORY_FREE(start) ((void) 0)
    #define DEBUG_MEMORY_RESET() ((void) 0)
#endif

#endif
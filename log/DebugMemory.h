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

#include "../stdlib/Stdlib.h"
#include "../thread/Atomic.h"

#ifndef DEBUG_MEMORY_RANGE_MAX
    // How many memory actions do we store per memory arena?
    #define DEBUG_MEMORY_RANGE_MAX 500
#endif

#ifndef DEBUG_MEMORY_RANGE_RES_MAX
    // How many reserved actions do we store?
    #define DEBUG_MEMORY_RANGE_RES_MAX 100
#endif

enum MemoryDebugType : char {
    MEMORY_DEBUG_TYPE_DELETE = -1,
    MEMORY_DEBUG_TYPE_READ = 0,
    MEMORY_DEBUG_TYPE_WRITE = 1,

    // Memory got reserved
    MEMORY_DEBUG_TYPE_RESERVE = 2,

    // Subregion created (e.g. a smaller sub buffer got created)
    MEMORY_DEBUG_TYPE_SUBREGION = 3,
};

struct DebugMemoryRange {
    MemoryDebugType type;
    uintptr_t start;
    size_t size;
    uint64 time;

    const char* function_name;
};

// @todo We probably want a way to log usage per frame for a chart
struct DebugMemory {
    uintptr_t start;

    size_t usage;
    size_t size;

    atomic_32 uint32 action_idx;
    atomic_32 uint32 reserve_action_idx;
    alignas(8) DebugMemoryRange last_action[DEBUG_MEMORY_RANGE_MAX];
    alignas(8) DebugMemoryRange reserve_action[DEBUG_MEMORY_RANGE_RES_MAX];
};

struct DebugMemoryContainer {
    uint32 memory_size;
    uint32 memory_element_idx;
    DebugMemory* memory_stats;
};
static DebugMemoryContainer* _dmc = NULL;

/**
 * Tries to find a memory region for a pointer where we can add logging information.
 *
 * @param uintptr_t start Address of the pointer
 *
 * @return DebugMemory*
 */
static FORCE_INLINE
DebugMemory* debug_memory_find(uintptr_t start) NO_EXCEPT
{
    for (uint32 i = 0; i < _dmc->memory_size; ++i) {
        if (_dmc->memory_stats[i].start <= start
            && _dmc->memory_stats[i].start + _dmc->memory_stats[i].size >= start
        ) {
            return &_dmc->memory_stats[i];
        }
    }

    return NULL;
}

/**
 * Initializes a new memory region for logging.
 *
 * @param uintptr_t start   Start of the memory region
 * @param uint64    size    Size of the memory region
 *
 * @return void
 */
void debug_memory_init(uintptr_t start, size_t size) NO_EXCEPT
{
    if (!start || !_dmc || (_dmc->memory_size && !_dmc->memory_stats)) {
        return;
    }

    const DebugMemory* const mem = debug_memory_find(start);
    if (mem) {
        return;
    }

    if (_dmc->memory_size <= _dmc->memory_element_idx) {
        const uint32 new_size = _dmc->memory_size + 3;
        // @performance Can we get rid of this calloc?
        DebugMemory* const new_stats = (DebugMemory *) calloc(new_size * sizeof(DebugMemory), 64);
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

    DebugMemory* const debug_mem = &_dmc->memory_stats[_dmc->memory_element_idx];
    debug_mem->start = start;
    debug_mem->size = size;
    debug_mem->usage = 0;

    ++_dmc->memory_element_idx;
}

/**
 * Log memory usage
 *
 * @param uintptr_t         start       Memory start
 * @param uint64            size        Memory size
 * @param MemoryDebugType   type        Log type
 * @param const char*       function    Function where this memory log happened
 *
 * @return void
 */
HOT_CODE
void debug_memory_log(uintptr_t start, size_t size, MemoryDebugType type, const char* function) NO_EXCEPT
{
    if (!start || !_dmc) {
        return;
    }

    DebugMemory* const mem = debug_memory_find(start);
    if (!mem) {
        return;
    }

    const uint32 idx = atomic_increment_wrap_relaxed(&mem->action_idx, ARRAY_COUNT(mem->last_action));

    DebugMemoryRange* const dmr = &mem->last_action[idx];
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

/**
 * Log memory reserve/subregion create
 *
 * @param uintptr_t         start       Memory start
 * @param size_t            size        Memory size
 * @param MemoryDebugType   type        Log type
 * @param const char*       function    Function where this memory log happened
 *
 * @return void
 */
void debug_memory_reserve(uintptr_t start, size_t size, MemoryDebugType type, const char* function) NO_EXCEPT
{
    if (!start || !_dmc) {
        return;
    }

    DebugMemory* const mem = debug_memory_find(start);
    if (!mem) {
        return;
    }

    const uint32 idx = atomic_increment_wrap_relaxed(
        &mem->reserve_action_idx,
        ARRAY_COUNT(mem->reserve_action)
    );

    DebugMemoryRange* const dmr = &mem->reserve_action[idx];
    dmr->type = type;
    dmr->start = start - mem->start;
    dmr->size = size;

    dmr->time = intrin_timestamp_counter();
    dmr->function_name = function;
}

/**
 * Frees the logging for a memory region e.g. when the memory region gets freed
 *
 * @param uintptr_t start Address of the memory
 *
 * @return void
 */
void debug_memory_free(uintptr_t start) NO_EXCEPT
{
    if (!start || !_dmc) {
        return;
    }

    DebugMemory* const mem = debug_memory_find(start);
    if (!mem) {
        return;
    }

    for (uint32 i = 0; i < ARRAY_COUNT(mem->reserve_action); ++i) {
        DebugMemoryRange* const dmr = &mem->reserve_action[i];
        if (dmr->start == start - mem->start) {
            dmr->size = 0;
            return;
        }
    }

    // @todo move over memory ranges and
}

/**
 * Reset the memory logs "older" than 1 GHZ
 *
 * @return void
 */
inline
void debug_memory_reset() NO_EXCEPT
{
    if (!_dmc) {
        return;
    }

    // We remove debug information that are "older" than 1GHz
    const uint64 time = intrin_timestamp_counter() - 1 * GHZ;

    for (uint32 i = 0; i < _dmc->memory_element_idx; ++i) {
        const int32 last = _dmc->memory_stats[i].action_idx;
        int32 idx = last;

        for (int32 j = 0; j < DEBUG_MEMORY_RANGE_MAX; ++j) {
            // @bug This probably requires thread safety
            // last could be updated while we loop
            if (_dmc->memory_stats[i].last_action[idx].time < time) {
                if (idx <= last) {
                    memset(
                        &_dmc->memory_stats[i].last_action[0], 0,
                        sizeof(DebugMemoryRange) * (idx + 1)
                    );

                    memset(
                        &_dmc->memory_stats[i].last_action[last + 1], 0,
                        sizeof(DebugMemoryRange) * (ARRAY_COUNT(_dmc->memory_stats[i].last_action) - (idx + 1))
                    );
                } else {
                    memset(
                        &_dmc->memory_stats[i].last_action[last + 1], 0,
                        sizeof(DebugMemoryRange) * (idx - last)
                    );
                }

                break;
            }

            idx = OMS_WRAPPED_DECREMENT(idx, DEBUG_MEMORY_RANGE_MAX);
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
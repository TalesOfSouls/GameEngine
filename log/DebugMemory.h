/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_LOG_DEBUG_MEMORY_H
#define COMS_LOG_DEBUG_MEMORY_H

#include "../stdlib/Stdlib.h"
#include "../thread/Atomic.h"

#ifndef DEBUG_MEMORY_RANGE_MAX
    // How many memory actions do we store per memory arena?
    #define DEBUG_MEMORY_RANGE_MAX 500
#endif

#ifndef DEBUG_MEMORY_RANGE_PERS_MAX
    // How many persistent actions do we store?
    // Persistent actions are "create subregion", "mark memory as in use"
    #define DEBUG_MEMORY_RANGE_PERS_MAX 32
#endif

enum MemoryDebugType : char {
    MEMORY_DEBUG_TYPE_DELETE = -1,
    // We use the value of the enum for calculations later on
    // For that reason it is important that read has value 0 = no size change
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

struct DebugMemory {
    atomic<int64> usage;
    atomic<int64> max_usage;
    atomic<uint32> action_idx;
    atomic<uint32> persistent_action_idx;

    uintptr_t start;
    size_t size;
    const char* name;

    // These actions are only stored temporarily until overwritten
    alignas(8) DebugMemoryRange last_action[DEBUG_MEMORY_RANGE_MAX];

    // Persistent actions are actions that are relevant at all times
    // These actions also get modified unlike the last_action which only get added or removed/overwritten
    // This allows us also to define sub-regions in a memory arena
    alignas(8) DebugMemoryRange persistent_action[DEBUG_MEMORY_RANGE_PERS_MAX];
};

#ifndef DEBUG_MEMORY_MAX_ALLOC
    #define DEBUG_MEMORY_MAX_ALLOC 16
#endif
struct DebugMemoryContainer {
    atomic<int32> is_active;
    atomic<uint32> memory_element_idx;
    DebugMemory* memory_stats[DEBUG_MEMORY_MAX_ALLOC];
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
    const uint32 memory_element_idx = _dmc->memory_element_idx.load();
    for (uint32 i = 0; i < memory_element_idx; ++i) {
        if (_dmc->memory_stats[i]->start <= start
            && _dmc->memory_stats[i]->start + _dmc->memory_stats[i]->size > start
        ) {
            return _dmc->memory_stats[i];
        }
    }

    return NULL;
}

/**
 * Initializes a new memory region for logging.
 *
 * @param uintptr_t start   Start of the memory region
 * @param size_t    size    Size of the memory region
 *
 * @return void
 */
void debug_memory_init(uintptr_t start, size_t size) NO_EXCEPT
{
    if (!start || !_dmc) {
        return;
    }

    const DebugMemory* const mem = debug_memory_find(start);
    if (mem) {
        return;
    }

    // @performance Can we get rid of this calloc?
    DebugMemory* const new_stats = (DebugMemory *) calloc(sizeof(DebugMemory), ASSUMED_CACHE_LINE_SIZE);
    if (!new_stats) {
        return;
    }

    const int32 is_active = _dmc->is_active.load();
    _dmc->is_active.store(0);
    _dmc->memory_stats[_dmc->memory_element_idx.fetch_add(1)] = new_stats;

    DebugMemory* const debug_mem = new_stats;
    debug_mem->start = start;
    debug_mem->size = size;
    debug_mem->usage.store(0);
    debug_mem->max_usage.store(0);

    _dmc->is_active.store(is_active);
}

/**
 * Names a memory range by a memory in the memory range
 */
void debug_memory_name(const char* __restrict name, const void* const __restrict addr) NO_EXCEPT
{
    if (!addr || !_dmc) {
        return;
    }

    DebugMemory* mem = debug_memory_find((uintptr_t) addr);
    if (!mem) {
        return;
    }

    mem->name = name;
}

/**
 * Log memory usage
 *
 * @param uintptr_t         start       Memory start
 * @param size_t            size        Memory size
 * @param MemoryDebugType   type        Log type
 * @param const char*       function    Function where this memory log happened
 *
 * @return void
 */
HOT_CODE
void debug_memory_log(uintptr_t start, size_t size, MemoryDebugType type, const char* const function) NO_EXCEPT
{
    if (!start || !_dmc || !_dmc->is_active.load()) {
        return;
    }

    DebugMemory* const mem = debug_memory_find(start);
    if (!mem) {
        return;
    }

    const uint32 action_idx = atomic_increment_wrap_acquire_release(
        mem->action_idx,
        (uint32) ARRAY_COUNT(mem->last_action)
    );

    DebugMemoryRange* const dmr = &mem->last_action[action_idx];
    dmr->type = type;
    dmr->start = start - mem->start;
    dmr->size = size;
    dmr->time = intrin_timestamp_counter();
    dmr->function_name = function;

    const int64 usage = mem->usage.fetch_add(size * type);
    mem->max_usage.store(OMS_MAX(usage, mem->max_usage.load()));
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
void debug_memory_persistent(uintptr_t start, size_t size, MemoryDebugType type, const char* const function) NO_EXCEPT
{
    if (!start || !_dmc || !_dmc->is_active.load()) {
        return;
    }

    DebugMemory* const mem = debug_memory_find(start);
    if (!mem) {
        return;
    }

    // We will most likely overwrite subregions in due time
    // It is what it is
    const uint32 action_idx = atomic_increment_wrap_acquire_release(
        mem->persistent_action_idx,
        (uint32) ARRAY_COUNT(mem->persistent_action)
    );

    DebugMemoryRange* const dmr = &mem->persistent_action[action_idx];
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
    if (!start || !_dmc || !_dmc->is_active.load()) {
        return;
    }

    DebugMemory* const mem = debug_memory_find(start);
    if (!mem) {
        return;
    }

    for (int i = 0; i < ARRAY_COUNT(mem->persistent_action); ++i) {
        DebugMemoryRange* const dmr = &mem->persistent_action[i];
        if (dmr->start == start - mem->start) {
            dmr->size = 0;
            return;
        }
    }
}

#if defined(DEBUG) && DEBUG
    #define DEBUG_MEMORY_INIT(start, size) debug_memory_init((start), (size))
    #define DEBUG_MEMORY_NAME(name, addr) debug_memory_name((name), (addr))
    #define DEBUG_MEMORY_READ(start, size) debug_memory_log((start), (size), MEMORY_DEBUG_TYPE_READ, __func__)
    #define DEBUG_MEMORY_WRITE(start, size) debug_memory_log((start), (size), MEMORY_DEBUG_TYPE_WRITE, __func__)
    #define DEBUG_MEMORY_DELETE(start, size) debug_memory_log((start), (size), MEMORY_DEBUG_TYPE_DELETE, __func__)
    #define DEBUG_MEMORY_RESERVE(start, size) debug_memory_persistent((start), (size), MEMORY_DEBUG_TYPE_RESERVE, __func__)
    #define DEBUG_MEMORY_SUBREGION(start, size) debug_memory_persistent((start), (size), MEMORY_DEBUG_TYPE_SUBREGION, __func__)
    #define DEBUG_MEMORY_FREE(start) debug_memory_free((start))
#else
    #define DEBUG_MEMORY_INIT(start, size) ((void) 0)
    #define DEBUG_MEMORY_NAME(name, addr) ((void) 0)
    #define DEBUG_MEMORY_READ(start, size) ((void) 0)
    #define DEBUG_MEMORY_WRITE(start, size) ((void) 0)
    #define DEBUG_MEMORY_DELETE(start, size) ((void) 0)
    #define DEBUG_MEMORY_RESERVE(start, size) ((void) 0)
    #define DEBUG_MEMORY_SUBREGION(start, size) ((void) 0)
    #define DEBUG_MEMORY_FREE(start) ((void) 0)
#endif

#endif
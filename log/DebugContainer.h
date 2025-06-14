/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_LOG_DEBUG_CONTAINER_H
#define COMS_LOG_DEBUG_CONTAINER_H

#include "Log.h"
#include "DebugMemory.h"
#include "PerformanceProfiler.h"
#include "Stats.h"

struct DebugContainer {
    DebugMemoryContainer* dmc;

    #if _WIN32
        HANDLE log_fp;
    #elif __linux__
        int32 log_fp;
    #endif
    LogMemory* log_memory;

    alignas(8) atomic_64 int64* stats_counter;

    PerformanceProfileResult* perf_stats;
    PerformanceProfiler** perf_current_scope;
    int32* perf_active;
};

#endif
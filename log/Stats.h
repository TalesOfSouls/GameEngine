
#ifndef COMS_LOG_STATS_H
#define COMS_LOG_STATS_H

#include "../stdlib/Types.h"
#include "../thread/Atomic.h"

// @question See PerformanceProfiler (hashmap) and implement same here
// The problem with that is, the hash map is much slower
// and we probably want to maybe use this (at least partially) in release mode?
#ifndef DEBUG_COUNTER
    #define DEBUG_COUNTER 1
    enum DebugCounter {
        DEBUG_COUNTER_MEM_ALLOC,

        DEBUG_COUNTER_DRIVE_READ,
        DEBUG_COUNTER_DRIVE_WRITE,

        DEBUG_COUNTER_THREAD,

        DEBUG_COUNTER_GPU_VERTEX_UPLOAD,
        DEBUG_COUNTER_GPU_UNIFORM_UPLOAD,
        DEBUG_COUNTER_GPU_DRAW_CALLS,
        DEBUG_COUNTER_GPU_DOWNLOAD,

        DEBUG_COUNTER_NETWORK_OUT_RAW,
        DEBUG_COUNTER_NETWORK_IN_RAW,

        DEBUG_COUNTER_SIZE
    };
#endif

static atomic_64 int64* _stats_counter = NULL;

inline
void reset_counter(int32 id) noexcept
{
    if (!_stats_counter) {
        return;
    }

    atomic_set_release(&_stats_counter[id], 0);
}

inline
void log_increment(int32 id, int64 by = 1) noexcept
{
    if (!_stats_counter) {
        return;
    }

    atomic_add_acquire(&_stats_counter[id], by);
}

inline
void log_decrement(int32 id, int64 by = 1) noexcept
{
    if (!_stats_counter) {
        return;
    }

    atomic_sub_acquire(&_stats_counter[id], by);
}

inline
void log_counter(int32 id, int64 value) noexcept
{
    if (!_stats_counter) {
        return;
    }

    atomic_set_release(&_stats_counter[id], value);
}

#if (!DEBUG && !INTERNAL) || RELEASE
    #define LOG_INCREMENT(a) ((void) 0)
    #define LOG_INCREMENT_BY(a, b) ((void) 0)
    #define LOG_DECREMENT(a) ((void) 0)
    #define LOG_COUNTER(a, b) ((void) 0)
    #define RESET_COUNTER(a) ((void) 0)
#else
    #define LOG_INCREMENT(a) log_increment((a), 1)
    #define LOG_INCREMENT_BY(a, b) log_increment((a), (b))
    #define LOG_DECREMENT(a) log_decrement((a), 1)
    #define LOG_COUNTER(a, b) log_counter((a), (b))
    #define RESET_COUNTER(a) reset_counter((a))
#endif

#endif
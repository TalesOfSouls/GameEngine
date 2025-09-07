
#ifndef COMS_LOG_STATS_H
#define COMS_LOG_STATS_H

#include "../stdlib/Types.h"
#include "../thread/Atomic.h"

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

#define MAX_STATS_COUNTER_HISTORY 1000
struct StatCounterHistory {
    atomic_32 int32 pos;
    atomic_64 int64 stats[MAX_STATS_COUNTER_HISTORY * DEBUG_COUNTER_SIZE];
};
static StatCounterHistory* _stats_counter = NULL;
static volatile int32* _stats_counter_active = NULL;

FORCE_INLINE
void stats_snapshot() {
    if (!_stats_counter_active || !*_stats_counter_active) {
        return;
    }

    int32 pos = atomic_increment_wrap_acquire_release(&_stats_counter->pos, MAX_STATS_COUNTER_HISTORY);
    memset(
        (void *) &_stats_counter->stats[pos * DEBUG_COUNTER_SIZE],
        0,
        DEBUG_COUNTER_SIZE * sizeof(int64)
    );
}

FORCE_INLINE
void log_increment(int32 id, int64 by = 1) NO_EXCEPT
{
    if (!_stats_counter_active || !*_stats_counter_active) {
        return;
    }

    int32 pos = atomic_get_acquire(&_stats_counter->pos) * DEBUG_COUNTER_SIZE;
    atomic_add_relaxed(&_stats_counter->stats[pos + id], by);
}

FORCE_INLINE
void log_decrement(int32 id, int64 by = 1) NO_EXCEPT
{
    if (!_stats_counter_active || !*_stats_counter_active) {
        return;
    }

    int32 pos = atomic_get_acquire(&_stats_counter->pos) * DEBUG_COUNTER_SIZE;
    atomic_sub_relaxed(&_stats_counter->stats[pos + id], by);
}

FORCE_INLINE
void log_counter(int32 id, int64 value) NO_EXCEPT
{
    if (!_stats_counter_active || !*_stats_counter_active) {
        return;
    }

    int32 pos = atomic_get_acquire(&_stats_counter->pos) * DEBUG_COUNTER_SIZE;
    atomic_set_relaxed(&_stats_counter->stats[pos + id], value);
}

#if (!DEBUG && !INTERNAL) || RELEASE
    #define LOG_INCREMENT(a) ((void) 0)
    #define LOG_INCREMENT_BY(a, b) ((void) 0)
    #define LOG_DECREMENT(a) ((void) 0)
    #define LOG_COUNTER(a, b) ((void) 0)

    #define STATS_SNAPSHOT() ((void) 0)
#else
    #define LOG_INCREMENT(a) log_increment((a), 1)
    #define LOG_INCREMENT_BY(a, b) log_increment((a), (b))
    #define LOG_DECREMENT(a) log_decrement((a), 1)
    #define LOG_COUNTER(a, b) log_counter((a), (b))

    #define STATS_SNAPSHOT() stats_snapshot()
#endif

#endif

#ifndef COMS_LOG_STATS_H
#define COMS_LOG_STATS_H

#include "../stdlib/Stdlib.h"
#include "../thread/Atomic.h"

#ifndef DEBUG_COUNTER
    // Defining the standard counter stat types if not already defined by the user
    #define DEBUG_COUNTER 1
    enum DebugCounter {
        DEBUG_COUNTER_MEM_ALLOC,
        DEBUG_COUNTER_STACK_ALLOC,

        DEBUG_COUNTER_DRIVE_READ,
        DEBUG_COUNTER_DRIVE_WRITE,

        DEBUG_COUNTER_THREAD,

        // How many threads are active, not just initialized
        DEBUG_COUNTER_THREAD_ACTIVE,

        DEBUG_COUNTER_GPU_UPLOAD,
        DEBUG_COUNTER_GPU_DRAW_CALLS,
        DEBUG_COUNTER_GPU_DOWNLOAD,

        // Network byte transfer
        DEBUG_COUNTER_NETWORK_OUT_RAW,
        DEBUG_COUNTER_NETWORK_IN_RAW,

        // Network packet transfer count
        DEBUG_COUNTER_NETWORK_OUT_COUNT,
        DEBUG_COUNTER_NETWORK_IN_COUNT,

        DEBUG_COUNTER_SIZE
    };
#endif

#define MAX_STATS_COUNTER_HISTORY 1000
struct StatCounterHistory {
    atomic_32 int32 pos;
    atomic_64 int64 stats[MAX_STATS_COUNTER_HISTORY * DEBUG_COUNTER_SIZE];
};
static StatCounterHistory* _stats_counter = NULL;
static int32* _stats_counter_active = NULL;

/**
 * Creates a snapshot of the current stats
 *
 * @return void
 */
FORCE_INLINE
void stats_snapshot() NO_EXCEPT
{
    if (!_stats_counter_active || !*_stats_counter_active) {
        return;
    }

    const int32 pos = atomic_increment_wrap_acquire_release(&_stats_counter->pos, MAX_STATS_COUNTER_HISTORY);
    memset(
        (void *) &_stats_counter->stats[pos * DEBUG_COUNTER_SIZE],
        0,
        DEBUG_COUNTER_SIZE * sizeof(int64)
    );
}

/**
 * Increments a counter variable
 *
 * @param int32 id  Stats id
 * @param int64 by  Change amount
 *
 * @return void
 */
inline HOT_CODE
void stats_increment(int32 id, int64 by = 1) NO_EXCEPT
{
    if (!_stats_counter_active || !*_stats_counter_active) {
        return;
    }

    const int32 pos = atomic_get_acquire(&_stats_counter->pos) * DEBUG_COUNTER_SIZE;
    atomic_add_relaxed(&_stats_counter->stats[pos + id], by);
}

/**
 * Decrements a counter variable
 *
 * @param int32 id  Stats id
 * @param int64 by  Change amount
 *
 * @return void
 */
inline HOT_CODE
void stats_decrement(int32 id, int64 by = 1) NO_EXCEPT
{
    if (!_stats_counter_active || !*_stats_counter_active) {
        return;
    }

    const int32 pos = atomic_get_acquire(&_stats_counter->pos) * DEBUG_COUNTER_SIZE;
    atomic_sub_relaxed(&_stats_counter->stats[pos + id], by);
}

/**
 * Sets a counter variable
 *
 * @param int32 id      Stats id
 * @param int64 value   New value
 *
 * @return void
 */
inline HOT_CODE
void stats_counter(int32 id, int64 value) NO_EXCEPT
{
    if (!_stats_counter_active || !*_stats_counter_active) {
        return;
    }

    const int32 pos = atomic_get_acquire(&_stats_counter->pos) * DEBUG_COUNTER_SIZE;
    atomic_set_relaxed(&_stats_counter->stats[pos + id], value);
}

/**
 * Sets a counter variable
 *
 * @param int32 id      Stats id
 * @param int64 value   New value
 *
 * @return void
 */
inline HOT_CODE
void stats_max(int32 id, int64 value) NO_EXCEPT
{
    if (!_stats_counter_active || !*_stats_counter_active) {
        return;
    }

    const int32 pos = atomic_get_acquire(&_stats_counter->pos) * DEBUG_COUNTER_SIZE;
    atomic_set_relaxed(&_stats_counter->stats[pos + id], OMS_MAX(_stats_counter->stats[pos + id], value));
}

/**
 * Sets a counter variable
 *
 * @param int32 id      Stats id
 * @param int64 value   New value
 *
 * @return void
 */
inline HOT_CODE
void stats_min(int32 id, int64 value) NO_EXCEPT
{
    if (!_stats_counter_active || !*_stats_counter_active) {
        return;
    }

    const int32 pos = atomic_get_acquire(&_stats_counter->pos) * DEBUG_COUNTER_SIZE;
    atomic_set_relaxed(&_stats_counter->stats[pos + id], OMS_MIN(_stats_counter->stats[pos + id], value));
}

/**
 * Logs the stats to the logger,
 * which may output the data to a file if the buffer is filled
 *
 * @return void
 */
inline
void stats_log_to_file() NO_EXCEPT
{
    // we don't log an empty log pool
    if (!_stats_counter_active) {
        return;
    }

    MAYBE_UNUSED size_t count = DEBUG_COUNTER_SIZE;
    LOG_1("[BEGIN] Stats log (count %d)", {DATA_TYPE_INT32, &count});

    MAYBE_UNUSED int32 size = sizeof(*_stats_counter);

    // Technically this isn't logging to a file, only if the end of the log buffer is reached
    LOG_1((const char *) _stats_counter, {DATA_TYPE_BYTE_ARRAY, &size});

    LOG_1("[END] Stats log");
}

#if (!DEBUG && !INTERNAL) || RELEASE
    #define STATS_INCREMENT(a) ((void) 0)
    #define STATS_INCREMENT_BY(a, b) ((void) 0)
    #define STATS_DECREMENT(a) ((void) 0)
    #define STATS_DECREMENT_BY(a, b) ((void) 0)
    #define STATS_COUNTER(a, b) ((void) 0)
    #define STATS_MAX(a, b) ((void) 0)
    #define STATS_MIN(a, b) ((void) 0)

    #define STATS_SNAPSHOT() ((void) 0)
    #define STATS_LOG_TO_FILE() ((void) 0)
#else
    #define STATS_INCREMENT(a) stats_increment((a), 1)
    #define STATS_INCREMENT_BY(a, b) stats_increment((a), (b))
    #define STATS_DECREMENT(a) stats_decrement((a), 1)
    #define STATS_DECREMENT_BY(a, b) stats_decrement((a), (b))
    #define STATS_COUNTER(a, b) stats_counter((a), (b))
    #define STATS_MAX(a, b) stats_max((a), (b))
    #define STATS_MIN(a, b) stats_min((a), (b))

    #define STATS_SNAPSHOT() stats_snapshot()
    #define STATS_LOG_TO_FILE() stats_log_to_file()
#endif

#endif
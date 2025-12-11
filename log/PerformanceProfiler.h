/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_LOG_PERFORMANCE_PROFILER_H
#define COMS_LOG_PERFORMANCE_PROFILER_H

#include "../stdlib/Types.h"
#include "../thread/Atomic.h"
#include "../architecture/Intrinsics.h"
#include "../compiler/CompilerUtils.h"
#include "../thread/ThreadDefines.h"
#include "Log.h"

#ifndef PERFORMANCE_PROFILE_STATS
    // Defining the standard timing stat types if not already defined by the user
    #define PERFORMANCE_PROFILE_STATS 1
    enum TimingStats {
        PROFILE_TEMP,

        PROFILE_SLEEP, // Used to time sleep (careful, also used in spinlocks)
        PROFILE_MEMORY_ALLOC,
        PROFILE_FILE_UTILS,
        PROFILE_BUFFER_ALLOC,
        PROFILE_CHUNK_ALLOC,
        PROFILE_RING_ALLOC,
        PROFILE_DB_POOL_ALLOC,
        PROFILE_THREAD_POOL_ALLOC,
        PROFILE_CMD_ITERATE,
        PROFILE_CMD_FONT_LOAD_SYNC,
        PROFILE_CMD_SHADER_LOAD_SYNC,
        PROFILE_CMD_LAYOUT_LOAD_SYNC,
        PROFILE_CMD_THEME_LOAD_SYNC,
        PROFILE_CMD_UI_LOAD_SYNC,
        PROFILE_CMD_ASSET_LOAD_SYNC,
        PROFILE_LAYOUT_FROM_DATA,
        PROFILE_LAYOUT_FROM_THEME,
        PROFILE_THEME_FROM_THEME,
        PROFILE_AUDIO_BUFFER_FILLABLE,
        PROFILE_AUDIO_PLAY_BUFFER,
        PROFILE_AUDIO_MIXER_MIX,
        PROFILE_ASSET_ARCHIVE_LOAD,
        PROFILE_ASSET_ARCHIVE_ASSET_LOAD,
        PROFILE_AMS_UPDATE,
        PROFILE_VERTEX_RECT_CREATE,
        PROFILE_VERTEX_TEXT_CREATE,
        PROFILE_PIPELINE_MAKE,
        PROFILE_THREADPOOL_WORK,
        PROFILE_MAIN, // Used to profile the main loop
        PROFILE_GPU, // Used to profile the total gpu time

        PROFILE_SIZE
    };
#endif

enum PerformanceProfileFlag : uint32 {
    PROFILE_FLAG_NONE = 0,

    // Stateless allows to ONLY output to log instead of storing the performance data in an array
    PROFILE_FLAG_STATELESS = 1 << 0,

    // Should be logged as well instead of just storing it in memory
    PROFILE_FLAG_SHOULD_LOG = 1 << 1,

    // Should be added to history, sometimes we don't want any historic data for a performance metric
    PROFILE_FLAG_ADD_HISTORY = 1 << 2,
};

// @question Should this store the thread_id?
struct alignas(8) PerformanceProfileResult {
    atomic_64 const char* name;

    // WARNING: rdtsc doesn't really return cycle count but we will just call it that
    atomic_64 int64 total_cycle;
    atomic_64 int64 self_cycle;

    atomic_32 uint32 counter;
    uint32 parent;
};

// If we call PROFILE_SNAPSHOT after every frame this number is the same as the amount frames can store
#define MAX_PERFORMANCE_STATS_HISTORY 1000
struct PerformanceStatHistory {
    atomic_32 int32 pos;
    // This contains all stats usually per frame in a 1D array
    alignas(8) PerformanceProfileResult perfs[MAX_PERFORMANCE_STATS_HISTORY * PROFILE_SIZE];
};
static PerformanceStatHistory* _perf_stats = NULL;
static volatile int32* _perf_active = NULL;

/**
 * Creates a snapshot of the current performance logs
 *
 * @return void
 */
FORCE_INLINE
void profile_performance_snapshot() NO_EXCEPT
{
    if (!_perf_stats || !*_perf_active) {
        return;
    }

    int32 pos = atomic_increment_wrap_acquire_release(&_perf_stats->pos, ARRAY_COUNT(_perf_stats->perfs) / PROFILE_SIZE);
    memset(
        &_perf_stats->perfs[pos * PROFILE_SIZE],
        0,
        PROFILE_SIZE * sizeof(PerformanceProfileResult)
    );
}

struct PerformanceThreadHistory {
    int32 id;
    uint64 start;
    uint64 end;
    const char* name;
};

// Used to show historic values per thread unlike PerformanceStatHistory,
// which doesn't differentiate between threads
#define MAX_PERFORMANCE_THREAD_HISTORY 10000
struct PerformanceProfileThread {
    atomic_32 int32 thread_id;
    int32 cpu_id;
    atomic_32 uint32 pos;

    // WARNING: This only shows tha last tick but when rendering the rendering thread may be way slower
    // As a result you will only output every n-th tick
    // @todo we probably want to log thread performance per frame for a chart
    uint64 tick;
    const char* name;
    PerformanceThreadHistory history[MAX_PERFORMANCE_THREAD_HISTORY];
};
// How many threads do we support?
static int32 _perf_thread_history_count = 0;
static PerformanceProfileThread* _perf_thread_history;

/**
 * Used to create a new thread history
 * Every thread needs one if you want to store historic values
 *
 * @param int32         thread_id   Id of the thread
 * @param const char*   name        Name of the thread
 *
 * @return void
 */
inline
void thread_profile_history_create(int32 thread_id, const char* name = NULL) NO_EXCEPT
{
    for (int32 i = 1; i < _perf_thread_history_count; ++i) {
        if (atomic_compare_exchange_strong_relaxed(
                &_perf_thread_history[i].thread_id,
                0,
                thread_id
            ) == 0
        ) {
            _perf_thread_history[i].thread_id = thread_id;
            _perf_thread_history[i].cpu_id = 0;
            _perf_thread_history[i].pos = 0;
            _perf_thread_history[i].name = name;
            _perf_thread_history[i].tick = 0;
            return;
        }
    }
}

/**
 * Sets the user defined name of a thread by id
 *
 * @param int32         thread_id   Id of the thread
 * @param const char*   name        Name of the thread
 *
 * @return void
 */
inline
void thread_profile_name(int32 thread_id, const char* name = NULL) NO_EXCEPT
{
    for (int32 i = 1; i < _perf_thread_history_count; ++i) {
        if (_perf_thread_history[i].thread_id == thread_id) {
            _perf_thread_history[i].name = name;
            return;
        }
    }
}

/**
 * Marks a thread as no longer available for logging
 *
 * @param int32 thread_id Id of the thread
 *
 * @return void
 */
inline
void thread_profile_history_delete(int32 thread_id) NO_EXCEPT
{
    for (int32 i = 1; i < _perf_thread_history_count; ++i) {
        if (_perf_thread_history[i].thread_id == thread_id) {
            atomic_set_relaxed(&_perf_thread_history[i].thread_id, 0);
            return;
        }
    }
}

struct PerformanceThreadProfiler {
    bool is_active;
    int32 _id;
    uint64 start_cycle;

    HOT_CODE
    PerformanceThreadProfiler(
        int32 id
    ) NO_EXCEPT
    {
        if (!_perf_active || !*_perf_active || !_perf_thread_history_count) {
            this->is_active = false;

            return;
        }

        this->is_active = true;
        this->_id = id;
        this->start_cycle = intrin_timestamp_counter();
    }

    HOT_CODE
    ~PerformanceThreadProfiler() NO_EXCEPT
    {
        if (!this->is_active) {
            return;
        }

        const uint64 end_cycle = intrin_timestamp_counter();

        for (int32 i = 0; i < _perf_thread_history_count; ++i) {
            if (_perf_thread_history[i].thread_id == this->_id) {
                _perf_thread_history[i].tick = (uint64) OMS_MAX(end_cycle - this->start_cycle, 0ULL);

                return;
            }
        }
    }
};

/**
 * Logs the performance stats to the logger,
 * which may output the data to a file if the buffer is filled
 *
 * @return void
 */
inline
void performance_log_to_file() NO_EXCEPT
{
    // we don't log an empty log pool
    if (!_perf_stats) {
        return;
    }

    MAYBE_UNUSED int32 count = PROFILE_SIZE;
    LOG_1("[BEGIN] Performance log (count %d)", {DATA_TYPE_INT32, &count});

    MAYBE_UNUSED int32 size = sizeof(*_perf_stats);

    LOG_1((const char *) _perf_stats, {DATA_TYPE_BYTE_ARRAY, (void *) &size});

    LOG_1("[END] Performance log");
}

/**
 * Logs the performance stats to the logger,
 * which may output the data to a file if the buffer is filled
 * The output format is CSV compatible
 *
 * @return void
 */
inline
void performance_log_to_file_formatted() NO_EXCEPT
{
    // we don't log an empty log pool
    if (!_perf_stats) {
        return;
    }

    MAYBE_UNUSED int32 count = PROFILE_SIZE;
    LOG_1("[BEGIN] Performance log (count %d)", {DATA_TYPE_INT32, &count});

    const int32 pos = atomic_get_acquire(&_perf_stats->pos) * PROFILE_SIZE;

    char line[512];
    for (int32 i = 0; i < PROFILE_SIZE; ++i) {
        const PerformanceProfileResult* const perf = &_perf_stats->perfs[pos + i];
        if (perf->name) {
            const int32 length = sprintf_fast(
                line, sizeof(line) - 1, "%s;%l\n",
                perf->name,
                perf->total_cycle
            );

            LOG_1((const char *) line, {DATA_TYPE_BYTE_ARRAY, (void *) &length});
        }
    }

    LOG_1("[END] Performance log");
}

struct PerformanceProfiler;
// Used when sharing profiler across dlls and threads (threads unlikely)
static thread_local PerformanceProfiler** _perf_current_scope = NULL;
// Used when in dll or thread and no shared pointer found
static thread_local PerformanceProfiler* _perf_current_scope_internal;
struct PerformanceProfiler {
    bool is_active;

    const char* name;
    const char* info_msg;
    int32 _id;
    uint32 _flags;

    int64 start_cycle;
    int64 total_cycle;
    int64 self_cycle;

    PerformanceProfiler* parent;

    HOT_CODE
    PerformanceProfiler(
        int32 id, const char* scope_name, const char* info = NULL,
        uint32 flags = 0
    ) NO_EXCEPT
    {
        if (!_perf_active || !*_perf_active) {
            this->is_active = false;

            return;
        }

        this->is_active = true;
        this->_id = id;

        this->name = scope_name;
        this->info_msg = info;
        this->_flags = flags;

        this->total_cycle = 0;
        this->self_cycle = 0;

        if (this->_flags & PROFILE_FLAG_STATELESS) {
            this->parent = NULL;
        } else if (_perf_current_scope) {
            this->parent = *_perf_current_scope;
            *_perf_current_scope = this;
        } else {
            this->parent = _perf_current_scope_internal;
            _perf_current_scope_internal = this;
        }

        this->start_cycle = intrin_timestamp_counter();
    }

    HOT_CODE
    ~PerformanceProfiler() NO_EXCEPT
    {
        if (!this->is_active) {
            return;
        }

        const uint64 end_cycle = intrin_timestamp_counter();
        this->total_cycle = (int64) OMS_MAX(end_cycle - this->start_cycle, 0ULL);
        this->self_cycle += total_cycle;

        const int32 pos = atomic_get_acquire(&_perf_stats->pos) * PROFILE_SIZE;
        atomic_increment_relaxed(&_perf_stats->perfs[pos + this->_id].counter);

        // Store result
        PerformanceProfileResult temp_perf = {};
        PerformanceProfileResult* const perf = (this->_flags & PROFILE_FLAG_STATELESS)
            ? &temp_perf
            : &_perf_stats->perfs[pos + this->_id];

        perf->name = this->name;
        perf->total_cycle = this->total_cycle;
        perf->self_cycle = this->self_cycle;

        // Add performance log to history
        if (this->_flags & PROFILE_FLAG_ADD_HISTORY && _perf_thread_history_count) {
            for (int32 i = 0; i < _perf_thread_history_count; ++i) {
                if (_perf_thread_history[i].thread_id == _thread_local_id) {
                    int32 hist_pos = atomic_increment_wrap_relaxed(
                        &_perf_thread_history[i].pos,
                        MAX_PERFORMANCE_THREAD_HISTORY
                    );

                    _perf_thread_history[i].cpu_id = _thread_cpu_id;
                    _perf_thread_history[i].history[hist_pos].id = this->_id;
                    _perf_thread_history[i].history[hist_pos].start = this->start_cycle;
                    _perf_thread_history[i].history[hist_pos].end = end_cycle;
                    _perf_thread_history[i].history[hist_pos].name = this->name;

                    break;
                }
            }
        }

        if (!(this->_flags & PROFILE_FLAG_STATELESS)) {
            if (this->parent) {
                this->parent->self_cycle -= this->total_cycle;
                perf->parent = this->parent->_id;
            }

            if (_perf_current_scope) {
                *_perf_current_scope = this->parent;
            } else {
                _perf_current_scope_internal = this->parent;
            }
        }

        if ((this->_flags & PROFILE_FLAG_STATELESS)
            || (this->_flags & PROFILE_FLAG_SHOULD_LOG)
        ) {
            if (this->info_msg && this->info_msg[0]) {
                LOG_2(
                    "[PERF] %s (%s): %n cycles",
                    {DATA_TYPE_CHAR_STR, (void *) perf->name},
                    {DATA_TYPE_CHAR_STR, (void *) this->info_msg},
                    {DATA_TYPE_INT64, (void *) &perf->total_cycle},
                );
            } else {
                LOG_2(
                    "[PERF] %s: %n cycles",
                    {DATA_TYPE_CHAR_STR, (void *) perf->name},
                    {DATA_TYPE_INT64, (void *) &perf->total_cycle},
                );
            }
        }
    }
};

/**
 * Manually start a profilling section that doesn't end when leaving scope
 *
 * @param int32         id      Profiling id
 * @param const char*   name    Profiling name
 *
 * return void
 */
inline HOT_CODE
void performance_profiler_start(int32 id, const char* name = NULL) NO_EXCEPT
{
    if (!_perf_active || !*_perf_active) {
        return;
    }

    const int32 pos = atomic_get_acquire(&_perf_stats->pos) * PROFILE_SIZE;

    PerformanceProfileResult* const perf = &_perf_stats->perfs[pos + id];
    perf->name = name;
    perf->self_cycle -= (int64) intrin_timestamp_counter();
}

/**
 * Manually end a profilling section that previously got manually started
 *
 * @param int32 id Profiling id
 *
 * return void
 */
inline HOT_CODE
void performance_profiler_end(int32 id) NO_EXCEPT
{
    if (!_perf_active || !*_perf_active) {
        return;
    }

    const int32 pos = atomic_get_acquire(&_perf_stats->pos) * PROFILE_SIZE;

    PerformanceProfileResult* const perf = &_perf_stats->perfs[pos + id];
    perf->self_cycle += intrin_timestamp_counter();
    perf->total_cycle = perf->self_cycle;
}

#if LOG_LEVEL > 1
    // Only these function can properly handle self-time calculation
    // Use these whenever you want to profile an entire function
    #define PROFILE(id, ...) PerformanceProfiler __profile_scope_##__func__##_##__LINE__((id), __func__, ##__VA_ARGS__)
    #define PROFILE_START(id, ...) performance_profiler_start((id), ##__VA_ARGS__)
    #define PROFILE_END(id) performance_profiler_end((id))
    #define PROFILE_SCOPE(id, name) PerformanceProfiler __profile_scope_##__func__##_##__LINE__((id), (name))
    #define PROFILE_SNAPSHOT() profile_performance_snapshot()
    #define PROFILE_LOG_TO_FILE() performance_log_to_file()
    #define PROFILE_LOG_FORMATTED() performance_log_to_file_formatted()

    #define THREAD_LOG_CREATE(id, ...) thread_profile_history_create((id), ##__VA_ARGS__)
    #define THREAD_LOG_NAME(id, name) thread_profile_name((id), (name))
    #define THREAD_LOG_DELETE(id) thread_profile_history_delete((id))
    #define THREAD_TICK(id) PerformanceThreadProfiler __profile_thread_##__func__##_##__LINE__((id))
#else
    #define PROFILE(id, ...) ((void) 0)
    #define PROFILE_START(id, ...) ((void) 0)
    #define PROFILE_END(id) ((void) 0)
    #define PROFILE_SCOPE(id, name) ((void) 0)
    #define PROFILE_SNAPSHOT() ((void) 0)
    #define PROFILE_LOG_TO_FILE() ((void) 0)
    #define PROFILE_LOG_FORMATTED() ((void) 0)

    #define THREAD_LOG_CREATE(id, ...) ((void) 0)
    #define THREAD_LOG_NAME(id, name) ((void) 0)
    #define THREAD_LOG_DELETE(id) ((void) 0)
#endif

#endif
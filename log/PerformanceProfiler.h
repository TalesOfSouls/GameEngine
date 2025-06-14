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
#include "../utils/TimeUtils.h"
#include "../thread/Spinlock.cpp"
#include "../thread/Atomic.h"
#include "../hash/GeneralHash.h"
#include "../architecture/Intrinsics.h"
#include "../compiler/CompilerUtils.h"
#include "Log.h"

#ifndef PERFORMANCE_PROFILE_STATS
    #define PERFORMANCE_PROFILE_STATS 1
    enum TimingStats {
        PROFILE_TEMP,

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

        PROFILE_SIZE,
    };
#endif

struct PerformanceProfileResult {
    alignas(8) atomic_64 const char* name;

    alignas(8) atomic_64 int64 total_cycle;
    alignas(8) atomic_64 int64 self_cycle;

    alignas(4) atomic_32 uint32 counter;
    uint32 parent;
};
static PerformanceProfileResult* _perf_stats = NULL;
static int32* _perf_active = NULL;

struct PerformanceProfiler;
static thread_local PerformanceProfiler** _perf_current_scope = NULL; // Used when sharing profiler across dlls and threads (threads unlikely)
static thread_local PerformanceProfiler* _perf_current_scope_internal; // Used when in dll or thread and no shared pointer found
struct PerformanceProfiler {
    bool is_active;

    const char* name;
    const char* info_msg;
    int32 _id;

    int64 start_cycle;
    int64 total_cycle;
    int64 self_cycle;

    PerformanceProfiler* parent;

    bool auto_log;
    bool is_stateless;

    // @question Do we want to make the self cost represent calls * "self_time/cycle"
    // Stateless allows to ONLY output to log instead of storing the performance data in an array
    PerformanceProfiler(
        int32 id, const char* scope_name, const char* info = NULL,
        bool stateless = false, bool should_log = false
    ) {
        if (!_perf_active || !*_perf_active) {
            this->is_active = false;

            return;
        }

        this->is_active = true;
        this->_id = id;
        atomic_increment_acquire_release(&_perf_stats[id].counter);

        this->name = scope_name;
        this->info_msg = info;
        this->is_stateless = stateless;
        this->auto_log = stateless || should_log;

        this->start_cycle = intrin_timestamp_counter();
        this->total_cycle = 0;
        this->self_cycle = 0;

        if (this->is_stateless) {
            this->parent = NULL;
        } else {
            if (_perf_current_scope) {
                this->parent = *_perf_current_scope;
                *_perf_current_scope = this;
            } else {
                this->parent = _perf_current_scope_internal;
                _perf_current_scope_internal = this;
            }
        }
    }

    ~PerformanceProfiler() {
        if (!this->is_active) {
            return;
        }

        uint64 end_cycle = intrin_timestamp_counter();
        this->total_cycle = OMS_MAX(end_cycle - start_cycle, 0);
        this->self_cycle += total_cycle;

        // Store result
        PerformanceProfileResult temp_perf = {};
        PerformanceProfileResult* perf = this->is_stateless ? &temp_perf : &_perf_stats[this->_id];

        perf->name = this->name;
        perf->total_cycle = this->total_cycle;
        perf->self_cycle = this->self_cycle;

        if (!this->is_stateless) {
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

        if (this->auto_log) {
            if (this->info_msg && this->info_msg[0]) {
                LOG_2(
                    "[PERF] %s (%s): %n cycles",
                    {
                        {LOG_DATA_CHAR_STR, (void *) perf->name},
                        {LOG_DATA_CHAR_STR, (void *) this->info_msg},
                        {LOG_DATA_INT64, (void *) &perf->total_cycle},
                    }
                );
            } else {
                LOG_2(
                    "[PERF] %s: %n cycles",
                    {
                        {LOG_DATA_CHAR_STR, (void *) perf->name},
                        {LOG_DATA_INT64, (void *) &perf->total_cycle},
                    }
                );
            }
        }
    }
};

inline
void performance_profiler_reset(int32 id) noexcept
{
    PerformanceProfileResult* perf = &_perf_stats[id];
    perf->total_cycle = 0;
    perf->self_cycle = 0;
    perf->parent = 0;
}

inline
void performance_profiler_start(int32 id, const char* name) noexcept
{
    PerformanceProfileResult* perf = &_perf_stats[id];
    perf->name = name;
    perf->self_cycle = -((int64) intrin_timestamp_counter());
}

inline
void performance_profiler_end(int32 id) noexcept
{
    PerformanceProfileResult* perf = &_perf_stats[id];
    perf->total_cycle = intrin_timestamp_counter() + perf->self_cycle;
    perf->self_cycle = perf->total_cycle;
}

#if LOG_LEVEL > 1
    // Only these function can properly handle self-time calculation
    // Use these whenever you want to profile an entire function
    #define PROFILE(id, ...) PerformanceProfiler __profile_scope_##__func__##_##__LINE__((id), __func__, ##__VA_ARGS__)

    #define PROFILE_START(id, name) if(_perf_active && *_perf_active) performance_profiler_start((id), (name))
    #define PROFILE_END(id) if(_perf_active && *_perf_active) performance_profiler_end((id))
    #define PROFILE_SCOPE(id, name) PerformanceProfiler __profile_scope_##__func__##_##__LINE__((id), (name))
    #define PROFILE_RESET(id) if(_perf_active && *_perf_active) performance_profiler_reset((id))
#else
    #define PROFILE(id, ...) ((void) 0)

    #define PROFILE_START(id, name) ((void) 0)
    #define PROFILE_END(id) ((void) 0)
    #define PROFILE_SCOPE(id, name) ((void) 0)
    #define PROFILE_RESET(id) ((void) 0)
#endif

#endif
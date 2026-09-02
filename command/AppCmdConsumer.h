/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_APP_COMMAND_CONSUMER_H
#define COMS_APP_COMMAND_CONSUMER_H

/**
 * The AppCmdBuffer by itself doesn't do much, it simply takes in commands and executes them.
 * The actual execution depends on the implementation of the underlying systems like:
 *      ECS, AMS, AudioMixer, ...
 * The AppCmdBuffer simplifies the interaction with those systems since the caller has to care less
 * about the information flow, function structure etc.
 * On the other hand the caller loses some control:
 *      No control over the execution order, unless additional overhead like priority gets introduced
 *      No control over what type of command are executed, unless additional overhead like command type checks get introduced
 *      ...
 * In many cases you don't need this type of control, but when you need it you should probably look at how
 * this AppCmdBuffer interacts with the individual systems and manually call those
 *
 *
 * Single threaded (_sync) vs. Multi threaded (no suffix) vs. Multi level multi threaded (_async)
 * -------------------------------------------------------------------------------------------------------------
 *
 * Some of the functions are suffixed with "_async", this means they may have to perform additional steps at a
 * later point in time. E.g. before playing audio it may have to load the audio data.
 * However the audio data loading will happen at an arbitrary point in time later and depends on the
 * amount of commands that are in the command buffer and the iteration speed of the command buffer (thread).
 *
 * Functions that don't have this suffix will perform the loading immediately and
 * then immediately issue the original command (e.g. play sound).
 * Of course there may be still some delay between the producer issuing the command and the consumer running it
 * due to the time it takes to iterate and find unhandled commands.
 * The thread loop that handles this iteration may even perform other tasks that add additional delay.
 *
 * Only functions that have the "_sync" suffix will immediately be run in a single threaded fashion but of course block.
 *
 * Functions with _internal_ in their name are intended only as internal helper functions.
 *
 *
 * Consumer vs. Producer
 * -------------------------------------------------------------------------------------------------------------
 *
 * The Command Buffer is split into consumers and producers and into different categories like Audio, Texture etc.
 * The reason for this is that if we have multiple libraries that have specific purposes, we don't have to load
 * all the code, apis etc. when we don't really use that part in that specific library.
 *
 * Of course the overall consumer needs everything because it needs to handle all the different commands.
 */
#include "../stdlib/Stdlib.h"
#include "../log/Log.h"
#include "../thread/ThreadDefines.h"
#include "../thread/ThreadPool.cpp"
#include "../memory/ChunkMemory.cpp"

#include "AppCmdBuffer.h"

#include "CmdGeneralProducer.h"
#include "CmdFileProducer.h"
#include "CmdAudioProducer.h"
#include "CmdFontProducer.h"
#include "CmdTextureProducer.h"
#include "CmdUiProducer.h"

#include "CmdAssetConsumer.h"
#include "CmdFileConsumer.h"
#include "CmdAudioConsumer.h"
#include "CmdFontConsumer.h"
#include "CmdTextureConsumer.h"
#include "CmdUiConsumer.h"
#include "CmdGpuApi.h"

inline
void cmd_buffer_create(AppCmdBuffer* const cb, BufferMemory* const buf, int32 command_capacity) NO_EXCEPT
{
    chunk_init(&cb->commands, buf, command_capacity, ASSUMED_CACHE_LINE_SIZE);
    DEBUG_MEMORY_SUBREGION(
        (uintptr_t) cb->commands.memory,
        command_capacity * sizeof(AppCommand)
    );

    LOG_1("[INFO] Created AppCmdBuffer: %n", {DATA_TYPE_INT32, &cb->commands.capacity});
}

inline
void cmd_buffer_alloc(AppCmdBuffer* const cb, int32 command_capacity) NO_EXCEPT
{
    chunk_alloc(&cb->commands, command_capacity, command_capacity, ASSUMED_CACHE_LINE_SIZE);

    LOG_1("[INFO] Created AppCmdBuffer: %n", {DATA_TYPE_INT32, &cb->commands.capacity});
}

inline
void cmd_buffer_free(AppCmdBuffer* const cb) NO_EXCEPT
{
    chunk_free(&cb->commands);
}

static inline
void* cmd_func_run(AppCommand* const cmd) NO_EXCEPT
{
    return cmd->func_body.func(cmd);
}

inline
void* cmd_func_run(AppCommandFunction func) NO_EXCEPT
{
    return func(NULL);
}

static inline void thrd_cmd_group_execute(void*);

static inline
void cmd_group_async(
    AppCmdBuffer* cb,
    AppCommand* const __restrict cmd
) NO_EXCEPT
{
    ASSERT_TRUE(cmd->group_async_body.count);

    // @todo We only chose 16 as an arbitrary number to avoid dynamic memory allocation
    //      This probably needs revisiting
    PoolWorker* jobs_ptr[16];
    ASSERT_TRUE(cmd->group_async_body.count <= ARRAY_COUNT(jobs_ptr));

    const int32 element_count = chunk_element_count(cb->mem, cmd->group_async_body.count * sizeof(AppCommandPool));
    const int32 element_id = chunk_reserve(cb->mem, element_count);
    AppCommandPool* pool_args = (AppCommandPool *) chunk_element_get(cb->mem, element_id);

    for (int32 i = 0; i < cmd->group_async_body.count; ++i) {
        pool_args[i].cb = cb;
        pool_args[i].commands = &cmd->group_async_body.commands[i];
        pool_args[i].count = 1;

        chunk_mark_complete(cb->mem, element_id + i);

        const PoolWorker job = {
            SMN(id) 0,
            SMN(state) POOL_WORKER_STATE_WAITING,
            SMN(automatic_release) false,
            SMN(arg_size) 0,
            SMN(arg) &pool_args[i],
            SMN(func) thrd_cmd_group_execute,
            SMN(callback) NULL,
            SMN(mem_size) 0,
            SMN(mem) NULL
        };
        jobs_ptr[i] = thread_pool_add_work(cb->thread_pool, &job);
    }

    const uint64 status = thread_pool_join(
        cb->thread_pool,
        jobs_ptr,
        cmd->group_async_body.count,
        100, cmd->group_async_body.count * 3 * SEC_MICRO // @todo This should probably depend on the command type?
    );

    // Release the commands memory
    int32 cmd_element_count = chunk_element_count(cb->mem, cmd->group_async_body.count * sizeof(AppCommand));
    chunk_free_elements(cb->mem, (byte *) cmd->group_async_body.commands, cmd_element_count);

    // Release the args memory used for the thread pool args
    chunk_free_elements(cb->mem, element_id, element_count);

    cmd->group_async_body.state->store(status);
}

inline
bool cmd_execute(AppCmdBuffer* const cb, AppCommand* cmd) NO_EXCEPT
{
    // Could the command be completed or did it queue subsequent commands that still need to run?
    bool completed = true;

    switch (cmd->type) {
        case CMD_FUNC_RUN: {
                cmd_func_run(cmd);
            } break;
        case CMD_ASSET_LOAD: {
                cmd_asset_load(cb->asset_archives, cb->ams, cb->mem, cmd);
            } break;
        case CMD_FILE_LOAD: {
                cmd_file_load(cb->mem, cmd);
            } break;
        case CMD_TEXTURE_ATLAS_LOAD: {
                completed = cmd_texture_atlas_load(cb, cmd) != NULL;
            } break;
        case CMD_TEXTURE_LOAD: {
                completed = cmd_texture_load(
                    cb,
                    cb->gpu_api_type,
                    cmd
                ) != NULL;
            } break;
        case CMD_INTERNAL_TEXTURE_CREATE: {
                cmd_internal_texture_create(
                    cb->ams,
                    cb->gpu_api_type,
                    cmd
                );
            } break;
        case CMD_FONT_LOAD: {
                completed = cmd_font_load(cb, cmd) != NULL;
            } break;
        case CMD_INTERNAL_FONT_CREATE: {
                cmd_internal_font_create(cb, cmd);
            } break;
        case CMD_AUDIO_PLAY: {
                completed = cmd_audio_play(cb, cmd) != NULL;
            } break;
        case CMD_INTERNAL_AUDIO_ENQUEUE: {
                completed = cmd_internal_audio_play_enqueue(cb->ams, cb->mixer, cmd) != NULL;
            } break;
        case CMD_SHADER_LOAD: {
                completed = cmd_shader_load(cb, cmd) != NULL;
            } break;
        case CMD_UI_LOAD: {
                cmd_ui_load(cb->mem, cmd);
            } break;
        case CMD_GROUP_ASYNC: {
                // @question Shouldn't this "container" function run in its own thread?
                cmd_group_async(cb, cmd);
            } break;
        default: {
            UNREACHABLE();
        }
    }

    return completed;
}

static inline
void thrd_cmd_pool_execute(void* data) {
    AppCommandPool* pool_cmd = (AppCommandPool*) data;

    cmd_execute(pool_cmd->cb, pool_cmd->commands);
    chunk_free_element(&pool_cmd->cb->commands, pool_cmd->chunk_id);
}

// Almost the same as pool_execute with the exception that
// we don't have to free the chunk element since the parent element is getting automatically removed
static inline
void thrd_cmd_group_execute(void* data) {
    AppCommandPool* pool_cmd = (AppCommandPool*) data;
    cmd_execute(pool_cmd->cb, pool_cmd->commands);
}

// Single threaded consumer that may dispatch work to multiple threads
void cmd_iterate(AppCmdBuffer* const cb) NO_EXCEPT
{
    PROFILE_DEBUG(PROFILE_CMD_ITERATE);
    int32 chunk_id = 0;
    static const int32 element_count = chunk_element_count(cb->mem, sizeof(AppCommandPool));

    thrd_chunk_iterate_start(&cb->commands, chunk_id) {
        AppCommand* cmd = (AppCommand *) chunk_element_get(&cb->commands, chunk_id);

        if (cmd->run_in_pool && !cmd->is_running) {
            const int32 pool_cmd_id = chunk_reserve(cb->mem, element_count);
            AppCommandPool* pool_cmd = (AppCommandPool *) chunk_element_get(cb->mem, pool_cmd_id);

            pool_cmd->chunk_id = chunk_id;
            pool_cmd->commands = cmd;
            chunk_mark_complete(cb->mem, pool_cmd_id);

            cmd->is_running = true;

            const PoolWorker job = {
                SMN(id) 0,
                SMN(state) POOL_WORKER_STATE_WAITING,
                SMN(automatic_release) true,
                SMN(arg_size) 0,
                SMN(arg) pool_cmd,
                SMN(func) thrd_cmd_pool_execute,
                SMN(callback) NULL,
                SMN(mem_size) 0,
                SMN(mem) NULL
            };
            thread_pool_add_work(cb->thread_pool, &job);
        } else if (!cmd->run_in_pool) {
            bool remove = cmd_execute(cb, cmd);
            if (!remove) {
                continue;
            }

            if (cmd->callback) {
                cmd->callback(cmd);
            }

            chunk_free_element(&cb->commands, chunk_id);
        }
    } thrd_chunk_iterate_end;
}

#endif
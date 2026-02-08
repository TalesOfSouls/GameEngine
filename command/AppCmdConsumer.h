/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
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
#include "../memory/ChunkMemory.h"

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
    thrd_chunk_init(&cb->commands, buf, command_capacity, ASSUMED_CACHE_LINE_SIZE);

    LOG_1("Created AppCmdBuffer: %n", {DATA_TYPE_UINT64, &cb->commands.capacity});
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

inline
bool cmd_execute(AppCmdBuffer* const cb, AppCommand* cmd) NO_EXCEPT
{
    // Could the command be completed or did it queue subsequent commands that still need to run?
    bool completed = true;

    switch (cmd->type) {
        case CMD_FUNC_RUN: {
                cmd_func_run(cmd);
            } break;
        case CMD_ASSET_ENQUEUE: {
                cmd_asset_load_enqueue(cb->assets_to_load, cmd);
            } break;
        case CMD_ASSET_LOAD: {
                cmd_asset_load(cb->asset_archives, cb->ams, cb->mem_vol, cmd);
            } break;
        case CMD_FILE_LOAD: {
                cmd_file_load(cb->mem_vol, cmd);
            } break;
        case CMD_TEXTURE_LOAD: {
                completed = cmd_texture_load_async(
                    cb->assets_to_load,
                    cb->ams,
                    cb->mem_vol,
                    cb->gpu_api_type,
                    cmd
                ) != NULL;
            } break;
        case CMD_INTERNAL_TEXTURE_CREATE: {
                cmd_internal_texture_create(
                    cb->ams,
                    cb->mem_vol,
                    cb->gpu_api_type,
                    cmd
                );
            } break;
        case CMD_FONT_LOAD: {
                completed = cmd_font_load_async(
                    cb->assets_to_load,
                    cb->ams,
                    cb->gpu_api_type,
                    cmd
                ) != NULL;
            } break;
        case CMD_INTERNAL_FONT_CREATE: {
                cmd_internal_font_create(cb->ams, cb->gpu_api_type, cmd);
            } break;
        case CMD_AUDIO_PLAY: {
                completed = cmd_audio_play_async(cb->assets_to_load, cb->ams, cb->mixer, cmd) != NULL;
            } break;
        case CMD_INTERNAL_AUDIO_ENQUEUE: {
                completed = cmd_internal_audio_play_enqueue(cb->ams, cb->mixer, cmd) != NULL;
            } break;
        case CMD_SHADER_LOAD: {
                // @todo Not yet implemented
                completed = cmd_shader_load(cb, cmd) != NULL;
            } break;
        case CMD_UI_LOAD: {
                cmd_ui_load(cb->mem_vol, cb->camera, cmd);
            } break;
        default: {
            UNREACHABLE();
        }
    }

    return completed;
}

// @question In some cases we don't remove an element if it couldn't get completed
//          Would it make more sense to remove it and add a new follow up command automatically in such cases?
//          e.g. couldn't play audio since it isn't loaded -> queue for asset load -> queue for internal play
void cmd_iterate(AppCmdBuffer* const cb) NO_EXCEPT
{
    PROFILE(PROFILE_CMD_ITERATE);
    int32 chunk_id = 0;
    chunk_iterate_start(&cb->commands, chunk_id) {
        AppCommand* cmd = (AppCommand *) chunk_get_element(&cb->commands, chunk_id);
        bool remove = cmd_execute(cb, cmd);

        if (!remove) {
            continue;
        }

        if (cmd->callback) {
            cmd->callback(cmd);
        }

        chunk_free_element(&cb->commands, chunk_id);
    } chunk_iterate_end;
}

// @performance Locking the entire thing during the iteration is horribly slow, fix.
// Solution 1: Use Queue
// Solution 2: create a mask for the chunk->free which will be set (and only then locked) after everything is done
//              This has the risk that if it takes a long time we may run out of free indices for insert
//              This shouldn't happen since the command buffer shouldn't fill up in just 1-3 frames
//              Actually, we can just use the completeness flag
inline
void thrd_cmd_iterate(AppCmdBuffer* const cb) NO_EXCEPT
{
    MutexGuard _guard(&cb->commands.lock);
    cmd_iterate(cb);
}

#endif
/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_APP_COMMAND_BUFFER_C
#define COMS_APP_COMMAND_BUFFER_C

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
 */
#include "AppCmdBuffer.h"
#include "../camera/Camera.h"
#include "../ui/UILayout.h"
#include "../ui/UILayout.cpp"
#include "../ui/UITheme.h"
#include "../log/Log.h"
#include "../scene/SceneInfo.h"
#include "../system/FileUtils.cpp"
#include "../compiler/CompilerUtils.h"

inline
void cmd_buffer_create(AppCmdBuffer* cb, BufferMemory* buf, int32 commands_count)
{
    chunk_init(&cb->commands, buf, commands_count, sizeof(Command), 64);
    mutex_init(&cb->mtx, NULL);

    LOG_1("Created AppCmdBuffer: %n B", {LOG_DATA_UINT64, &cb->commands.size});
}

// This doesn't load the file directly but tells (most likely) a worker thread to load a file
static inline
void cmd_file_load_enqueue(AppCmdBuffer* __restrict cb, Command* __restrict cmd)
{
    // cmd->data structure:
    //      start with a pointer to a callback function
    //      file path
    queue_enqueue_wait_atomic(cb->files_to_load, (byte *) cmd->data);
}

static inline
void cmd_file_load(AppCmdBuffer* __restrict cb, Command* __restrict cmd)
{
    FileBody file = {};
    file_read((const char *) cmd->data + sizeof(CommandFunction), &file, cb->thrd_mem_vol);

    // WARNING: This is not the normal cmd.callback
    // This is a special callback part of the cmd data;
    CommandFunction callback = *((CommandFunction *) cmd->data);
    callback(&file);
}

static inline
void* cmd_func_run(AppCmdBuffer*, Command* cmd)
{
    CommandFunction func = *((CommandFunction *) cmd->data);
    return func(cmd);
}

// General purpose cmd command enqueue
inline
void thrd_cmd_insert(AppCmdBuffer* __restrict cb, Command* __restrict cmd_temp)
{
    mutex_lock(&cb->mtx);
    int32 index = chunk_reserve_one(&cb->commands);
    if (index < 0) {
        mutex_unlock(&cb->mtx);
        ASSERT_TRUE(false);

        return;
    }

    if (index > cb->last_element) {
        cb->last_element = index;
    }

    Command* cmd = (Command *) chunk_get_element(&cb->commands, index);
    memcpy_aligned(cmd, cmd_temp, sizeof(Command));
    mutex_unlock(&cb->mtx);
}

inline
void thrd_cmd_insert(AppCmdBuffer* cb, CommandType type, int32 data)
{
    Command cmd;
    cmd.callback = NULL;
    cmd.type = type;
    *((int32 *) cmd.data) = data;

    thrd_cmd_insert(cb, &cmd);
}

inline
void thrd_cmd_insert(AppCmdBuffer* cb, CommandType type, const char* data)
{
    Command cmd;
    cmd.callback = NULL;
    cmd.type = type;
    str_copy((char *) cmd.data, data);

    thrd_cmd_insert(cb, &cmd);
}

inline
void thrd_cmd_insert(AppCmdBuffer* cb, CommandFunction* func) {
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_FUNC_RUN;
    *((CommandFunction *) cmd.data) = *func;

    thrd_cmd_insert(cb, &cmd);
}

inline
void* cmd_func_run(AppCmdBuffer*, CommandFunction func) {
    return func(NULL);
}

#include "CmdAsset.cpp"
#include "CmdAudio.cpp"
#include "CmdTexture.cpp"
#include "CmdFont.cpp"
#include "CmdUi.cpp"

// @question In some cases we don't remove an element if it couldn't get completed
// Would it make more sense to remove it and add a new follow up command automatically in such cases?
// e.g. couldn't play audio since it isn't loaded -> queue for asset load -> queue for internal play
// I guess this only makes sense if we would switch to a queue
// @question Some of the functions create another async call
//      E.g. do something that requires an asset, if asset not available queue for asset load.
//      Do we really want to do that or do we instead want to load the asset right then and there
//      If we do it right then and DON'T defer it, this would also solve the first question
// @question Maybe allow to pass a thread pool which if present is used for handling in worker threads
void cmd_iterate(AppCmdBuffer* cb)
{
    PROFILE(PROFILE_CMD_ITERATE);
    int32 last_element = 0;
    uint32 chunk_id = 0;
    chunk_iterate_start(&cb->commands, chunk_id) {
        Command* cmd = (Command *) chunk_get_element(&cb->commands, chunk_id);
        bool remove = true;

        switch (cmd->type) {
            case CMD_FUNC_RUN: {
                    cmd_func_run(cb, cmd);
                } break;
            case CMD_ASSET_ENQUEUE: {
                    cmd_asset_load_enqueue(cb, cmd);
                } break;
            case CMD_ASSET_LOAD: {
                    cmd_asset_load(cb, cmd);
                } break;
            case CMD_FILE_LOAD: {
                    cmd_file_load(cb, cmd);
                } break;
            case CMD_TEXTURE_LOAD: {
                    remove = cmd_texture_load_async(cb, cmd) != NULL;
                } break;
            case CMD_INTERNAL_TEXTURE_CREATE: {
                    cmd_internal_texture_create(cb, cmd);
                } break;
            case CMD_FONT_LOAD: {
                    remove = cmd_font_load_async(cb, cmd) != NULL;
                } break;
            case CMD_INTERNAL_FONT_CREATE: {
                    cmd_internal_font_create(cb, cmd);
                } break;
            case CMD_AUDIO_PLAY: {
                    cmd_audio_play_async(cb, cmd);
                } break;
            case CMD_INTERNAL_AUDIO_ENQUEUE: {
                    remove = cmd_internal_audio_play_enqueue(cb, cmd) != NULL;
                } break;
            case CMD_SHADER_LOAD: {
                    remove = cmd_shader_load(cb, cmd) != NULL;
                } break;
            case CMD_UI_LOAD: {
                    cmd_ui_load(cb, cmd);
                } break;
            default: {
                UNREACHABLE();
            }
        }

        if (!remove) {
            last_element = chunk_id;
            continue;
        }

        if (cmd->callback) {
            cmd->callback(cmd);
        }

        chunk_free_element(&cb->commands, free_index, bit_index);

        // @performance This adds some unnecessary overhead.
        // It would be better, if we could define cb->last_element as the limit in the for loop
        if (chunk_id == (uint32) cb->last_element) {
            break;
        }
    } chunk_iterate_end;

    cb->last_element = last_element;
}

// @performance Locking the entire thing during the iteration is horribly slow, fix.
// Solution 1: Use Queue
// Solution 2: create a mask for the chunk->free which will be set (and only then locked) after everything is done
//              This has the risk that if it takes a long time we may run out of free indices for insert
//              This shouldn't happen since the command buffer shouldn't fill up in just 1-3 frames
inline
void thrd_cmd_iterate(AppCmdBuffer* cb)
{
    mutex_lock(&cb->mtx);
    cmd_iterate(cb);
    mutex_unlock(&cb->mtx);
}

#endif
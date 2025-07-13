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

// @todo Move the different functions to their own respective files (e.g. CmdAsset.cpp, CmdLayout.cpp)

inline
void cmd_buffer_create(AppCmdBuffer* cb, BufferMemory* buf, int32 commands_count)
{
    chunk_init(&cb->commands, buf, commands_count, sizeof(Command), 64);
    mutex_init(&cb->mtx, NULL);

    LOG_1("Created AppCmdBuffer: %n B", {{LOG_DATA_UINT64, &cb->commands.size}});
}

// This doesn't load the asset directly but tells (most likely) a worker thread to load an asset
static inline
void cmd_asset_load_enqueue(AppCmdBuffer* __restrict cb, Command* __restrict cmd)
{
    queue_enqueue_wait_atomic(cb->assets_to_load, (byte *) cmd->data);
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

static inline
Asset* cmd_asset_load(AppCmdBuffer* __restrict cb, Command* __restrict cmd)
{
    int32 asset_id = (int32) str_to_int((char *) cmd->data);
    int32 archive_id = (asset_id >> 24) & 0xFF;
    return asset_archive_asset_load(&cb->asset_archives[archive_id], asset_id, cb->ams, cb->thrd_mem_vol);
}

static inline
Asset* cmd_audio_play_enqueue(AppCmdBuffer* __restrict cb, Command* __restrict cmd)
{
    Asset* asset = thrd_ams_get_asset_wait(cb->ams, (char *) cmd->data);
    if (!asset) {
        return asset;
    }

    // @todo How to handle settings = AudioInstance
    audio_mixer_play(
        &cb->mixer[(cmd->data + 32) ? *((int32 *) (cmd->data + 32)) : 0], // @bug how to handle multiple mixers
        asset->official_id + 1, // @bug + 1 necessary since it starts at 0, I think. we are still in the design phase :)
        (Audio *) asset->self
    );

    return asset;
}

static inline
Asset* cmd_audio_play_async(AppCmdBuffer* __restrict cb, Command* __restrict cmd)
{
    Asset* asset = thrd_ams_get_asset_wait(cb->ams, (char *) cmd->data);
    if (!asset) {
        cmd_asset_load_enqueue(cb, cmd);
    } else {
        cmd_audio_play_enqueue(cb, cmd);
    }

    return asset;
}

static inline
Asset* cmd_texture_create(AppCmdBuffer* __restrict cb, Command* __restrict cmd)
{
    Asset* asset = thrd_ams_get_asset_wait(cb->ams, (char *) cmd->data);
    if (!asset) {
        return asset;
    }

    Texture* texture = (Texture *) asset->self;
    if ((cb->gpu_api_type == GPU_API_TYPE_OPENGL || cb->gpu_api_type == GPU_API_TYPE_VULKAN)
        && !(texture->image.image_settings & IMAGE_SETTING_BOTTOM_TO_TOP)
    ) {
        image_flip_vertical(cb->thrd_mem_vol, &texture->image);
    }

    return asset;
}

static inline
Asset* cmd_texture_load_async(AppCmdBuffer* __restrict cb, Command* __restrict cmd)
{
    Asset* asset = thrd_ams_get_asset_wait(cb->ams, (char *) cmd->data);
    if (!asset) {
        cmd_asset_load_enqueue(cb, cmd);
    } else {
        cmd_texture_create(cb, cmd);
    }

    return asset;
}

static inline
Asset* cmd_font_create(AppCmdBuffer* __restrict cb, Command* __restrict cmd)
{
    Asset* asset = thrd_ams_get_asset_wait(cb->ams, (char *) cmd->data);
    if (!asset) {
        return asset;
    }

    Font* font = (Font *) asset->self;
    if (cb->gpu_api_type == GPU_API_TYPE_OPENGL || cb->gpu_api_type == GPU_API_TYPE_VULKAN) {
        font_invert_coordinates(font);
    }

    return asset;
}

static inline
Asset* cmd_font_load_async(AppCmdBuffer* __restrict cb, Command* __restrict cmd)
{
    Asset* asset = thrd_ams_get_asset_wait(cb->ams, (char *) cmd->data);
    if (!asset) {
        cmd_asset_load_enqueue(cb, cmd);
    } else {
        cmd_font_create(cb, cmd);
    }

    return asset;
}

inline
void thrd_cmd_insert(AppCmdBuffer* __restrict cb, Command* __restrict cmd_temp)
{
    mutex_lock(&cb->mtx);
    int32 index = chunk_reserve(&cb->commands, 1);
    if (index < 0) {
        mutex_unlock(&cb->mtx);
        ASSERT_SIMPLE(false);

        return;
    }

    if (index > cb->last_element) {
        cb->last_element = index;
    }

    Command* cmd = (Command *) chunk_get_element(&cb->commands, index);
    memcpy(cmd, cmd_temp, sizeof(Command));
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
    str_copy_short((char *) cmd.data, data);

    thrd_cmd_insert(cb, &cmd);
}

inline void thrd_cmd_func_insert(AppCmdBuffer* cb, CommandFunction* func) {
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_FUNC_RUN;
    *((CommandFunction *) cmd.data) = *func;

    thrd_cmd_insert(cb, &cmd);
}

inline void thrd_cmd_audio_play(AppCmdBuffer* cb, int32 data) {
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_AUDIO_PLAY;
    *((int32 *) cmd.data) = data;

    thrd_cmd_insert(cb, &cmd);
}

inline void thrd_cmd_audio_play(AppCmdBuffer* cb, const char* data) {
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_AUDIO_PLAY;
    str_copy_short((char *) cmd.data, data);

    thrd_cmd_insert(cb, &cmd);
}

inline void thrd_cmd_func_run(AppCmdBuffer* cb, CommandFunction* func) {
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_FUNC_RUN;
    *((CommandFunction *) cmd.data) = *func;

    thrd_cmd_insert(cb, &cmd);
}

inline void thrd_cmd_texture_load(AppCmdBuffer* cb, int32 data) {
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_TEXTURE_LOAD;
    *((int32 *) cmd.data) = data;

    thrd_cmd_insert(cb, &cmd);
}

inline void thrd_cmd_texture_load(AppCmdBuffer* cb, const char* data) {
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_TEXTURE_LOAD;
    str_copy_short((char *) cmd.data, data);

    thrd_cmd_insert(cb, &cmd);
}

inline void thrd_cmd_font_load(AppCmdBuffer* cb, int32 data) {
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_FONT_LOAD;
    *((int32 *) cmd.data) = data;

    thrd_cmd_insert(cb, &cmd);
}

inline void thrd_cmd_font_load(AppCmdBuffer* cb, const char* data) {
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_FONT_LOAD;
    str_copy_short((char *) cmd.data, data);

    thrd_cmd_insert(cb, &cmd);
}

inline Asset* cmd_asset_load_sync(AppCmdBuffer* cb, int32 asset_id)
{
    int32 archive_id = (asset_id >> 24) & 0xFF;
    return asset_archive_asset_load(&cb->asset_archives[archive_id], asset_id, cb->ams, cb->mem_vol);
}

inline Asset* cmd_asset_load_sync(AppCmdBuffer* cb, const char* asset_id_str)
{
    int32 asset_id = (int32) str_to_int(asset_id_str);
    int32 archive_id = (asset_id >> 24) & 0xFF;
    return asset_archive_asset_load(&cb->asset_archives[archive_id], asset_id, cb->ams, cb->mem_vol);
}

inline Asset* cmd_audio_play(AppCmdBuffer* cb, int32 asset_id)
{
    // Check if asset already loaded
    char id_str[9];
    int_to_hex(asset_id, id_str);

    Asset* asset = thrd_ams_get_asset_wait(cb->ams, id_str);

    // Load asset if not loaded
    if (!asset) {
        int32 archive_id = (asset_id >> 24) & 0xFF;
        asset = asset_archive_asset_load(&cb->asset_archives[archive_id], asset_id, cb->ams, cb->mem_vol);
    }

    // @todo How to handle settings = AudioInstance
    audio_mixer_play(
        &cb->mixer[0], // @bug how to handle multiple mixers
        asset->official_id + 1, // @bug + 1 necessary since it starts at 0, I think. we are still in the design phase :)
        (Audio *) asset->self
    );

    return asset;
}

inline Asset* cmd_audio_play(AppCmdBuffer* cb, const char* name) {
    // Check if asset already loaded
    Asset* asset = thrd_ams_get_asset_wait(cb->ams, name);

    // Load asset if not loaded
    if (!asset) {
        int32 asset_id = (int32) hex_to_int(name);
        int32 archive_id = (asset_id >> 24) & 0xFF;
        asset = asset_archive_asset_load(&cb->asset_archives[archive_id], asset_id, cb->ams, cb->mem_vol);
    }

    // @todo How to handle settings = AudioInstance
    audio_mixer_play(
        &cb->mixer[0], // @bug how to handle multiple mixers
        asset->official_id + 1, // @bug + 1 necessary since it starts at 0, I think. we are still in the design phase :)
        (Audio *) asset->self
    );

    return asset;
}

inline void* cmd_func_run(AppCmdBuffer*, CommandFunction func) {
    return func(NULL);
}

inline Asset* cmd_texture_load_sync(AppCmdBuffer* cb, int32 asset_id) {
    LOG_1("Load texture %d", {{LOG_DATA_INT32, &asset_id}});

    // Check if asset already loaded
    char id_str[9];
    int_to_hex(asset_id, id_str);
    PROFILE(PROFILE_CMD_ASSET_LOAD_SYNC, id_str, false, true);

    Asset* asset = thrd_ams_get_asset_wait(cb->ams, id_str);

    // Load asset if not loaded
    if (!asset) {
        int32 archive_id = (asset_id >> 24) & 0xFF;
        asset = asset_archive_asset_load(&cb->asset_archives[archive_id], asset_id, cb->ams, cb->mem_vol);
    }

    // Setup basic texture
    Texture* texture = (Texture *) asset->self;
    if ((cb->gpu_api_type == GPU_API_TYPE_OPENGL || cb->gpu_api_type == GPU_API_TYPE_VULKAN)
        && !(texture->image.image_settings & IMAGE_SETTING_BOTTOM_TO_TOP)
    ) {
        image_flip_vertical(cb->mem_vol, &texture->image);
    }

    // @question What about texture upload?

    return asset;
}

inline Asset* cmd_texture_load_sync(AppCmdBuffer* cb, const char* name) {
    LOG_1("Load texture %d", {{LOG_DATA_CHAR_STR, (void *) name}});
    PROFILE(PROFILE_CMD_ASSET_LOAD_SYNC, name, false, true);

    // Check if asset already loaded
    Asset* asset = thrd_ams_get_asset_wait(cb->ams, name);

    // Load asset if not loaded
    if (!asset) {
        int32 asset_id = (int32) hex_to_int(name);
        int32 archive_id = (asset_id >> 24) & 0xFF;
        asset = asset_archive_asset_load(&cb->asset_archives[archive_id], asset_id, cb->ams, cb->mem_vol);
    }

    // Setup basic texture
    Texture* texture = (Texture *) asset->self;
    if ((cb->gpu_api_type == GPU_API_TYPE_OPENGL || cb->gpu_api_type == GPU_API_TYPE_VULKAN)
        && !(texture->image.image_settings & IMAGE_SETTING_BOTTOM_TO_TOP)
    ) {
        image_flip_vertical(cb->mem_vol, &texture->image);
    }

    // @question What about texture upload?

    return asset;
}

inline Asset* cmd_font_load_sync(AppCmdBuffer* cb, int32 asset_id)
{
    LOG_1("Load font %d", {{LOG_DATA_INT32, &asset_id}});

    // Check if asset already loaded
    char id_str[9];
    int_to_hex(asset_id, id_str);

    PROFILE(PROFILE_CMD_FONT_LOAD_SYNC, id_str, false, true);

    Asset* asset = thrd_ams_get_asset_wait(cb->ams, id_str);

    // Load asset if not loaded
    if (!asset) {
        int32 archive_id = (asset_id >> 24) & 0xFF;
        asset = asset_archive_asset_load(&cb->asset_archives[archive_id], asset_id, cb->ams, cb->mem_vol);
    }

    // Setup font
    Font* font = (Font *) asset->self;
    if (cb->gpu_api_type == GPU_API_TYPE_OPENGL || cb->gpu_api_type == GPU_API_TYPE_VULKAN) {
        font_invert_coordinates(font);
    }

    // @question What about also loading the font atlas

    return asset;
}

inline Asset* cmd_font_load_sync(AppCmdBuffer* cb, const char* name)
{
    LOG_1("Load font %s", {{LOG_DATA_CHAR_STR, (void *) name}});
    PROFILE(PROFILE_CMD_FONT_LOAD_SYNC, name, false, true);

    // Check if asset already loaded
    Asset* asset = thrd_ams_get_asset_wait(cb->ams, name);

    // Load asset if not loaded
    if (!asset) {
        int32 asset_id = (int32) hex_to_int(name);
        int32 archive_id = (asset_id >> 24) & 0xFF;
        asset = asset_archive_asset_load(&cb->asset_archives[archive_id], asset_id, cb->ams, cb->mem_vol);
    }

    // Setup font
    Font* font = (Font *) asset->self;
    if (cb->gpu_api_type == GPU_API_TYPE_OPENGL || cb->gpu_api_type == GPU_API_TYPE_VULKAN) {
        font_invert_coordinates(font);
    }

    // @question What about also loading the font atlas

    return asset;
}

inline
UILayout* cmd_layout_load_sync(
    AppCmdBuffer* __restrict cb,
    UILayout* __restrict layout, const char* __restrict layout_path
) {
    PROFILE(PROFILE_CMD_LAYOUT_LOAD_SYNC, layout_path, false, true);
    LOG_1("Load layout %s", {{LOG_DATA_CHAR_STR, (void *) layout_path}});

    FileBody layout_file = {};
    file_read(layout_path, &layout_file, cb->mem_vol);

    if (!layout_file.content) {
        LOG_1("Failed loading layout \"%s\"", {{LOG_DATA_CHAR_STR, (void *) layout_path}});
        return NULL;
    }

    layout_from_data(layout_file.content, layout);

    return layout;
}

inline
UIThemeStyle* cmd_theme_load_sync(
    AppCmdBuffer* __restrict cb,
    UIThemeStyle* __restrict theme, const char* __restrict theme_path
) {
    PROFILE(PROFILE_CMD_THEME_LOAD_SYNC, theme_path, false, true);
    LOG_1("Load theme %s", {{LOG_DATA_CHAR_STR, (void *) theme_path}});

    FileBody theme_file = {};
    file_read(theme_path, &theme_file, cb->mem_vol);
    theme_from_data(theme_file.content, theme);

    return theme;
}

inline
void cmd_layout_populate_sync(
    AppCmdBuffer*,
    UILayout* layout, const UIThemeStyle* theme
) {
    layout_from_theme(layout, theme);
}

inline
UILayout* cmd_ui_load_sync(
    AppCmdBuffer* __restrict cb,
    UILayout* __restrict layout, const char* __restrict layout_path,
    UIThemeStyle* __restrict general_theme,
    UIThemeStyle* __restrict theme, const char* __restrict theme_path,
    const Camera* __restrict camera
) {
    PROFILE(PROFILE_CMD_UI_LOAD_SYNC, layout_path, false, true);
    LOG_1("Load ui with layout %s and theme %s", {{LOG_DATA_CHAR_STR, (void *) layout_path}, {LOG_DATA_CHAR_STR, (void *) theme_path}});

    if (!cmd_layout_load_sync(cb, layout, layout_path)) {
        // We have to make sure that at least the font is set
        layout->font = general_theme->font;

        return NULL;
    }

    cmd_layout_populate_sync(cb, layout, general_theme);
    cmd_theme_load_sync(cb, theme, theme_path);
    cmd_layout_populate_sync(cb, layout, theme);

    UIElement* root = layout_get_element(layout, "root");
    UIWindow* default_style = (UIWindow *) layout_get_element_style(layout, root, UI_STYLE_TYPE_DEFAULT);
    if (default_style) {
        default_style->dimension.dimension.width = camera->viewport_width;
        default_style->dimension.dimension.height = camera->viewport_height;
    }

    return layout;
}

static inline
UILayout* cmd_ui_load(AppCmdBuffer* __restrict cb, const Command* __restrict cmd)
{
    const byte* pos = cmd->data;

    SceneInfo* scene = (SceneInfo *) *((uintptr_t *) pos);
    pos += sizeof(uintptr_t);

    char* layout_path = (char *) pos;
    str_move_to((const char **) &pos, '\0'); ++pos;

    UIThemeStyle* general_theme = (UIThemeStyle *) *((uintptr_t *) pos);
    pos += sizeof(uintptr_t);

    char* theme_path = (char *) pos;
    str_move_to((const char **) &pos, '\0'); ++pos;

    const Camera* camera = (Camera *) *((uintptr_t *) pos);

    return cmd_ui_load_sync(
        cb,
        &scene->ui_layout, layout_path,
        general_theme,
        &scene->ui_theme, theme_path,
        camera
    );
}

inline
void thrd_cmd_ui_load(
    AppCmdBuffer* __restrict cb,
    SceneInfo* __restrict scene_info,
    const char* __restrict layout_path,
    UIThemeStyle* __restrict general_theme,
    const char* __restrict theme_path,
    const Camera* __restrict camera,
    CommandFunction callback
) {
    Command cmd;
    cmd.type = CMD_UI_LOAD;
    cmd.callback = callback;
    byte* pos = cmd.data;

    // Scene info pointer
    *((uintptr_t *) pos) = (uintptr_t) scene_info;
    pos += sizeof(uintptr_t);

    // Layout path
    pos += str_copy_until((char *) pos, layout_path, '\0');
    *pos = '\0'; ++pos;

    // General theme pointer
    *((uintptr_t *) pos) = (uintptr_t) general_theme;
    pos += sizeof(uintptr_t);

    // Theme path
    pos += str_copy_until((char *) pos, theme_path, '\0');
    *pos = '\0'; ++pos;

    // Camera pointer
    *((uintptr_t *) pos) = (uintptr_t) camera;

    thrd_cmd_insert(cb, &cmd);
}

// @question In some cases we don't remove an element if it couldn't get completed
// Would it make more sense to remove it and add a new follow up command automatically in such cases?
// e.g. couldn't play audio since it isn't loaded -> queue for asset load -> queue for internal play
// I guess this only makes sense if we would switch to a queue
// @question Some of the functions create another async call
//      E.g. do something that requires an asset, if asset not available queue for asset load.
//      Do we really want to do that or do we instead want to load the asset right then and there
//      If we do it right then and DON'T defer it, this would also solve the first question
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
            case CMD_TEXTURE_CREATE: {
                    // Internal only
                    cmd_texture_create(cb, cmd);
                } break;
            case CMD_FONT_LOAD: {
                    remove = cmd_font_load_async(cb, cmd) != NULL;
                } break;
            case CMD_FONT_CREATE: {
                    // Internal only
                    cmd_font_create(cb, cmd);
                } break;
            case CMD_AUDIO_PLAY: {
                    cmd_audio_play_async(cb, cmd);
                } break;
            case CMD_AUDIO_ENQUEUE: {
                    // Internal only
                    remove = cmd_audio_play_enqueue(cb, cmd) != NULL;
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
void thrd_cmd_iterate(AppCmdBuffer* cb)
{
    mutex_lock(&cb->mtx);
    cmd_iterate(cb);
    mutex_unlock(&cb->mtx);
}

#endif
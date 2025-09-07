/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_APP_COMMAND_FONT_C
#define COMS_APP_COMMAND_FONT_C

static inline
Asset* cmd_internal_font_create(AppCmdBuffer* __restrict cb, Command* __restrict cmd)
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
        cmd_internal_font_create(cb, cmd);
    }

    return asset;
}

inline
void thrd_cmd_font_load(AppCmdBuffer* cb, int32 data) {
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_FONT_LOAD;
    *((int32 *) cmd.data) = data;

    thrd_cmd_insert(cb, &cmd);
}

inline
void thrd_cmd_font_load(AppCmdBuffer* cb, const char* data) {
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_FONT_LOAD;
    str_copy((char *) cmd.data, data);

    thrd_cmd_insert(cb, &cmd);
}

inline
Asset* cmd_font_load_sync(AppCmdBuffer* cb, int32 asset_id)
{
    LOG_1("Load font %d", {LOG_DATA_INT32, &asset_id});

    // Check if asset already loaded
    char id_str[9];
    int_to_hex(asset_id, id_str);

    PROFILE(PROFILE_CMD_FONT_LOAD_SYNC, id_str, PROFILE_FLAG_SHOULD_LOG);

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

inline
Asset* cmd_font_load_sync(AppCmdBuffer* cb, const char* name)
{
    LOG_1("Load font %s", {LOG_DATA_CHAR_STR, (void *) name});
    PROFILE(PROFILE_CMD_FONT_LOAD_SYNC, name, PROFILE_FLAG_SHOULD_LOG);

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

#endif
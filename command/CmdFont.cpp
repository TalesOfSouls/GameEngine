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
Asset* cmd_internal_font_create(AppCmdBuffer* const __restrict cb, Command* const __restrict cmd) NO_EXCEPT
{
    Asset* const asset = thrd_ams_get_asset_wait(cb->ams, (char *) cmd->data);
    if (!asset) {
        return asset;
    }

    Font* const font = (Font *) asset->self;
    if (cb->gpu_api_type == GPU_API_TYPE_OPENGL
        || cb->gpu_api_type == GPU_API_TYPE_VULKAN
        || cb->gpu_api_type == GPU_API_TYPE_SOFTWARE
    ) {
        font_invert_coordinates(font);
    }

    return asset;
}

static inline
Asset* cmd_font_load_async(AppCmdBuffer* const __restrict cb, Command* const __restrict cmd) NO_EXCEPT
{
    Asset* const asset = thrd_ams_get_asset_wait(cb->ams, (char *) cmd->data);
    if (!asset) {
        cmd_asset_load_enqueue(cb, cmd);
    } else {
        cmd_internal_font_create(cb, cmd);
    }

    return asset;
}

inline
void thrd_cmd_font_load(AppCmdBuffer* const cb, int32 data) NO_EXCEPT
{
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_FONT_LOAD;
    *((int32 *) cmd.data) = data;

    thrd_cmd_insert(cb, &cmd);
}

inline
void thrd_cmd_font_load(AppCmdBuffer* const cb, const char* data) NO_EXCEPT
{
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_FONT_LOAD;
    str_copy((char *) cmd.data, data);

    thrd_cmd_insert(cb, &cmd);
}

inline
Asset* cmd_font_load_sync(AppCmdBuffer* const cb, int32 asset_id) NO_EXCEPT
{
    LOG_1("Load font %d", {DATA_TYPE_INT32, &asset_id});

    // Check if asset already loaded
    char id_str[9];
    int_to_hex(asset_id, id_str);

    PROFILE(PROFILE_CMD_FONT_LOAD_SYNC, id_str, PROFILE_FLAG_SHOULD_LOG);

    Asset* asset = thrd_ams_get_asset_wait(cb->ams, id_str);

    // Load asset if not loaded
    if (!asset) {
        const int32 archive_id = (asset_id >> 24) & 0xFF;
        asset = asset_archive_asset_load(&cb->asset_archives[archive_id], asset_id, cb->ams, cb->mem_vol);
    }

    // Setup font
    Font* const font = (Font *) asset->self;
    if (cb->gpu_api_type == GPU_API_TYPE_OPENGL
        || cb->gpu_api_type == GPU_API_TYPE_VULKAN
        || cb->gpu_api_type == GPU_API_TYPE_SOFTWARE
    ) {
        font_invert_coordinates(font);
    }

    // @question What about also loading the font atlas

    return asset;
}

inline
Asset* cmd_font_load_sync(AppCmdBuffer* const cb, const char* name) NO_EXCEPT
{
    LOG_1("Load font %s", {DATA_TYPE_CHAR_STR, (void *) name});
    PROFILE(PROFILE_CMD_FONT_LOAD_SYNC, name, PROFILE_FLAG_SHOULD_LOG);

    // Check if asset already loaded
    Asset* asset = thrd_ams_get_asset_wait(cb->ams, name);

    // Load asset if not loaded
    if (!asset) {
        const int32 asset_id = (int32) hex_to_int(name);
        const int32 archive_id = (asset_id >> 24) & 0xFF;
        asset = asset_archive_asset_load(&cb->asset_archives[archive_id], asset_id, cb->ams, cb->mem_vol);
    }

    // Setup font
    Font* font = (Font *) asset->self;
    if (cb->gpu_api_type == GPU_API_TYPE_OPENGL
        || cb->gpu_api_type == GPU_API_TYPE_VULKAN
        || cb->gpu_api_type == GPU_API_TYPE_SOFTWARE
    ) {
        font_invert_coordinates(font);
    }

    // @question What about also loading the font atlas

    return asset;
}

#endif
/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_APP_COMMAND_TEXTURE_C
#define COMS_APP_COMMAND_TEXTURE_C

static inline
Asset* cmd_internal_texture_create(AppCmdBuffer* __restrict cb, Command* __restrict cmd)
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
        cmd_internal_texture_create(cb, cmd);
    }

    return asset;
}

inline
void thrd_cmd_texture_load(AppCmdBuffer* cb, int32 data) {
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_TEXTURE_LOAD;
    *((int32 *) cmd.data) = data;

    thrd_cmd_insert(cb, &cmd);
}

inline
void thrd_cmd_texture_load(AppCmdBuffer* cb, const char* data) {
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_TEXTURE_LOAD;
    str_copy((char *) cmd.data, data);

    thrd_cmd_insert(cb, &cmd);
}

inline
Asset* cmd_texture_load_sync(AppCmdBuffer* cb, int32 asset_id) {
    LOG_1("Load texture %d", {LOG_DATA_INT32, &asset_id});

    // Check if asset already loaded
    char id_str[9];
    int_to_hex(asset_id, id_str);
    PROFILE(PROFILE_CMD_ASSET_LOAD_SYNC, id_str, PROFILE_FLAG_SHOULD_LOG);

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

inline
Asset* cmd_texture_load_sync(AppCmdBuffer* cb, const char* name) {
    LOG_1("Load texture %d", {LOG_DATA_CHAR_STR, (void *) name});
    PROFILE(PROFILE_CMD_ASSET_LOAD_SYNC, name, PROFILE_FLAG_SHOULD_LOG);

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

#endif
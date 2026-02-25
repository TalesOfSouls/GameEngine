/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_APP_COMMAND_FONT_CONSUMER_H
#define COMS_APP_COMMAND_FONT_CONSUMER_H

#include "../stdlib/Stdlib.h"
#include "../memory/QueueT.h"
#include "../font/Font.cpp"
#include "../asset/Asset.h"
#include "../asset/AssetArchive.cpp"
#include "../asset/AssetManagementSystem.cpp"
#include "../gpuapi/GpuApiType.h"
#include "AppCommand.h"

static inline
Asset* cmd_internal_font_create(
    AssetManagementSystem* const __restrict ams,
    GpuApiType gpu_api_type,
    AppCommand* const __restrict cmd
) NO_EXCEPT
{
    char id_str[9];
    int_to_hex(cmd->font_body.asset.asset_id, id_str);

    Asset* const asset = thrd_ams_get_asset_wait(ams, id_str);
    if (!asset) {
        return asset;
    }

    Font* const font = (Font *) asset->self;
    if (gpu_api_type == GPU_API_TYPE_OPENGL
        || gpu_api_type == GPU_API_TYPE_VULKAN
        || gpu_api_type == GPU_API_TYPE_SOFTWARE
    ) {
        font_invert_coordinates(font);
    }

    return asset;
}

static inline
Asset* cmd_font_load_async(
    QueueT<int32>* const __restrict assets_to_load,
    AssetManagementSystem* const __restrict ams,
    GpuApiType gpu_api_type,
    AppCommand* const __restrict cmd
) NO_EXCEPT
{
    char id_str[9];
    int_to_hex(cmd->font_body.asset.asset_id, id_str);

    Asset* const asset = thrd_ams_get_asset_wait(ams, id_str);
    if (asset) {
        //@performance The function call below also loads the asset again. That is unnecessary in this specific case
        //          Maybe we can pass the asset?
        cmd_internal_font_create(ams, gpu_api_type, cmd);
    } else {
        AppCommand asset_cmd = {0};
        asset_cmd.type = CMD_ASSET_ENQUEUE;
        asset_cmd.asset_body.asset_id = cmd->font_body.asset.asset_id;

        cmd_asset_load_enqueue(assets_to_load, &asset_cmd);
    }

    return asset;
}

inline
Asset* cmd_font_load_sync(
    const AssetArchive* const __restrict asset_archives,
    AssetManagementSystem* const __restrict ams,
    RingMemory* const __restrict ring,
    GpuApiType gpu_api_type,
    int32 asset_id
) NO_EXCEPT
{
    LOG_1("[INFO] Load font %d", {DATA_TYPE_INT32, &asset_id});

    // Check if asset already loaded
    char id_str[9];
    int_to_hex(asset_id, id_str);

    PROFILE(PROFILE_CMD_FONT_LOAD_SYNC, id_str, PROFILE_FLAG_SHOULD_LOG);

    Asset* asset = thrd_ams_get_asset_wait(ams, id_str);

    // Load asset if not loaded
    if (!asset) {
        const int32 archive_id = (asset_id >> 24) & 0xFF;
        asset = asset_archive_asset_load(&asset_archives[archive_id], asset_id, ams, ring);
    }

    // Setup font
    Font* const font = (Font *) asset->self;
    if (gpu_api_type == GPU_API_TYPE_OPENGL
        || gpu_api_type == GPU_API_TYPE_VULKAN
        || gpu_api_type == GPU_API_TYPE_SOFTWARE
    ) {
        font_invert_coordinates(font);
    }

    // @question What about also loading the font atlas

    return asset;
}

#endif
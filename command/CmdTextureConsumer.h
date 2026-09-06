/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_APP_COMMAND_TEXTURE_CONSUMER_H
#define COMS_APP_COMMAND_TEXTURE_CONSUMER_H

#include "../stdlib/Stdlib.h"
#include "../memory/QueueT.cpp"
#include "../memory/ThrdChunkMemoryT.h"
#include "../memory/ChunkMemory.h"
#include "../asset/Asset.h"
#include "../asset/AssetArchive.cpp"
#include "../object/TextureAtlas.cpp"
#include "../asset/AssetManagementSystem.cpp"
#include "../gpuapi/GpuApiType.h"
#include "AppCommand.h"
#include "CmdAssetProducer.h"
#include "CmdAssetConsumer.h"

static inline
Asset* cmd_texture_load(
    AppCmdBuffer* cb,
    GpuApiType gpu_api_type,
    AppCommand* const __restrict cmd
) NO_EXCEPT
{
    Asset* const asset = cmd_asset_load_sync(
        cb->asset_archives,
        cb->ams,
        cb->mem,
        cmd->texture_body.asset.asset_id
    );

    Texture* const texture = (Texture *) asset->self;
    if ((gpu_api_type == GPU_API_TYPE_OPENGL
        || gpu_api_type == GPU_API_TYPE_VULKAN
        || gpu_api_type == GPU_API_TYPE_SOFTWARE
    )
        && !(texture->image.image_settings & IMAGE_SETTING_BOTTOM_TO_TOP)
    ) {
        image_flip_vertical(&texture->image);
    }

    return asset;
}

template <typename T>
inline
Asset* cmd_texture_load_sync(
    const AssetArchive* const __restrict asset_archives,
    AssetManagementSystem* const __restrict ams,
    T* const __restrict mem,
    GpuApiType gpu_api_type,
    int32 asset_id
) NO_EXCEPT
{
    LOG_1("[INFO] Load texture %d", {DATA_TYPE_INT32, &asset_id});

    // Check if asset already loaded
    char id_str[9];
    int_to_hex(asset_id, id_str);
    PROFILE_DEBUG(PROFILE_CMD_ASSET_LOAD_SYNC, id_str, PROFILE_FLAG_SHOULD_LOG);

    // Load asset if not loaded
    Asset* asset = asset_archive_asset_load(
        &asset_archives[ARCHIVE_ID_FROM_ASSET_ID(asset_id)],
        asset_id,
        ams,
        mem
    );

    // Setup basic texture
    Texture* const texture = (Texture *) asset->self;
    if ((gpu_api_type == GPU_API_TYPE_OPENGL
        || gpu_api_type == GPU_API_TYPE_VULKAN
        || gpu_api_type == GPU_API_TYPE_SOFTWARE
    )
        && !(texture->image.image_settings & IMAGE_SETTING_BOTTOM_TO_TOP)
    ) {
        image_flip_vertical(&texture->image);
    }

    // @question What about texture upload?

    return asset;
}

static inline
Asset* cmd_texture_atlas_load(
    AppCmdBuffer* cb,
    AppCommand* const __restrict cmd
) NO_EXCEPT
{
    // Atlas data
    Asset* const asset = cmd_asset_load_sync(
        cb->asset_archives,
        cb->ams,
        cb->mem,
        cmd->texture_body.asset.asset_id
    );

    // Atlas image
    Asset* const texture_asset = cmd_texture_load_sync(
        cb->asset_archives,
        cb->ams,
        cb->mem,
        cb->gpu_api_type,
        asset->references[0]
    );

    TextureAtlas* const atlas = (TextureAtlas *) asset->self;
    atlas->texture = (Texture*) texture_asset->self;

    if (cb->gpu_api_type == GPU_API_TYPE_OPENGL
        || cb->gpu_api_type == GPU_API_TYPE_VULKAN
        || cb->gpu_api_type == GPU_API_TYPE_SOFTWARE
    ) {
        atlas_invert_coordinates(atlas);
    }

    return asset;
}

template <typename T>
inline
Asset* cmd_texture_atlas_load(
    const AssetArchive* const __restrict asset_archives,
    AssetManagementSystem* const __restrict ams,
    T* const __restrict mem,
    GpuApiType gpu_api_type,
    int32 asset_id
) NO_EXCEPT
{
    LOG_1("[INFO] Load texture %d", {DATA_TYPE_INT32, &asset_id});

    // Check if asset already loaded
    char id_str[9];
    int_to_hex(asset_id, id_str);
    PROFILE_DEBUG(PROFILE_CMD_ASSET_LOAD_SYNC, id_str, PROFILE_FLAG_SHOULD_LOG);

    // Load asset if not loaded
    Asset* const asset = asset_archive_asset_load(
        &asset_archives[ARCHIVE_ID_FROM_ASSET_ID(asset_id)],
        asset_id,
        ams,
        mem
    );

    Asset* const texture_asset = cmd_texture_load_sync(
        asset_archives,
        ams,
        mem,
        gpu_api_type,
        asset->references[0]
    );

    TextureAtlas* atlas = (TextureAtlas*) asset->self;
    atlas->texture = (Texture*) texture_asset->self;

    if (gpu_api_type == GPU_API_TYPE_OPENGL
        || gpu_api_type == GPU_API_TYPE_VULKAN
        || gpu_api_type == GPU_API_TYPE_SOFTWARE
    ) {
        atlas_invert_coordinates(atlas);
    }

    return asset;
}

#endif
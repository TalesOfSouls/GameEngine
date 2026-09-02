/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_APP_COMMAND_AUDIO_CONSUMER_H
#define COMS_APP_COMMAND_AUDIO_CONSUMER_H

#include "../stdlib/Stdlib.h"
#include "../memory/ThrdChunkMemoryT.cpp"
#include "../memory/ChunkMemory.cpp"
#include "../memory/QueueT.cpp"
#include "../audio/AudioMixer.cpp"
#include "../asset/Asset.h"
#include "../asset/AssetManagementSystem.cpp"
#include "../audio/Audio.h"
#include "AppCommand.h"
#include "CmdAssetProducer.h"

static inline
Asset* cmd_internal_audio_play_enqueue(
    AssetManagementSystem* const __restrict ams,
    AudioMixer* const __restrict mixer,
    const AppCommand* const __restrict cmd
) NO_EXCEPT
{
    char id_str[9];
    int_to_hex(cmd->audio_body.asset.asset_id, id_str);

    Asset* const asset = ams_asset_get_wait(ams, id_str);
    if (!asset) {
        return NULL;
    }

    // @todo How to handle settings = AudioInstance
    audio_mixer_play(
        &mixer[cmd->audio_body.mixer_id],
        asset->official_id + 1, // @bug + 1 necessary since it starts at 0, I think. we are still in the design phase
        (Audio *) asset->self
    );

    return asset;
}

static inline
Asset* cmd_audio_play(
    AppCmdBuffer* cb,
    const AppCommand* const __restrict cmd
) NO_EXCEPT
{
    char id_str[9];
    int_to_hex(cmd->audio_body.asset.asset_id, id_str);

    Asset* const asset = ams_asset_get_wait(cb->ams, id_str);
    if (!asset) {
        cmd_asset_load_sync(cb->asset_archives, cb->ams, cb->mem, cmd->audio_body.asset.asset_id);
    }

    //@performance The function call below also loads the asset again. That is unnecessary in this specific case
    //          Maybe we can pass the asset?
    cmd_internal_audio_play_enqueue(cb->ams, cb->mixer, cmd);

    return asset;
}

inline
Asset* cmd_audio_play(
    const AssetArchive* const __restrict asset_archives,
    AssetManagementSystem* const __restrict ams,
    AudioMixer* const __restrict mixer,
    ThrdChunkMemory* const __restrict mem,
    int32 asset_id
) NO_EXCEPT
{
    // Check if asset already loaded
    char id_str[9];
    int_to_hex(asset_id, id_str);

    // Load asset if not loaded
    Asset* asset = asset_archive_asset_load(
        &asset_archives[ARCHIVE_ID_FROM_ASSET_ID(asset_id)],
        asset_id,
        ams,
        mem
    );

    // @todo How to handle settings = AudioInstance
    audio_mixer_play(
        &mixer[0], // @bug how to handle multiple mixers
        asset->official_id + 1, // @bug + 1 necessary since it starts at 0, I think. we are still in the design phase
        (Audio *) asset->self
    );

    return asset;
}

#endif
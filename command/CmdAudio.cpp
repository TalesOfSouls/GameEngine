/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_APP_COMMAND_AUDIO_C
#define COMS_APP_COMMAND_AUDIO_C

static inline
Asset* cmd_internal_audio_play_enqueue(AppCmdBuffer* const __restrict cb, const Command* const __restrict cmd) NO_EXCEPT
{
    Asset* const asset = thrd_ams_get_asset_wait(cb->ams, (char *) cmd->data);
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
Asset* cmd_audio_play_async(AppCmdBuffer* const __restrict cb, const Command* const __restrict cmd) NO_EXCEPT
{
    Asset* const asset = thrd_ams_get_asset_wait(cb->ams, (char *) cmd->data);
    if (!asset) {
        cmd_asset_load_enqueue(cb, cmd);
    } else {
        cmd_internal_audio_play_enqueue(cb, cmd);
    }

    return asset;
}

inline
void thrd_cmd_audio_play(AppCmdBuffer* const cb, int32 data) NO_EXCEPT
{
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_AUDIO_PLAY;
    *((int32 *) cmd.data) = data;

    thrd_cmd_insert(cb, &cmd);
}

inline
void thrd_cmd_audio_play(AppCmdBuffer* const cb, const char* data) NO_EXCEPT
{
    Command cmd;
    cmd.callback = NULL;
    cmd.type = CMD_AUDIO_PLAY;
    str_copy((char *) cmd.data, data);

    thrd_cmd_insert(cb, &cmd);
}

inline
Asset* cmd_audio_play(AppCmdBuffer* const cb, int32 asset_id) NO_EXCEPT
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

inline
Asset* cmd_audio_play(AppCmdBuffer* const cb, const char* const name) NO_EXCEPT
{
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

#endif
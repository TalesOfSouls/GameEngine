/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_APP_COMMAND_ASSET_C
#define COMS_APP_COMMAND_ASSET_C

// This doesn't load the asset directly but tells (most likely) a worker thread to load an asset
static inline
void cmd_asset_load_enqueue(AppCmdBuffer* __restrict cb, Command* __restrict cmd) NO_EXCEPT
{
    queue_enqueue_wait_atomic(cb->assets_to_load, (byte *) cmd->data);
}

static inline
Asset* cmd_asset_load(AppCmdBuffer* __restrict cb, Command* __restrict cmd) NO_EXCEPT
{
    int32 asset_id = (int32) str_to_int((char *) cmd->data);
    int32 archive_id = (asset_id >> 24) & 0xFF;
    return asset_archive_asset_load(&cb->asset_archives[archive_id], asset_id, cb->ams, cb->thrd_mem_vol);
}

FORCE_INLINE
Asset* cmd_asset_load_sync(AppCmdBuffer* cb, int32 asset_id) NO_EXCEPT
{
    int32 archive_id = (asset_id >> 24) & 0xFF;
    return asset_archive_asset_load(&cb->asset_archives[archive_id], asset_id, cb->ams, cb->mem_vol);
}

FORCE_INLINE
Asset* cmd_asset_load_sync(AppCmdBuffer* cb, const char* asset_id_str) NO_EXCEPT
{
    int32 asset_id = (int32) str_to_int(asset_id_str);
    int32 archive_id = (asset_id >> 24) & 0xFF;
    return asset_archive_asset_load(&cb->asset_archives[archive_id], asset_id, cb->ams, cb->mem_vol);
}

#endif
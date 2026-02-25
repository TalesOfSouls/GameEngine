/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_APP_COMMAND_ASSET_CONSUMER_H
#define COMS_APP_COMMAND_ASSET_CONSUMER_H

#include "../stdlib/Stdlib.h"
#include "../asset/AssetArchive.cpp"
#include "../asset/AssetManagementSystem.cpp"
#include "../memory/RingMemory.cpp"
#include "AppCommand.h"

static inline
Asset* cmd_asset_load(
    const AssetArchive* const __restrict asset_archives,
    AssetManagementSystem* const __restrict ams,
    RingMemory* const __restrict ring,
    const AppCommand* const __restrict cmd
) NO_EXCEPT
{
    const int32 asset_id = cmd->asset_body.asset_id;
    const int32 archive_id = (asset_id >> 24) & 0xFF;
    return asset_archive_asset_load(&asset_archives[archive_id], asset_id, ams, ring);
}

FORCE_INLINE
Asset* cmd_asset_load_sync(
    const AssetArchive* const __restrict asset_archives,
    AssetManagementSystem* const __restrict ams,
    RingMemory* const __restrict ring,
    int32 asset_id
) NO_EXCEPT
{
    const int32 archive_id = (asset_id >> 24) & 0xFF;
    return asset_archive_asset_load(&asset_archives[archive_id], asset_id, ams, ring);
}

#endif
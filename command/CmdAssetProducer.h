/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_APP_COMMAND_ASSET_PRODUCER_H
#define COMS_APP_COMMAND_ASSET_PRODUCER_H

#include "../stdlib/Stdlib.h"
#include "../memory/QueueT.h"
#include "AppCommand.h"

// This doesn't load the asset directly but tells (most likely) a worker thread to load an asset
static inline
void cmd_asset_load_enqueue(
    QueueT<int32>* const __restrict assets_to_load,
    const AppCommand* const __restrict cmd
) NO_EXCEPT
{
    thrd_queue_enqueue_wait(assets_to_load, &cmd->asset_body.asset_id);
}

static inline
void cmd_asset_load_enqueue(
    QueueT<int32>* const __restrict assets_to_load,
    int32 asset_id
) NO_EXCEPT
{
    thrd_queue_enqueue_wait(assets_to_load, &asset_id);
}

#endif
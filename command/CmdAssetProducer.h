/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_APP_COMMAND_ASSET_PRODUCER_H
#define COMS_APP_COMMAND_ASSET_PRODUCER_H

#include "../stdlib/Stdlib.h"
#include "AppCommand.h"
#include "CmdGeneralProducer.h"

inline
void thrd_cmd_asset_load(
    ChunkMemoryT<AppCommand>* const cb,
    int32 asset_id
) NO_EXCEPT
{
    AppCommand cmd;
    cmd.callback = NULL;
    cmd.type = CMD_ASSET_LOAD;
    cmd.asset_body.asset_id = asset_id;

    thrd_cmd_insert(cb, &cmd);
}

#endif
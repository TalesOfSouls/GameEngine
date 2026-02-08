/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMMAND_BUFFER_AUDIO_PRODUCER_H
#define COMS_COMMAND_BUFFER_AUDIO_PRODUCER_H

#include "../stdlib/Stdlib.h"
#include "../memory/ChunkMemoryT.h"
#include "AppCommand.h"
#include "CmdGeneralProducer.h"

inline
void thrd_cmd_audio_play(
    ChunkMemoryT<AppCommand>* const cb,
    int32 asset_id
) NO_EXCEPT
{
    AppCommand cmd;
    cmd.callback = NULL;
    cmd.type = CMD_AUDIO_PLAY;
    cmd.audio_body.asset.asset_id = asset_id;

    thrd_cmd_insert(cb, &cmd);
}

#endif
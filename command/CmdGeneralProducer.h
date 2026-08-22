/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_COMMAND_BUFFER_GENERAL_PRODUCER_H
#define COMS_COMMAND_BUFFER_GENERAL_PRODUCER_H

#include "../stdlib/Stdlib.h"
#include "../memory/ThrdChunkMemoryT.cpp"
#include "../thread/ThreadHelper.cpp"
#include "AppCommand.h"

// General purpose cmd command enqueue
inline
void thrd_cmd_insert(ThrdChunkMemoryT<AppCommand>* const __restrict cb, const AppCommand* const __restrict cmd_temp) NO_EXCEPT
{
    const int32 index = chunk_reserve_one(cb);
    if (index < 0) {
        ASSERT_THROW();

        return;
    }

    AppCommand* cmd = (AppCommand *) chunk_element_get(cb, index);
    memcpy(cmd, cmd_temp, sizeof(AppCommand));
    chunk_mark_complete(cb, index);
}

inline
void thrd_cmd_insert(ThrdChunkMemoryT<AppCommand>* const cb, AppCommandFunction const func) NO_EXCEPT
{
    AppCommand cmd;
    cmd.callback = NULL;
    cmd.type = CMD_FUNC_RUN;
    cmd.func_body.func = func;

    thrd_cmd_insert(cb, &cmd);
}

#endif
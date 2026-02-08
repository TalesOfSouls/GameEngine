/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMMAND_BUFFER_GENERAL_PRODUCER_H
#define COMS_COMMAND_BUFFER_GENERAL_PRODUCER_H

#include "../stdlib/Stdlib.h"
#include "../memory/ChunkMemoryT.h"
#include "../memory/QueueT.h"
#include "../thread/ThreadDefines.h"
#include "AppCommand.h"

// General purpose cmd command enqueue
inline
void thrd_cmd_insert(ChunkMemoryT<AppCommand>* const __restrict cb, AppCommand* const __restrict cmd_temp) NO_EXCEPT
{
    // @performance Consider to replace with atomic operations
    MutexGuard _guard(&cb->lock);
    const int32 index = chunk_reserve_one(cb);
    if (index < 0) {
        ASSERT_TRUE(false);

        return;
    }

    AppCommand* cmd = (AppCommand *) chunk_get_element(cb, index);
    memcpy(cmd, cmd_temp, sizeof(AppCommand));
}

inline
void thrd_cmd_insert(ChunkMemoryT<AppCommand>* const cb, AppCommandFunction const func) NO_EXCEPT
{
    AppCommand cmd;
    cmd.callback = NULL;
    cmd.type = CMD_FUNC_RUN;
    cmd.func_body.func = func;

    thrd_cmd_insert(cb, &cmd);
}

#endif
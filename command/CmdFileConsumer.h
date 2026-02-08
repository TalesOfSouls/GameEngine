/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMMAND_BUFFER_PRODUCER_FILE_C
#define COMS_COMMAND_BUFFER_PRODUCER_FILE_C

#include "../stdlib/Stdlib.h"
#include "../memory/QueueT.h"
#include "../system/FileUtils.cpp"
#include "AppCommand.h"

static inline
void cmd_file_load(
    RingMemory* const __restrict ring,
    AppCommand* const __restrict cmd
) NO_EXCEPT
{
    FileBody file = {0};
    file_read(cmd->file_body.file_path, &file, ring);

    // WARNING: This is not the normal cmd.callback
    // This is a special callback part of the cmd data;
    if (cmd->file_body.callback) {
        cmd->file_body.callback(&file);
    }
}

#endif
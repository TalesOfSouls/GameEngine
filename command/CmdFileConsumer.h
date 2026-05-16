/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_COMMAND_BUFFER_PRODUCER_FILE_C
#define COMS_COMMAND_BUFFER_PRODUCER_FILE_C

#include "../stdlib/Stdlib.h"
#include "../memory/ChunkMemory.cpp"
#include "../system/FileUtils.cpp"
#include "AppCommand.h"

static inline
void cmd_file_load(
    ChunkMemory* const __restrict mem,
    AppCommand* const __restrict cmd
) NO_EXCEPT
{
    FileBody file;
    file.size = file_size(cmd->file_body.file_path);
    THRD_CHUNK_STACK_MEMORY(mem, &file.content, file.size + 1);
    file_read(cmd->file_body.file_path, &file);

    // WARNING: This is not the normal cmd.callback
    // This is a special callback part of the cmd data;
    if (cmd->file_body.callback) {
        cmd->file_body.callback(&file);
    }
}

#endif
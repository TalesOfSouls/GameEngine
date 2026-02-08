/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_COMMAND_BUFFER_FILE_PRODUCER_H
#define COMS_COMMAND_BUFFER_FILE_PRODUCER_H

#include "../stdlib/Stdlib.h"
#include "../memory/QueueT.h"
#include "../system/FileUtils.cpp"
#include "AppCommand.h"

// This doesn't load the file directly but tells (most likely) a worker thread to load a file
static inline
void cmd_file_load_enqueue(
    QueueT<FileToLoad>* const __restrict files_to_load,
    AppCommand* const __restrict cmd
) NO_EXCEPT
{
    thrd_queue_enqueue_wait(files_to_load, cmd->file_body.file_to_load);
}

#endif
/**
 * Jingga
 *
 * @copyright Jingga
 * @license   OMS License 2.0
 * @version   1.0.0
 * @link      https://jingga.app
 */
#ifndef COMS_APP_COMMAND_BUFFER_H
#define COMS_APP_COMMAND_BUFFER_H

#include "../stdlib/Stdlib.h"
#include "../memory/ChunkMemoryT.h"
#include "../memory/RingMemory.cpp"
#include "../audio/AudioMixer.h"
#include "../asset/AssetArchive.h"
#include "../gpuapi/GpuApiType.h"
#include "../asset/AssetManagementSystem.h"
#include "../memory/QueueT.h"
#include "../system/FileUtils.h"
#include "../thread/ThreadDefines.h"
#include "../camera/Camera.h"
#include "AppCommand.h"

// The Application AppCommand Buffer is a shotgun tool to run commands in a "generalized" way
// The developer can enqueue pre-defined command types which are then run
// You can also think of this as an event queue.
struct AppCmdBuffer {
    // @performance A queue would be much faster than ChunkMemory.
    // We only use Chunk memory since we might want to run only certain commands instead of all of them
    ChunkMemoryT<AppCommand> commands;

    // @bug Currently we never differentiate between multi threaded and single threaded
    //      We need to adjust the functions so that they allocate memory accordingly
    //      Especially the file_read function currently doesn't use a multi threaded ring usage
    //      We probably need thrd_file_read. I don't like it a file_read overload would be nicer for ThreadedRingMemory,
    //      but we discarded ThreadedRingMemory and put it in the normal RingMemory.
    RingMemory* mem_vol;
    AssetManagementSystem* ams;
    AssetArchive* asset_archives;
    QueueT<int32>* assets_to_load;
    QueueT<FileToLoad>* files_to_load;
    AudioMixer* mixer;
    GpuApiType gpu_api_type;

    Camera* camera;
};

#endif
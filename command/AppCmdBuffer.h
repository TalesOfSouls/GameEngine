/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_APP_COMMAND_BUFFER_H
#define COMS_APP_COMMAND_BUFFER_H

#include "../stdlib/Stdlib.h"
#include "../memory/ThrdChunkMemoryT.h"
#include "../memory/ChunkMemory.h"
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
    ThrdChunkMemoryT<AppCommand> commands;

    // Memory that can be used in threads as MC even though we have only one consumer
    // Why is it threaded though? -> Well, because we want to avoid too many memory pools
    ThrdChunkMemory* mem;
    AssetManagementSystem* ams;
    AssetArchive* asset_archives;
    AudioMixer* mixer;
    GpuApiType gpu_api_type;

    Camera* camera;
};

#endif
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

#include "../stdlib/Types.h"
#include "../memory/ChunkMemory.h"
#include "../memory/RingMemory.h"
#include "../audio/AudioMixer.h"
#include "../audio/Audio.h"
#include "../asset/AssetArchive.h"
#include "../gpuapi/GpuApiType.h"
#include "../asset/Asset.h"
#include "../asset/AssetManagementSystem.h"
#include "../object/Texture.h"
#include "../memory/Queue.h"
#include "../system/FileUtils.cpp"
#include "Command.h"

struct AppCmdBuffer {
    // @performance A queue would be much faster than ChunkMemory.
    // We only use Chunk memory since we might want to run only certain commands instead of all of them
    ChunkMemory commands;
    int32 last_element;

    mutex mtx;

    // Application data for cmd access
    // The list below depends on what kind of systems our command buffer needs access to
    // Memory for when a buffer function (e.g. load_asset) is run in a thread context
    RingMemory* thrd_mem_vol;

    // Memory for when a buffer function (e.g. load_asset) is run in the main loop
    RingMemory* mem_vol;
    AssetManagementSystem* ams;
    AssetArchive* asset_archives;
    Queue* assets_to_load;
    Queue* files_to_load;
    AudioMixer* mixer;
    GpuApiType gpu_api_type;
};

#if OPENGL
    #include "../gpuapi/opengl/AppCmdBuffer.h"
#elif VULKAN
    #include "../gpuapi/vulkan/AppCmdBuffer.h"
#elif DIRECTX
    #include "../gpuapi/direct3d/AppCmdBuffer.h"
#else
    inline void* cmd_shader_load(AppCmdBuffer*, Command*) { return NULL; }
    inline void* cmd_shader_load_sync(AppCmdBuffer*, void*, const int32*, ...) { return NULL; }
#endif

#endif
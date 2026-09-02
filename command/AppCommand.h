/**
 * @copyright Jingga
 * @license   OMS License 2.0
 * @link      https://jingga.app
 */
#pragma once
#ifndef COMS_COMMAND_H
#define COMS_COMMAND_H

#include "../stdlib/Stdlib.h"
#include "../ui/UILayout.h"
#include "../ui/UITheme.h"
#include "../scene/SceneInfo.h"
#include "../asset/AssetManagementSystem.h"

// WARNING: INTERNAL types are only intended for internal use (not by other programmers/systems)
//          Currently the INTERNAL command type is never used at least not in terms of the normal command pool iteration
enum AppCommandType : uint8 {
    CMD_FUNC_RUN,
    CMD_ASSET_LOAD,
    CMD_FILE_LOAD,
    CMD_FONT_LOAD,
    CMD_INTERNAL_FONT_CREATE,
    CMD_TEXTURE_LOAD,
    CMD_TEXTURE_ATLAS_LOAD,
    CMD_INTERNAL_TEXTURE_CREATE,
    CMD_AUDIO_PLAY,
    CMD_INTERNAL_AUDIO_ENQUEUE,
    CMD_SHADER_LOAD,
    CMD_UI_LOAD,
    CMD_GROUP_ASYNC,
};

typedef void* (*AppCommandFunction)(void* data);

struct CmdFunctionBody {
    AppCommandFunction func;
    void* parameter;
};

struct CmdFileBody {
    AppCommandFunction callback;

    union {
        const wchar_t* file_path;
        FileToLoad* file_to_load;
    };
};

struct CmdAssetBody {
    union {
        int32 asset_id;
        const wchar_t* asset_path;
    };
};

struct CmdAudioBody {
    int32 mixer_id;

    CmdAssetBody asset;
};

struct CmdFontBody {
    CmdAssetBody asset;
};

struct CmdTextureBody {
    CmdAssetBody asset;
};

struct AppCommand;
struct CmdGroupAsyncBody {
    int count;

    // The child commands need to be stored in a persistent memory region
    // The memory is "released" after the group finished
    AppCommand* commands;

    // This is used to indicate the producer the state and needs to be stored
    // in a persistent memory region
    atomic<size_t>* state;
};

struct CmdLayoutBody {
    void* app;
    AssetManagementSystem* ams;

    UITheme* general_theme;

    wchar_t layout_path[64];
    wchar_t theme_path[64];

    SceneInfo* scene_info;

    GpuApiType gpu_api_type;

    ThrdChunkMemory* mem;
};

// Another name for this concept is event queue and the command below is a generic event
struct AppCommand {
    AppCommandFunction callback;
    AppCommandType type;

    // Should run in a thread pool worker
    bool run_in_pool;

    // Required for thread pool commands to avoid multiple executions
    bool is_running;

    // This defines the actual size of AppCommand
    union {
        CmdAssetBody asset_body;
        CmdAudioBody audio_body;
        CmdFontBody font_body;
        CmdFunctionBody func_body;
        CmdFileBody file_body;
        CmdTextureBody texture_body;
        CmdLayoutBody layout_body;
        CmdGroupAsyncBody group_async_body;
    };
};

#endif